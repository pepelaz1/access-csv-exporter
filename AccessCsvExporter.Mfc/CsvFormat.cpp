#include "CsvFormat.h"

#include <Windows.h>

#include <stdexcept>

namespace access_csv
{
    std::string ToUtf8(std::wstring_view value)
    {
        if (value.empty()) return {};
        const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
            value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (size == 0) throw std::runtime_error("Cannot encode text as UTF-8.");
        std::string result(size, '\0');
        if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), size, nullptr, nullptr) == 0)
            throw std::runtime_error("Cannot encode text as UTF-8.");
        return result;
    }

    void WriteCsvRow(std::ostream& output, const std::vector<std::wstring>& fields)
    {
        for (std::size_t index = 0; index < fields.size(); ++index)
        {
            if (index != 0) output.put(',');
            const std::wstring_view value(fields[index]);
            const bool quoted = value.find_first_of(L",\"\r\n") != std::wstring_view::npos;
            std::wstring escaped;
            escaped.reserve(value.size() + 2);
            if (quoted) escaped.push_back(L'"');
            for (const wchar_t ch : value)
            {
                if (ch == L'"') escaped.push_back(L'"');
                escaped.push_back(ch);
            }
            if (quoted) escaped.push_back(L'"');
            const std::string utf8 = ToUtf8(escaped);
            output.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
        }
        output.write("\r\n", 2);
    }
}
