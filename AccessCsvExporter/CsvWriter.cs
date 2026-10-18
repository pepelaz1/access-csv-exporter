using System.Data;
using System.Globalization;

namespace AccessCsvExporter;

public static class CsvWriter
{
    public static void Write(IDataReader reader, TextWriter writer)
    {
        var headers = Enumerable.Range(0, reader.FieldCount).Select(reader.GetName);
        WriteRecord(writer, headers);

        while (reader.Read())
        {
            var values = Enumerable.Range(0, reader.FieldCount)
                .Select(index => FormatValue(reader.GetValue(index)));
            WriteRecord(writer, values);
        }
    }

    private static string FormatValue(object? value) => value switch
    {
        null or DBNull => string.Empty,
        DateTime date => date.ToString("O", CultureInfo.InvariantCulture),
        byte[] bytes => Convert.ToBase64String(bytes),
        IFormattable formattable => formattable.ToString(null, CultureInfo.InvariantCulture) ?? string.Empty,
        _ => value.ToString() ?? string.Empty
    };

    private static void WriteRecord(TextWriter writer, IEnumerable<string> fields)
    {
        writer.WriteLine(string.Join(',', fields.Select(Escape)));
    }

    private static string Escape(string field)
    {
        if (!field.Contains(',') && !field.Contains('"') && !field.Contains('\r') && !field.Contains('\n'))
            return field;

        return $"\"{field.Replace("\"", "\"\"")}\"";
    }
}
