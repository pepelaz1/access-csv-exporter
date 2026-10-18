#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace access_csv
{
    std::string ToUtf8(std::wstring_view value);
    void WriteCsvRow(std::ostream& output, const std::vector<std::wstring>& fields);
}
