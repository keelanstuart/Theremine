#include "pch.h"

#include "TriangleWave.h"


CTriangleWave::CTriangleWave() : COscillator()
{
	dPos = 0;
	dK = 0;
}


void CTriangleWave::Initialize(float Frequency, int SampleRate, float Amplitude, float Distortion, float Reverb)
{
	COscillator::Initialize(Frequency, SampleRate, Amplitude, Distortion, Reverb);

	SetFrequency(Frequency);
}


void CTriangleWave::SetFrequency(float Frequency)
{
	COscillator::SetFrequency(Frequency);

	dK = m_fFreq * (M_PI * 2.0) / (double)m_iSampleRate;
}


int CTriangleWave::Generate(int16_t *data, int Samples)
{
	double fSample, dDistVal;
	while (Samples--)
	{
		dPos += dK;
		fSample = dPos / (M_PI * 2.0);
		if (fSample > 1.0)
			fSample = 2.0 - fSample;

		dDistVal = (double)((int16_t)(rand()) / 4) * m_fDistortion;
		fSample += dDistVal;

		*data++ = (int16_t)(fSample * m_fAmplitude * (double)SHRT_MAX);

		if (dPos > (M_PI * 2.0))
			dPos -= (M_PI * 2.0);
	}

	return 0;
}
