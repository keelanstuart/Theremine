#include "pch.h"
#include "Oscillator.h"


COscillator::COscillator()
{
	dK = 0;	  // radians to increment per sample
	dPos = 0; // current position in the unit circle
}


void COscillator::Initialize(float Frequency, int SampleRate, float Amplitude) 
{
	m_fFreq = Frequency;
	m_iSampleRate = SampleRate;
	m_fAmplitude = Amplitude;
	return;
}


void COscillator::SetFrequency(float Frequency)
{
	m_fFreq = Frequency;
}


void COscillator::SetAmplitude(float Amplitude)
{
	m_fAmplitude = Amplitude;
}


int COscillator::Generate(int16_t *data, int Samples) 
{
	return 0;
}




