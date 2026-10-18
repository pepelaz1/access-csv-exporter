using System.Data;
using AccessCsvExporter;

var table = new DataTable();
table.Columns.Add("Id", typeof(int));
table.Columns.Add("Description", typeof(string));
table.Columns.Add("Price", typeof(decimal));
table.Columns.Add("Note", typeof(string));
table.Rows.Add(1, "Bolt, 10 mm", 12.50m, "Two lines\nwith \"quotes\"");
table.Rows.Add(2, DBNull.Value, 0.75m, "plain");

using var reader = table.CreateDataReader();
using var writer = new StringWriter();
CsvWriter.Write(reader, writer);

var expected = string.Join(Environment.NewLine,
    "Id,Description,Price,Note",
    "1,\"Bolt, 10 mm\",12.50,\"Two lines\nwith \"\"quotes\"\"\"",
    "2,,0.75,plain",
    "");

if (writer.ToString() != expected)
    throw new Exception($"Unexpected CSV output:\n{writer}");

Console.WriteLine("CSV smoke test passed.");
