using NAudio.Wave.SampleProviders;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using static synth0815.Waveforms;

namespace synth0815
{
    public class Waveforms
    {
        private List<Waveform> _list;
        public Waveform _current;

        public struct Waveform
        {
            public SignalGeneratorType SignalType;
            public ToolStripMenuItem Item;
        }

        public Waveforms ()
        {
            _list = new List<Waveform>();
        }

        public void Add (string name, SignalGeneratorType sigType)
        {
            var item = new ToolStripMenuItem(name)
            {
                Checked = false,
                CheckOnClick = true
            };
            item.Click += (s, e) => ChangeTo(name);

            var wf = new Waveform()
            {
                SignalType = sigType,
                Item = item
            };

            _list.Add(wf);
        }

        public void ChangeTo (string name)
        {
            foreach (var wf in _list)
            {
                if (name == wf.Item.Text)
                {
                    wf.Item.Checked = true;
                    _current = wf;
                }
                else
                {
                    wf.Item.Checked = false;
                }
            }
        }

        public void ForEach (Action<Waveform> action)
        {
            if (action == null) throw new ArgumentNullException(nameof(action));
            foreach (var item in _list) action(item);
        }

        public Waveform Next ()
        {
            if (_list.Count < 2) return _current;

            int index = -1;
            for (int i =0; i < _list.Count; i++)
            {
                if (_current.Item.Text == _list[i].Item.Text)
                {
                    index = i;
                }
            }

            if (index == -1) return _current;

            if (index == _list.Count - 1)
            {
                index = 0;
            } else {
                index++;
            }

            ChangeTo(_list[index].Item.Text);
            return _current;
        }
    }

}
