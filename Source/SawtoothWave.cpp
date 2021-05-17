#include "pch.h"

#include "SawtoothWave.h"


CSawtoothWave::CSawtoothWave() : COscillator()
{
	dPos = 0;
	dK = 0;
}


void CSawtoothWave::Initialize(float Frequency, int SampleRate, float Amplitude, float Distortion, float Reverb)
{
	COscillator::Initialize(Frequency, SampleRate, Amplitude, Distortion, Reverb);

	SetFrequency(Frequency);
}


void CSawtoothWave::SetFrequency(float Frequency)
{
	COscillator::SetFrequency(Frequency);

	dK = m_fFreq * (M_PI * 2.0) / (double)m_iSampleRate;
}


int CSawtoothWave::Generate(int16_t *data, int Samples)
{
	double fSample, dDistVal;
	while (Samples--)
	{
		dPos += dK;
		fSample = dPos / (M_PI * 2.0);

		dDistVal = (double)((int16_t)(rand()) / 4) * m_fDistortion;
		fSample += dDistVal;

		*data++ = (int16_t)(fSample * m_fAmplitude * SHRT_MAX);

		if (dPos > (M_PI * 2.0))
			dPos -= (M_PI * 2.0);
	}

	return 0;
}


int CNegSawtoothWave::Generate(int16_t *data, int Samples)
{
	double fSample;
	while (Samples--)
	{
		dPos += dK;
		fSample = 1.0 - (dPos / (M_PI * 2.0));

		*data++ = (int16_t)(fSample * m_fAmplitude * SHRT_MAX);

		if (dPos > (M_PI * 2.0))
			dPos -= (M_PI * 2.0);
	}

	return 0;
}
