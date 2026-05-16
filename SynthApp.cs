using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

using NAudio.Midi;
using NAudio.Wave;
using NAudio.Wave.SampleProviders;

namespace synth0815
{
    public sealed class SynthApp : IDisposable
    {


        private NotifyIcon _icon;
        private ContextMenuStrip _menu;
        private ToolStripMenuItem _devicesItem;
        private ToolStripMenuItem _channelItem;
        private ToolStripMenuItem _refreshItem;
        private ToolStripMenuItem _exitItem;

        private MidiIn? _selectedMidiIn;
        private int _selectedChannel = 0;
        private readonly HelperForm _invokeHelper;

        static Waveforms _waveforms = new Waveforms();

        static readonly int sampleRate = 22050;
        static readonly int audioChannels = 1;

        static SynthMixer _mixer;
        static WaveOutEvent _output;
        static bool _sustainPedal;
        static int _sustainCount;

        static void NoteOn (int noteNumber, int velocity)
        {
            lock (_mixer.Voices)
            {
                Voice v = new Voice(
                    noteNumber,
                    velocity / 127.0f * 0.4f,
                    _waveforms._current.SignalType
                );
                _mixer.Voices.Add(v);
            }
            _sustainCount = 0;
        }

        static void NoteOff (int noteNumber)
        {
            lock (_mixer.Voices)
            {
                _mixer.Voices.FindAll(v => v._note == noteNumber).ForEach(v => {
                    if (_sustainPedal)
                    {
                        v._pendingNoteOff = true;
                    }
                    else
                    {
                        v._enabled = false;
                    }
                });
            }
        }

        void Midi_MessageReceived (object sender, MidiInMessageEventArgs e)
        {
            if (_selectedChannel > 0 && e.MidiEvent.Channel != _selectedChannel)
            {
                FileLogger.WriteLine($"MIDI: {e.MidiEvent} channel {e.MidiEvent.Channel}");
                return;
            }

            switch (e.MidiEvent)
            {
                case NoteOnEvent noteOn when noteOn.Velocity > 0:
                    NoteOn(noteOn.NoteNumber, noteOn.Velocity);
                    break;

                case NoteEvent noteOff when noteOff.CommandCode == MidiCommandCode.NoteOff:
                    NoteOff(noteOff.NoteNumber);
                    break;

                case NoteOnEvent noteOnZero when noteOnZero.Velocity == 0:
                    NoteOff(noteOnZero.NoteNumber);
                    break;

                case ControlChangeEvent cc when cc.Controller == MidiController.Sustain:
                    bool newState = cc.ControllerValue >= 64;
                    if (newState != _sustainPedal)
                    {
                        _sustainPedal = newState;

                        if (_sustainPedal == false)
                        {                            
                            lock (_mixer.Voices)
                            {
                                _mixer.Voices.FindAll(v => v._pendingNoteOff).ForEach(v => {
                                    v._enabled = false;
                                    v._pendingNoteOff = false;
                                });
                            }

                        } else
                        {
                            if (_mixer.Voices.Count == 0)
                            {
                                _sustainCount++;
                                if (_sustainCount > 2)
                                {
                                    // on silence -> 3x sustain = cycles through waveforms
                                    UiInvoker.UiContext?.Post(_ =>
                                    {
                                        _waveforms.Next();
                                        _sustainCount = 0;
                                    }, null);
                                }
                            }
                            else
                            {
                                _sustainCount = 0;
                            }
                        }

                    }
                    break;
                case PitchWheelChangeEvent pc:

                    // relative -1..+1 (center -> 0)
                    double rel = (pc.Pitch - 8192) / (double)8192.0;

                    // semitone offset -4 .. +4
                    double semitones = rel * 4.0;

                    // Frequency factor and new frequency
                    float factor = (float)Math.Pow(2.0, semitones / 12.0);

                    lock (_mixer.Voices)
                    {
                        _mixer.Voices.ForEach(v => {
                            v._frequency = v._orgFrequency * factor;
                        });
                    }
                    break;
                default:
                    FileLogger.WriteLine($"midi event: {e.MidiEvent}");
                    break;
            }
        }

        void Midi_ErrorReceived (object sender, MidiInMessageEventArgs e) =>
            FileLogger.WriteLine($"MIDI‑Error: {e.MidiEvent} channel {e.MidiEvent.Channel}");



        public SynthApp ()
        {
            _sustainCount = 0;
            _sustainPedal = false;
            _mixer = new SynthMixer(sampleRate, fade: 0.000001, audioChannels);

            _output = new WaveOutEvent
            {
                DesiredLatency = 5,
                NumberOfBuffers = 30
            };
            _output.Init(_mixer);

            _invokeHelper = new HelperForm();
            _invokeHelper.CreateControl();

            Icon icon;
            try
            {
                icon = IconFactory.LoadIconFromPng("icon.png");
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    $"Icon:\n{ex.Message}",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning
                );
                icon = SystemIcons.Application;
            }

            _icon = new NotifyIcon
            {
                Icon = icon,
                Visible = true,
                Text = "synth0815"
            };

            _menu = new ContextMenuStrip();

            _devicesItem = new ToolStripMenuItem("Devices");
            _menu.Items.Add(_devicesItem);

            _channelItem = new ToolStripMenuItem("Channel ...");
            _channelItem.Click += (s, e) => ShowChannelDialog();
            _menu.Items.Add(_channelItem);

            _refreshItem = new ToolStripMenuItem("Refresh");
            _refreshItem.Click += (s, e) => RefreshDevices();
            _menu.Items.Add(_refreshItem);

            _menu.Items.Add(new ToolStripSeparator());

            _waveforms.Add("~~ Sinus", SignalGeneratorType.Sin);
            _waveforms.Add("/// SawTooth", SignalGeneratorType.SawTooth);
            _waveforms.Add("|_| Square", SignalGeneratorType.Square);
            _waveforms.Add("/\\/ Triangle", SignalGeneratorType.Triangle);

            _waveforms.ForEach(wf => {
                _menu.Items.Add(wf.Item);
            });
            _menu.Items.Add(new ToolStripSeparator());

            _exitItem = new ToolStripMenuItem("Exit");
            _exitItem.Click += (s, e) => Application.Exit();
            _menu.Items.Add(_exitItem);

            _icon.ContextMenuStrip = _menu;
            _icon.MouseUp += TrayIcon_MouseUp;

            BuildDeviceMenu();

            // default
            _waveforms.ChangeTo("/// SawTooth");
        }

        private void TrayIcon_MouseUp (object? sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                if (_menu.Visible)
                {
                    _menu.Hide();
                } else {
                    _menu.Show(Cursor.Position);
                }
            }
        }

        private void RefreshDevices ()
        {
            if (_invokeHelper.IsHandleCreated)
                _invokeHelper.BeginInvoke(new MethodInvoker(BuildDeviceMenu));
            else
                BuildDeviceMenu();
        }

        private void BuildDeviceMenu ()
        {
            while (_devicesItem.DropDownItems.Count > 0)
                _devicesItem.DropDownItems.Clear();

            var deviceCount = MidiIn.NumberOfDevices;
            if (deviceCount == 0)
            {
                var none = new ToolStripMenuItem("(no devices)") { Enabled = false };
                _devicesItem.DropDownItems.Add(none);
                DisposeCurrentMidiIn();
                return;
            }

            for (int i = 0; i < deviceCount; i++)
            {
                var caps = MidiIn.DeviceInfo(i);
                var item = new ToolStripMenuItem($"{caps.ProductName} (#{i})")
                {
                    Tag = i,
                    Checked = false,
                    CheckOnClick = true
                };
                item.Click += DeviceItem_Click;
                _devicesItem.DropDownItems.Add(item);
            }

            if (_devicesItem.DropDownItems.Count == 1)
            {
                var single = (ToolStripMenuItem)_devicesItem.DropDownItems[0];
                single.PerformClick();
            }
        }

        private void DeviceItem_Click (object? sender, EventArgs e)
        {
            if (sender is not ToolStripMenuItem clicked) return;

            foreach (ToolStripMenuItem itm in _devicesItem.DropDownItems)
                itm.Checked = false;

            clicked.Checked = true;
            var deviceIndex = (int)clicked.Tag!;
            OpenMidiIn(deviceIndex);
        }



        private void OpenMidiIn (int deviceIndex)
        {
            DisposeCurrentMidiIn();

            try
            {
                _selectedMidiIn = new MidiIn(deviceIndex);
                _selectedMidiIn.MessageReceived += Midi_MessageReceived;
                _selectedMidiIn.ErrorReceived += Midi_ErrorReceived;
                _selectedMidiIn.Start();
                _output.Play();
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    $"Unable to open midi device: {ex.Message}",
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
        }

        private void DisposeCurrentMidiIn ()
        {
            if (_selectedMidiIn != null)
            {
                _selectedMidiIn.Stop();
                _selectedMidiIn.Dispose();
                _selectedMidiIn = null;
            }
        }

        public void Dispose ()
        {
            DisposeCurrentMidiIn();
            _output.Stop();
            _output.Dispose();
            _icon.Dispose();
            _menu.Dispose();
            _invokeHelper.Dispose();
        }

        private void ShowChannelDialog ()
        {
            using var dlg = new Form
            {
                FormBorderStyle = FormBorderStyle.FixedDialog,
                StartPosition = FormStartPosition.CenterScreen,
                Width = 250,
                Height = 120,
                Text = "select MIDI channel",
                MinimizeBox = false,
                MaximizeBox = false,
                ShowInTaskbar = false,
            };

            var num = new NumericUpDown
            {
                Minimum = 0,
                Maximum = 16,
                Value = _selectedChannel,
                Dock = DockStyle.Top,
            };
            var btnOk = new Button { Text = "OK", DialogResult = DialogResult.OK, Dock = DockStyle.Bottom };
            var btnCancel = new Button { Text = "Cancel", DialogResult = DialogResult.Cancel, Dock = DockStyle.Bottom };

            dlg.Controls.Add(num);
            dlg.Controls.Add(btnOk);
            dlg.Controls.Add(btnCancel);
            dlg.AcceptButton = btnOk;
            dlg.CancelButton = btnCancel;

            if (dlg.ShowDialog() == DialogResult.OK)
            {
                _selectedChannel = (int)num.Value;
            }
        }
    }

}
