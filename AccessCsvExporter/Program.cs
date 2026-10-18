using System.Data.OleDb;

namespace AccessCsvExporter;

public static class Program
{
    public static int Main(string[] args)
    {
        if (args.Length != 3 || args[0] is "--help" or "-h")
        {
            Console.Error.WriteLine("Usage: AccessCsvExporter <database.accdb|database.mdb> <table> <output.csv>");
            return 2;
        }

        var databasePath = Path.GetFullPath(args[0]);
        var tableName = args[1];
        var outputPath = Path.GetFullPath(args[2]);

        if (!File.Exists(databasePath))
        {
            Console.Error.WriteLine($"Database not found: {databasePath}");
            return 2;
        }

        if (tableName.Length == 0 || tableName.Contains('[') || tableName.Contains(']'))
        {
            Console.Error.WriteLine("Table name is empty or contains unsupported bracket characters.");
            return 2;
        }

        if (string.Equals(databasePath, outputPath, StringComparison.OrdinalIgnoreCase))
        {
            Console.Error.WriteLine("Output path must differ from the database path.");
            return 2;
        }

        try
        {
            var connectionString = new OleDbConnectionStringBuilder
            {
                Provider = "Microsoft.ACE.OLEDB.12.0",
                DataSource = databasePath
            }.ConnectionString;

            using var connection = new OleDbConnection(connectionString);
            connection.Open();

            var tables = connection.GetSchema("Tables");
            var exists = tables.Rows.Cast<System.Data.DataRow>().Any(row =>
                string.Equals(row["TABLE_NAME"]?.ToString(), tableName, StringComparison.OrdinalIgnoreCase)
                && string.Equals(row["TABLE_TYPE"]?.ToString(), "TABLE", StringComparison.OrdinalIgnoreCase));
            if (!exists)
            {
                Console.Error.WriteLine($"Table not found: {tableName}");
                return 2;
            }

            using var command = connection.CreateCommand();
            command.CommandText = $"SELECT * FROM [{tableName}]";
            using var reader = command.ExecuteReader();
            if (reader is null)
            {
                Console.Error.WriteLine("The database returned no result set.");
                return 1;
            }

            // Write beside the target, then replace it only after a complete export.
            var outputDirectory = Path.GetDirectoryName(outputPath)!;
            Directory.CreateDirectory(outputDirectory);
            var temporaryPath = Path.Combine(outputDirectory, $".{Path.GetFileName(outputPath)}.{Guid.NewGuid():N}.tmp");
            try
            {
                using (var stream = new FileStream(temporaryPath, FileMode.CreateNew, FileAccess.Write))
                using (var writer = new StreamWriter(stream, new System.Text.UTF8Encoding(encoderShouldEmitUTF8Identifier: true)))
                {
                    CsvWriter.Write(reader, writer);
                }

                File.Move(temporaryPath, outputPath, overwrite: true);
            }
            finally
            {
                if (File.Exists(temporaryPath)) File.Delete(temporaryPath);
            }

            Console.WriteLine($"Exported [{tableName}] to {outputPath}");
            return 0;
        }
        catch (OleDbException ex)
        {
            Console.Error.WriteLine($"Access error: {ex.Message}");
            return 1;
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidOperationException)
        {
            Console.Error.WriteLine($"Export failed: {ex.Message}");
            return 1;
        }
    }
}
