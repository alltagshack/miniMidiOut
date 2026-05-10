using NAudio.Midi;
using NAudio.Wave;
using NAudio.Wave.SampleProviders;

namespace synth0815
{
    class Program
    {
        static readonly int sampleRate = 22050;
        static readonly int channels = 1;

        static SynthMixer _mixer;
        static MidiIn _midiIn;
        static WaveOutEvent _output;
        static SignalGeneratorType _currentWaveform;
        static bool _sustainPedal;

        static void NoteOn (int noteNumber, int velocity)
        {
            lock (_mixer.Voices)
            {
                Voice v = new Voice(note: noteNumber, volume: velocity / 127.0f * 0.4f, _currentWaveform);
                _mixer.Voices.Add(v);
            }

            Console.WriteLine($"NoteOn {noteNumber}");
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
            Console.WriteLine($"NoteOff {noteNumber}");
        }

        static void Midi_MessageReceived (object sender, MidiInMessageEventArgs e)
        {
            switch (e.MidiEvent)
            {
                case NoteOnEvent noteOn when noteOn.Velocity > 0:
                    NoteOn(noteOn.NoteNumber, noteOn.Velocity);
                    break;

                case NoteEvent noteOff when noteOff.CommandCode == MidiCommandCode.NoteOff:
                    NoteOff(noteOff.NoteNumber);
                    break;

                // NoteOn mit Velocity = 0 wird im MIDI‑Standard als NoteOff betrachtet
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
                            Console.WriteLine("Sustain OFF");
                            
                            lock (_mixer.Voices)
                            {
                                _mixer.Voices.FindAll(v => v._pendingNoteOff).ForEach(v => {
                                    v._enabled = false;
                                    v._pendingNoteOff = false;
                                });
                            }

                        }
                        else
                        {
                            Console.WriteLine("Sustain ON");
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
                    Console.WriteLine($"midi event: {e.MidiEvent}");
                    break;
            }
        }

        static void Midi_ErrorReceived (object sender, MidiInMessageEventArgs e) =>
            Console.WriteLine($"MIDI‑Error: {e.MidiEvent}");



        static void Main (string[] args)
        {
            _currentWaveform = SignalGeneratorType.Square;
            _sustainPedal = false;

            _mixer = new SynthMixer(sampleRate, fade: 0.000001, channels);

            _output = new WaveOutEvent
            {
                DesiredLatency = 5,
                NumberOfBuffers = 30
            };

            _output.Init(_mixer);
            _output.Play();

            int devIdx = 0;
            for (int i = 0; i < MidiIn.NumberOfDevices; i++)
            {
                Console.WriteLine($"{i} = {MidiIn.DeviceInfo(i).ProductName}");
            }

            try
            {
                _midiIn = new MidiIn(devIdx);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                _output.Stop();
                _output.Dispose();
                return;
            }


            _midiIn.MessageReceived += Midi_MessageReceived;
            _midiIn.ErrorReceived += Midi_ErrorReceived;
            _midiIn.Start();

            Console.WriteLine(
                "    1 - Sinus\n" +
                "    2 - Sawtooth\n" +
                "    3 - Square\n" +
                "    4 - Triangle\n" +
                "  ESC - Quit program");

            bool running = true;
            while (running)
            {
                // wartet blockierend auf einen Tastendruck, ohne die Eingabe an die Konsole
                var keyInfo = Console.ReadKey(intercept: true);

                switch (keyInfo.Key)
                {
                    case ConsoleKey.D1:
                    case ConsoleKey.NumPad1:
                        _currentWaveform = SignalGeneratorType.Sin;
                        Console.WriteLine("Sinus");
                        break;

                    case ConsoleKey.D2:
                    case ConsoleKey.NumPad2:
                        _currentWaveform = SignalGeneratorType.SawTooth;
                        Console.WriteLine("Sawtooth");
                        break;

                    case ConsoleKey.D3:
                    case ConsoleKey.NumPad3:
                        _currentWaveform = SignalGeneratorType.Square;
                        Console.WriteLine("Square");
                        break;

                    case ConsoleKey.D4:
                    case ConsoleKey.NumPad4:
                        _currentWaveform = SignalGeneratorType.Triangle;
                        Console.WriteLine("Triangle");
                        break;

                    case ConsoleKey.Escape:
                        running = false;
                        break;

                    default:
                        break;
                }
            }

            _midiIn.Stop();
            _midiIn.Dispose();
            _output.Stop();
            _output.Dispose();
        }

    }

}
