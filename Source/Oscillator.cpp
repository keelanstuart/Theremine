#include "pch.h"
#include "Oscillator.h"


COscillator::COscillator()
{
	dK = 0;	  // radians to increment per sample
	dPos = 0; // current position in the unit circle
}


void COscillator::Initialize(float Frequency, int SampleRate, float Amplitude, float Distortion, float Reverb) 
{
	m_fFreq = Frequency;
	m_iSampleRate = SampleRate;
	m_fAmplitude = Amplitude;
	m_fDistortion = Distortion;
	m_fReverb = Reverb;

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


void COscillator::SetDistortion(float Distortion)
{
	m_fDistortion = Distortion;
}


void COscillator::SetReverb(float Reverb)
{
	m_fReverb = Reverb;
}


int COscillator::Generate(int16_t *data, int Samples) 
{
	return 0;
}




