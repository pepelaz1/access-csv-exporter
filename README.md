# Access CSV Exporter

A small C#/.NET 8 Windows console example that reads one table from a Microsoft Access `.accdb` or `.mdb` database and exports it to UTF-8 CSV. It validates that the table exists, streams rows instead of loading the whole database into memory, handles commas/quotes/newlines, and writes through a temporary file so an interrupted export does not replace an existing CSV.

## Requirements

- Windows and .NET 8 SDK.
- Microsoft Access Database Engine (ACE) installed with the same architecture as the running `dotnet` process. The program uses `Microsoft.ACE.OLEDB.12.0`. Microsoft 365 Access Runtime can provide the driver. Access itself is not required.

## Run

```powershell
dotnet run --project AccessCsvExporter -- "C:\data\orders.accdb" "Orders" "C:\data\orders.csv"
```

The output includes a header row and uses commas as the delimiter. Text is CSV-escaped; dates use ISO 8601, numbers use the invariant culture, binary fields use Base64, and nulls become empty fields. The CSV is UTF-8 with a BOM for Excel compatibility.

If the provider is missing or its 32/64-bit architecture does not match the process, `connection.Open()` fails. Install a matching ACE runtime, or run the matching x86/x64 .NET SDK. No database file is included in this repository because client data must not be published.

## Check CSV formatting

```powershell
dotnet run --project AccessCsvExporter.Tests
```

This smoke test uses an in-memory table to verify quoting, multiline fields, decimal formatting, and null handling. A real Access export additionally needs an `.accdb` or `.mdb` file and the ACE provider installed on Windows.

This is a focused sample of the data-export portion of a larger desktop application. It does not claim to rebuild or modify a particular customer's MFC program.
