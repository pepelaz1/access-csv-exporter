#include "OdbcCsvExporter.h"
#include "CsvFormat.h"

#include <Windows.h>
#include <sql.h>
#include <sqlext.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "odbc32.lib")

namespace
{
    std::string Diagnostic(SQLSMALLINT kind, SQLHANDLE handle)
    {
        SQLWCHAR state[6]{};
        SQLWCHAR message[1024]{};
        SQLINTEGER nativeError = 0;
        SQLSMALLINT length = 0;
        if (SQL_SUCCEEDED(SQLGetDiagRecW(kind, handle, 1, state, &nativeError,
            message, static_cast<SQLSMALLINT>(std::size(message)), &length)))
            return access_csv::ToUtf8(reinterpret_cast<const wchar_t*>(message));
        return "Unknown ODBC error.";
    }

    void Check(SQLRETURN result, SQLSMALLINT kind, SQLHANDLE handle, const char* action)
    {
        if (!SQL_SUCCEEDED(result))
            throw std::runtime_error(std::string(action) + ": " + Diagnostic(kind, handle));
    }

    class Connection
    {
    public:
        explicit Connection(const std::wstring& databasePath)
        {
            if (!std::filesystem::is_regular_file(databasePath))
                throw std::runtime_error("Access database file was not found.");

            if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &environment_)))
                throw std::runtime_error("Cannot initialize ODBC.");
            try
            {
                Check(SQLSetEnvAttr(environment_, SQL_ATTR_ODBC_VERSION,
                    reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0),
                    SQL_HANDLE_ENV, environment_, "Set ODBC version");
                Check(SQLAllocHandle(SQL_HANDLE_DBC, environment_, &database_),
                    SQL_HANDLE_ENV, environment_, "Create ODBC connection");

                const std::wstring connectionString =
                    L"DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};DBQ=" +
                    databasePath + L";";
                Check(SQLDriverConnectW(database_, nullptr,
                    reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(connectionString.c_str())),
                    SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT),
                    SQL_HANDLE_DBC, database_, "Open Access database");
                connected_ = true;
            }
            catch (...)
            {
                Close();
                throw;
            }
        }

        ~Connection() { Close(); }
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        SQLHDBC Get() const { return database_; }

    private:
        void Close()
        {
            if (connected_) SQLDisconnect(database_);
            if (database_ != SQL_NULL_HDBC) SQLFreeHandle(SQL_HANDLE_DBC, database_);
            if (environment_ != SQL_NULL_HENV) SQLFreeHandle(SQL_HANDLE_ENV, environment_);
            database_ = SQL_NULL_HDBC;
            environment_ = SQL_NULL_HENV;
            connected_ = false;
        }

        SQLHENV environment_ = SQL_NULL_HENV;
        SQLHDBC database_ = SQL_NULL_HDBC;
        bool connected_ = false;
    };

    class Statement
    {
    public:
        explicit Statement(SQLHDBC database)
        {
            Check(SQLAllocHandle(SQL_HANDLE_STMT, database, &handle_),
                SQL_HANDLE_DBC, database, "Create ODBC statement");
        }
        ~Statement() { if (handle_ != SQL_NULL_HSTMT) SQLFreeHandle(SQL_HANDLE_STMT, handle_); }
        Statement(const Statement&) = delete;
        Statement& operator=(const Statement&) = delete;
        SQLHSTMT Get() const { return handle_; }
    private:
        SQLHSTMT handle_ = SQL_NULL_HSTMT;
    };

    std::wstring ReadText(SQLHSTMT statement, SQLUSMALLINT column)
    {
        std::wstring result;
        wchar_t buffer[2048]{};
        SQLLEN indicator = 0;
        for (;;)
        {
            const SQLRETURN status = SQLGetData(statement, column, SQL_C_WCHAR,
                buffer, sizeof(buffer), &indicator);
            if (status == SQL_NO_DATA) break;
            Check(status, SQL_HANDLE_STMT, statement, "Read Access value");
            if (indicator == SQL_NULL_DATA) return {};
            result.append(buffer, wcsnlen_s(buffer, std::size(buffer)));
            if (status == SQL_SUCCESS) break;
        }
        return result;
    }

    bool IsBinary(SQLSMALLINT type)
    {
        return type == SQL_BINARY || type == SQL_VARBINARY || type == SQL_LONGVARBINARY;
    }
}

namespace access_csv
{
    std::vector<std::wstring> ListTables(const std::wstring& databasePath)
    {
        Connection connection(databasePath);
        Statement statement(connection.Get());
        const wchar_t* tableType = L"TABLE";
        Check(SQLTablesW(statement.Get(), nullptr, 0, nullptr, 0, nullptr, 0,
            reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(tableType)), SQL_NTS),
            SQL_HANDLE_STMT, statement.Get(), "List Access tables");

        std::vector<std::wstring> tables;
        for (;;)
        {
            const SQLRETURN status = SQLFetch(statement.Get());
            if (status == SQL_NO_DATA) break;
            Check(status, SQL_HANDLE_STMT, statement.Get(), "Read table list");
            std::wstring name = ReadText(statement.Get(), 3);
            if (!name.empty() && name.find_first_of(L"[]") == std::wstring::npos)
                tables.push_back(std::move(name));
        }
        std::sort(tables.begin(), tables.end());
        return tables;
    }

    std::size_t ExportTable(const std::wstring& databasePath,
                            const std::wstring& tableName,
                            const std::wstring& outputPath)
    {
        if (tableName.empty() || tableName.find_first_of(L"[]") != std::wstring::npos)
            throw std::runtime_error("Choose a valid table name.");
        if (outputPath.empty()) throw std::runtime_error("Choose an output CSV file.");
        const auto source = std::filesystem::absolute(databasePath).lexically_normal();
        const auto target = std::filesystem::absolute(outputPath).lexically_normal();
        if (_wcsicmp(source.c_str(), target.c_str()) == 0 ||
            (std::filesystem::exists(source) && std::filesystem::exists(target) &&
             std::filesystem::equivalent(source, target)))
            throw std::runtime_error("The CSV path must differ from the database path.");

        const auto tables = ListTables(databasePath);
        if (std::find(tables.begin(), tables.end(), tableName) == tables.end())
            throw std::runtime_error("The selected table was not found in this database.");

        Connection connection(databasePath);
        Statement statement(connection.Get());
        const std::wstring sql = L"SELECT * FROM [" + tableName + L"]";
        Check(SQLExecDirectW(statement.Get(),
            reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(sql.c_str())), SQL_NTS),
            SQL_HANDLE_STMT, statement.Get(), "Read Access table");

        SQLSMALLINT columnCount = 0;
        Check(SQLNumResultCols(statement.Get(), &columnCount),
            SQL_HANDLE_STMT, statement.Get(), "Read column count");
        std::vector<std::wstring> headers;
        for (SQLUSMALLINT column = 1; column <= columnCount; ++column)
        {
            SQLWCHAR name[512]{};
            SQLSMALLINT nameLength = 0;
            SQLSMALLINT dataType = 0;
            SQLULEN columnSize = 0;
            SQLSMALLINT digits = 0;
            SQLSMALLINT nullable = 0;
            Check(SQLDescribeColW(statement.Get(), column, name,
                static_cast<SQLSMALLINT>(std::size(name)), &nameLength, &dataType,
                &columnSize, &digits, &nullable),
                SQL_HANDLE_STMT, statement.Get(), "Read column description");
            if (IsBinary(dataType))
                throw std::runtime_error("Binary columns are not supported by this demo.");
            headers.emplace_back(reinterpret_cast<wchar_t*>(name), nameLength);
        }

        const std::filesystem::path destination(outputPath);
        const std::filesystem::path directory = destination.parent_path();
        if (!directory.empty()) std::filesystem::create_directories(directory);
        const std::filesystem::path temporary = destination.wstring() + L"." +
            std::to_wstring(GetCurrentProcessId()) + L"." +
            std::to_wstring(GetTickCount64()) + L".partial";
        std::size_t rows = 0;
        try
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) throw std::runtime_error("Cannot create the CSV file.");
            output.write("\xEF\xBB\xBF", 3);
            WriteCsvRow(output, headers);
            for (;;)
            {
                const SQLRETURN status = SQLFetch(statement.Get());
                if (status == SQL_NO_DATA) break;
                Check(status, SQL_HANDLE_STMT, statement.Get(), "Read Access row");
                std::vector<std::wstring> fields;
                for (SQLUSMALLINT column = 1; column <= columnCount; ++column)
                    fields.push_back(ReadText(statement.Get(), column));
                WriteCsvRow(output, fields);
                ++rows;
            }
            output.close();
            if (!output) throw std::runtime_error("Cannot finish writing the CSV file.");
            if (!MoveFileExW(temporary.c_str(), destination.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                throw std::runtime_error("Cannot replace the destination CSV file.");
        }
        catch (...)
        {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            throw;
        }
        return rows;
    }
}
