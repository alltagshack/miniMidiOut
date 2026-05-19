using NAudio.Wave.SampleProviders;

namespace synth0815
{
    public class Voice
    {
        private readonly double TWO_PI = 2.0 * Math.PI;

        public int _note;
        public float _orgFrequency;
        public float _frequency;
        public double _volume;      // 0..1
        public double _phase;      // internal, in radians
        public bool _enabled;
        public bool _pendingNoteOff;

        public float _modFactor;
        public uint _modFrequency;
        private double _mod;
        private bool _modulationUp;

        private SignalGeneratorType _waveform;

        public Voice (
            int note,
            float volume,
            SignalGeneratorType waveform = SignalGeneratorType.Sin,
            float modFactor = 0.00f,
            uint modFrequency = 5)
        {
            this._waveform = waveform;
            this._note = note;
            this._phase = 0.0;
            this._volume = volume;
            this._enabled = true;
            this._pendingNoteOff = false;

            this._modFactor = modFactor;
            this._modFrequency = modFrequency;
            this._mod = 0.0;
            this._modulationUp = true;

            this._frequency = MidiNoteToFrequency(note);
            this._orgFrequency = this._frequency;
        }

        private double Increment (int sampleRate)
        {
            if (_modFrequency == 0 || _modFactor < 0.0001)
            {
                return TWO_PI* _frequency / sampleRate;
            }

            double val = TWO_PI * (_frequency * (1.0 + _mod)) / sampleRate;

            if (_modulationUp)
            {
                _mod += 0.5 * _modFrequency / sampleRate;
            }
            else
            {
                _mod -= 0.5 * _modFrequency / sampleRate;
            }


            if (_mod < (-1.0 * _modFactor)) _modulationUp = true;
            if (_mod > _modFactor) _modulationUp = false;

            return val;
        }

        public float NextSample (int sampleRate)
        {
            float sample = 0f;
            double phaseNorm = _phase / TWO_PI; // 0..1

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
            if (_phase >= TWO_PI) _phase -= TWO_PI;

            return sample;
        }

        static public float MidiNoteToFrequency (int note)
        {
            return 440.0f * (float)Math.Pow(2.0, (note - 69) / 12.0);
        }


    }
}
