#pragma once

#include "oscillator.h"

class CSawtoothWave : public COscillator
{

public:
	CSawtoothWave();

	virtual void Initialize(float Frequency, int SampleRate, float Amplitude = 1.0);
	virtual void SetFrequency(float Frequency);
	virtual int Generate(int16_t *data, int Samples);

};

class CNegSawtoothWave : public CSawtoothWave
{

public:
	virtual int Generate(int16_t *data, int Samples);

};

