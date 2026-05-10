using NAudio.Wave.SampleProviders;

namespace synth0815
{
    public class Voice
    {
        private readonly double twoPi = 2.0 * Math.PI;

        public int _note;
        public float _orgFrequency;
        public float _frequency;
        public double _volume;      // 0..1
        public double _phase;      // internal, in radians
        public bool _enabled;
        public bool _pendingNoteOff;

        private SignalGeneratorType _waveform;

        public Voice (int note, float volume, SignalGeneratorType waveform = SignalGeneratorType.Sin)
        {
            this._waveform = waveform;
            this._note = note;
            this._phase = 0.0;
            this._volume = volume;
            this._enabled = true;
            this._pendingNoteOff = false;

            this._frequency = MidiNoteToFrequency(note);
            this._orgFrequency = this._frequency;
        }

        private double Increment (int sampleRate)
        {
            return twoPi * _frequency / sampleRate;
        }

        public float NextSample (int sampleRate)
        {
            float sample = 0f;
            double phaseNorm = _phase / twoPi; // 0..1

            switch (_waveform)
            {
                case SignalGeneratorType.SawTooth:
                    // Saw: -1..1 over phase 0..2pi. Using normalized phase: (2*phaseNorm - 1)
                    sample = (float)((2.0 * phaseNorm - 1.0) * _volume);
                    break;

                case SignalGeneratorType.Square:
                    // Square: +1 for first half, -1 for second half
                    sample = (float)((phaseNorm < 0.5) ? _volume : -_volume);
                    break;

                case SignalGeneratorType.Triangle:
                    // Triangle: rises from -1 to +1 in first half, falls back in second half
                    // formula using normalized phase: 4*abs(phaseNorm - 0.5) - 1 gives -1..1 inverted,
                    // so we invert sign to get -1..1 triangular wave
                    sample = (float)((1.0 - 4.0 * Math.Abs(phaseNorm - 0.5)) * _volume);
                    break;

                case SignalGeneratorType.Sin:
                default:
                    sample = (float)(Math.Sin(_phase) * _volume);
                    break;
            }


            // advance phase, keep within 0..2pi
            _phase += Increment(sampleRate);
            if (_phase >= twoPi) _phase -= twoPi;

            return sample;
        }

        static public float MidiNoteToFrequency (int note)
        {
            return 440.0f * (float)Math.Pow(2.0, (note - 69) / 12.0);
        }


    }
}
