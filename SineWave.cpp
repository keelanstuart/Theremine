#include "pch.h"

#include "SineWave.h"


CSineWave::CSineWave()
{
	m_fFreq = 1000.0;
	dK = 0;	  // radians to increment per sample
	dPos = 0; // current position in the unit circle
}


void CSineWave::Initialize(float Frequency, int SampleRate, float Amplitude)
{
	COscillator::Initialize(Frequency, SampleRate, Amplitude);

	SetFrequency(Frequency);
}


void CSineWave::SetFrequency(float Frequency)
{
	COscillator::SetFrequency(Frequency);

	dK = m_fFreq * M_PI / (double)m_iSampleRate;
}


int CSineWave::Generate(int16_t *data, int Samples)
{
	double dSinVal, dAmpVal;
	int i = 0;

    for (int j = 0; j < Samples; j++)
    {
		dPos += dK;
        dSinVal = cos(dPos);
        dAmpVal = (double)SHRT_MAX * dSinVal * m_fAmplitude;
        
		data[i++]  = (int16_t)dAmpVal;
    }

	if (dPos > (M_PI * 2.0))
	{
		int mod = (int)(dPos / (M_PI * 2.0));
		dPos -= (M_PI / 2.0) * mod;
	}

	return 0;
}



