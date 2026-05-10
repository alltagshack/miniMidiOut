using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace synth0815
{
    public static class FileLogger
    {
        private static readonly object _lock = new();
        private static string? _logDirectory;
        private static string? _currentLogFile;
        private static StreamWriter? _writer;

        public static void Init (string relativePath = "Logs")
        {
            var exeFolder = AppDomain.CurrentDomain.BaseDirectory;
            _logDirectory = Path.Combine(exeFolder, relativePath);
            Directory.CreateDirectory(_logDirectory);

            UpdateLogFileName(force: true);
        }

        public static void WriteLine (string format, params object?[] args)
            => WriteLine(string.Format(format, args));

        public static void WriteLine (string message)
        {
            try
            {
                UpdateLogFileName();

                var line = $"{DateTime.Now:yyyy-MM-dd HH:mm:ss.fff} | {message}";
                lock (_lock)
                {
                    _writer?.WriteLine(line);
                }
            }
            catch (Exception ex)
            {
                try
                {
                    System.Diagnostics.EventLog.WriteEntry(
                        "Application",
                        $"FileLogger: {ex}",
                        System.Diagnostics.EventLogEntryType.Error);
                }
                catch { }
            }
        }

        public static void Shutdown ()
        {
            lock (_lock)
            {
                _writer?.Flush();
                _writer?.Dispose();
                _writer = null;
            }
        }

        private static void UpdateLogFileName (bool force = false)
        {
            if (_logDirectory == null) throw new InvalidOperationException("FileLogger not initialized.");

            var today = DateTime.Today.ToString("yyyyMMdd");
            var candidate = Path.Combine(_logDirectory, $"MidiTray_{today}.log");

            if (force || !string.Equals(_currentLogFile, candidate, StringComparison.OrdinalIgnoreCase))
            {
                lock (_lock)
                {
                    _writer?.Flush();
                    _writer?.Dispose();

                    _writer = new StreamWriter(new FileStream(candidate,
                                         FileMode.Append,
                                         FileAccess.Write,
                                         FileShare.Read), Encoding.UTF8)
                    {
                        AutoFlush = true
                    };
                    _currentLogFile = candidate;
                }
            }
        }
    }
}
