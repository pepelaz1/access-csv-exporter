#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace access_csv
{
    std::vector<std::wstring> ListTables(const std::wstring& databasePath);

    // Writes a UTF-8 CSV with a header row. The existing output is replaced only
    // after the complete export has been written to a temporary file.
    std::size_t ExportTable(const std::wstring& databasePath,
                            const std::wstring& tableName,
                            const std::wstring& outputPath);
}
