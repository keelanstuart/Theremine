#include "pch.h"

#include "TriangleWave.h"


CTriangleWave::CTriangleWave()
{
	m_fFreq = 1000.0;
	dPos = 0;
	dK = 0;
}


void CTriangleWave::Initialize(float Frequency, int SampleRate, float Amplitude)
{
	COscillator::Initialize(Frequency, SampleRate, Amplitude);

	SetFrequency(Frequency);
}


void CTriangleWave::SetFrequency(float Frequency)
{
	COscillator::SetFrequency(Frequency);

	dK = m_fFreq * (M_PI * 2.0) / (double)m_iSampleRate;
}


int CTriangleWave::Generate(int16_t *data, int Samples)
{
	double fSample;
	while (Samples--)
	{

		dPos += dK;
		fSample = dPos / (M_PI * 2.0);
		if (fSample > 1.0)
			fSample = 2.0 - fSample;

		*data++ = (int16_t)(fSample * m_fAmplitude * (double)SHRT_MAX);

		if (dPos > (M_PI * 2.0))
			dPos -= (M_PI * 2.0);
	}

	return 0;
}
