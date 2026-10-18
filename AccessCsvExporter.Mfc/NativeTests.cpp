#include "CsvFormat.h"
#include "OdbcCsvExporter.h"

#include <sstream>
#include <stdexcept>

int main()
{
    std::ostringstream output;
    access_csv::WriteCsvRow(output, {L"Code", L"Description", L"Note"});
    access_csv::WriteCsvRow(output, {L"42", L"Камера, 4K \"Pro\"\nновая", L""});
    const std::string expected =
        "Code,Description,Note\r\n"
        "42,\"Камера, 4K \"\"Pro\"\"\nновая\",\r\n";
    if (output.str() != expected)
        throw std::runtime_error("CSV quoting or Unicode formatting failed.");

    bool missingDatabaseReported = false;
    try
    {
        (void)access_csv::ListTables(L"missing-test-database.accdb");
    }
    catch (const std::exception&)
    {
        missingDatabaseReported = true;
    }
    if (!missingDatabaseReported)
        throw std::runtime_error("Missing Access database was not reported.");
}
