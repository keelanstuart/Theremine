#pragma once

#include "oscillator.h"

class CSquareWave : public COscillator
{

public:
	CSquareWave();

	virtual void Initialize(float Frequency, int SampleRate, float Amplitude = 1.0f, float Distortion = 0.0f, float Reverb = 0.0f);
	virtual void SetFrequency(float Frequency);
	virtual void SetAmplitude(float Amplitude);
	virtual int Generate(int16_t *data, int Samples);

protected:
	int m_iSampleValue;

};

