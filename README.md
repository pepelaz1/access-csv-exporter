# Access CSV Exporter

A small native Windows desktop app built with C++ and MFC. Select a Microsoft Access `.accdb` or `.mdb` file, load its tables, choose one, and export it to UTF-8 CSV.

![Access to CSV desktop window](AccessCsvExporter.Mfc/screenshots/main-window.jpg)

## Build

1. Install Visual Studio 2022 with **Desktop development with C++**. In **Individual components**, include **C++ MFC for latest v143 build tools (x86 & x64)**, the MSVC v143 compiler, and a Windows SDK.
2. Open [`AccessCsvExporter.Mfc.sln`](AccessCsvExporter.Mfc/AccessCsvExporter.Mfc.sln).
3. Select **Release | x64**, then **Build → Build Solution**.
4. Run `AccessCsvExporter.Mfc/build/Release/AccessCsvExporter.Mfc.exe`.

A prebuilt x64 executable is available under **Artifacts → AccessCsvExporter-MFC-x64** in the latest successful [GitHub Actions build](https://github.com/pepelaz1/access-csv-exporter/actions/workflows/mfc-windows.yml).

## Requirements and behavior

- Windows with the 64-bit **Microsoft Access Driver (`*.mdb`, `*.accdb`)** installed. Microsoft Access itself is not required.
- The app lists user tables through ODBC and streams the selected table to a UTF-8 CSV file with a BOM for spreadsheet compatibility.
- Commas, quotes, and line breaks in values are escaped according to CSV rules. An interrupted export does not replace the destination file.
- Binary columns are not supported in this focused example.

## Checks

Run `AccessCsvExporter.Mfc/build-native-tests.cmd` to compile and run the C++ CSV and error-handling checks without MFC. GitHub Actions runs these checks and builds the desktop app. The executable has been launched on Windows; export against a real Access database still needs an end-to-end check.

This standalone example does not contain or modify a customer's program or data.
