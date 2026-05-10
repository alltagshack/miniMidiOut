using NAudio.Wave;

namespace synth0815
{
    internal class SynthMixer : ISampleProvider
    {
        public double _fade;
        public List<Voice> Voices;
        private readonly int _channels;
        public WaveFormat WaveFormat { get; }

        public SynthMixer (int sampleRate, double fade, int channels = 1)
        {
            this._fade = fade;
            this._channels = channels;
            WaveFormat = WaveFormat.CreateIeeeFloatWaveFormat(sampleRate, channels);
            Voices = new List<Voice>();
        }

        public int Read (float[] buffer, int offset, int count)
        {
            int samplesPerChannel = count / _channels;
            for (int n = 0; n < samplesPerChannel; n++)
            {
                float mixedSample = 0f;

                lock (Voices)
                {
                    for (int i = Voices.Count - 1; i >= 0; i--)
                    {
                        var v = Voices[i];
                        if (v._enabled == true && v._volume > 0.00001)
                        {
                            v._volume -= _fade;
                            mixedSample += v.NextSample(WaveFormat.SampleRate);
                        }
                        else
                        {
                            Voices.RemoveAt(i);
                        }
                    }
                }

                if (mixedSample > 1f) mixedSample = 1f;
                else if (mixedSample < -1f) mixedSample = -1f;

                // write to channels
                for (int ch = 0; ch < _channels; ch++)
                {
                    buffer[offset + n * _channels + ch] = mixedSample;
                }
            }

            return count;
        }
    }
}
