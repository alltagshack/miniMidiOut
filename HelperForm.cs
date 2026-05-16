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

    internal sealed class HelperForm : Form
    {
        public HelperForm ()
        {
            ShowInTaskbar = false;
            Opacity = 0;
            Width = Height = 0;
            UiInvoker.UiContext = SynchronizationContext.Current;
        }
    }
}
