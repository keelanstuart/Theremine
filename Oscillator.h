#pragma once


class COscillator
{

public:
	COscillator();

	virtual void Initialize(float Frequency, int SampleRate, float Amplitude = 1.0f);
	virtual void SetFrequency(float Frequency);
	virtual void SetAmplitude(float Amplitude);
	virtual int Generate(int16_t *data, int Samples);

protected:
	float m_fFreq;
	float m_fAmplitude;

	int m_iSampleRate;

	double dK;
	double dPos;

};

