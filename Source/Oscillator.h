#pragma once


class COscillator
{

public:
	COscillator();

	virtual void Initialize(float Frequency, int SampleRate, float Amplitude = 1.0f, float Distortion = 0.0f, float Reverb = 0.0f);
	virtual void SetFrequency(float Frequency);
	virtual void SetAmplitude(float Amplitude);
	virtual void SetDistortion(float Distortion);
	virtual void SetReverb(float Reverb);
	virtual int Generate(int16_t *data, int Samples);

protected:
	float m_fFreq;
	float m_fAmplitude;
	float m_fDistortion;
	float m_fReverb;

	int m_iSampleRate;

	double dK;
	double dPos;

};

