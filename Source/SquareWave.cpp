#include "pch.h"

#include "SquareWave.h"


CSquareWave::CSquareWave() : COscillator()
{
}


void CSquareWave::Initialize(float Frequency, int SampleRate, float Amplitude, float Distortion, float Reverb)
{
	COscillator::Initialize(Frequency, SampleRate, Amplitude, Distortion, Reverb);
	m_iSampleValue = int(Amplitude * (double)SHRT_MAX);

	SetFrequency(Frequency);
}


void CSquareWave::SetFrequency(float Frequency)
{
	COscillator::SetFrequency(Frequency);

	dK = m_fFreq * (M_PI * 2.0) / (double)m_iSampleRate;
}


void CSquareWave::SetAmplitude(float Amplitude)
{
	COscillator::SetAmplitude(Amplitude);

	m_iSampleValue = int(Amplitude * (double)SHRT_MAX);
}


int CSquareWave::Generate(int16_t *data, int Samples)
{
	double dDistVal;
	while (Samples--)
	{
		dPos += dK;

		dDistVal = (double)((int16_t)(rand()) / 4) * m_fDistortion;
		dPos += dDistVal;

		if (dPos <= (M_PI * 2.0))
			*data++ = (int16_t)((float)m_iSampleValue);
		else
			*data++ = (int16_t)((float)-m_iSampleValue);

		if (dPos > (M_PI * 2.0))
			dPos -= (M_PI * 2.0);
	}

	return 0;
}
