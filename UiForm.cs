using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace synth0815
{
    static class UiInvoker
    {
        public static SynchronizationContext UiContext { get; set; }
    }

    internal sealed class UiForm : Form
    {

        public UiForm ()
        {
            ShowInTaskbar = true;
            Opacity = 1.0;
            Width = Height = 300;

            UiInvoker.UiContext = SynchronizationContext.Current;
        }
    }
}
