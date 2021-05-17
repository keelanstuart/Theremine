#include "pch.h"

#include "SineWave.h"


CSineWave::CSineWave() : COscillator()
{
	dK = 0;	  // radians to increment per sample
	dPos = 0; // current position in the unit circle
}


void CSineWave::Initialize(float Frequency, int SampleRate, float Amplitude, float Distortion, float Reverb)
{
	COscillator::Initialize(Frequency, SampleRate, Amplitude, Distortion, Reverb);

	SetFrequency(Frequency);
}


void CSineWave::SetFrequency(float Frequency)
{
	COscillator::SetFrequency(Frequency);

	dK = m_fFreq * M_PI / (double)m_iSampleRate;
}


int CSineWave::Generate(int16_t *data, int Samples)
{
	double dSinVal, dAmpVal, dDistVal;
	int i = 0;

    for (int j = 0; j < Samples; j++)
    {
		dPos += dK;

		if (dPos > (M_PI * 2.0))
		{
			int mod = (int)(dPos / (M_PI * 2.0));
			dPos -= (M_PI / 2.0) * mod;
		}

		dSinVal = cos(dPos);
        dAmpVal = (double)SHRT_MAX * dSinVal * m_fAmplitude;

		dDistVal = (double)((int16_t)(rand()) / 4) * m_fDistortion;
		dAmpVal += dDistVal;

		data[i++]  = std::min<int16_t>(std::max<int16_t>(0, (int16_t)dAmpVal), SHRT_MAX);
    }

	return 0;
}



