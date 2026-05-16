using System;
using System.Windows.Forms;
using static System.Net.Mime.MediaTypeNames;
using Application = System.Windows.Forms.Application;

namespace synth0815
{
    internal static class Program
    {
        [STAThread]
        static void Main ()
        {
            FileLogger.Init();
            ApplicationConfiguration.Initialize();

            using var tray = new SynthApp();
            Application.Run();
            FileLogger.Shutdown();
        }
    }
}