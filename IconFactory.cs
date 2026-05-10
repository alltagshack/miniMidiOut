using System;
using System.Drawing;
using System.IO;
using System.Windows.Forms;

namespace synth0815
{
    internal static class IconFactory
    {
        public static Icon LoadIconFromPng (string pngFileName)
        {
            string exeFolder = AppDomain.CurrentDomain.BaseDirectory;
            string pngPath = Path.Combine(exeFolder, pngFileName);

            if (!File.Exists(pngPath))
                throw new FileNotFoundException($"PNG‑Icon nicht gefunden: {pngPath}");

            using var bmp = new Bitmap(pngPath);

            const int iconSize = 32;
            using var scaled = new Bitmap(bmp, new Size(iconSize, iconSize));

            IntPtr hIcon = scaled.GetHicon();
            return Icon.FromHandle(hIcon);
        }
    }
}
