// **************************************************************
// Theremine Source File
// An optical theremin for the Leap Motion controller series of devices
//
// Copyright © 2020-2025, Keelan Stuart

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#include "miniaudio.h"

#include <mutex>

template <typename T> class CSyncVar
{
public:
	CSyncVar()
	{
	}

	CSyncVar(const T init_val)
	{
		m_Val = init_val;
	}

	virtual ~CSyncVar()
	{
	}

	T &Get()
	{
		std::lock_guard lock(m_mutex);
		return m_Val;
	}

	void Set(T val)
	{
		std::lock_guard lock(m_mutex);
		m_Val = val;
	}

	T *Lock()
	{
		m_mutex.lock();
		return &m_Val;
	}

	void Unlock()
	{
		m_mutex.unlock();
	}

protected:
	T m_Val;
	std::mutex m_mutex;
};

template <typename T> class CBoundedSyncVar : public CSyncVar<T>
{
public:
	CBoundedSyncVar() : CSyncVar<T>()
	{
	}

	CBoundedSyncVar(const T &init_val, const T &init_min, const T &init_max) : CSyncVar<T>(init_val)
	{
		m_MinVal = init_min;
		m_MaxVal = init_max;
	}

	virtual ~CBoundedSyncVar() { }

	T &GetMin()
	{
		std::lock_guard lock(m_mutex);
		return m_MinVal;
	}

	void SetMin(T &minval)
	{
		std::lock_guard lock(m_mutex);
		m_MinVal = minval;
	}

	T &GetMax()
	{
		std::lock_guard lock(m_mutex);
		return m_MinVax;
	}

	void SetMax(T &maxval)
	{
		std::lock_guard lock(m_mutex);
		m_MaxVal = maxval;
	}

protected:
	T m_MinVal, m_MaxVal;
};

// CTheremineApp:
// See Theremine.cpp for the implementation of this class
//

class CTheremineApp : public CWinApp
{

public:

	props::IPropertySet *m_pProps;

	LEAP_CONNECTION m_LeapConn;

	typedef struct sLeapDevData
	{
		sLeapDevData()
		{
			serial.resize(44);
		};

		sLeapDevData(const struct sLeapDevData &dd)
		{
			dev = dd.dev;
			info = dd.info;
			serial = dd.serial;
		};

		LEAP_DEVICE dev;
		LEAP_DEVICE_INFO info;
		std::vector<char> serial;
	} SLeapDevData;

	typedef std::map<LEAP_DEVICE_REF, SLeapDevData> TLeapDevMap;
	TLeapDevMap m_LeapDev;

	pool::IThreadPool *m_pLeapPool;

	int m_iSampleRate;
	CSyncVar<bool> m_Enabled;

	props::IProperty *prop_osctype;
	props::IProperty *prop_freqmin;
	props::IProperty *prop_freqmax;
	props::IProperty *prop_freqsteps;
	props::IProperty *prop_intmode;
	props::IProperty *prop_sustain;

	using INPUT_TYPE = enum
	{
		R_PALM_NPOS = 0,			// Normalized Position of the Right Palm
		R_PALM_DDOWN,				// Dot product of [DOWN] and the Right Palm normal
		R_FIST_TIGHTNESS,			// Tightness of the right fist
		R_PINCH_TIGHTNESS,			// Tightness of the right index finger and thumb
		R_FINGER_THUMB_ANG,
		R_FINGER_INDEX_ANG,
		R_FINGER_MIDDLE_ANG,
		R_FINGER_RING_ANG,
		R_FINGER_PINKY_ANG,

		L_PALM_NPOS,				// Normalized Position of the Left Palm
		L_PALM_DDOWN,				// Dot product of [DOWN] and the Left Palm normal
		L_FIST_TIGHTNESS,			// Tightness of the left fist
		L_PINCH_TIGHTNESS,			// Tightness of the left index finger and thumb
		L_FINGER_THUMB_ANG,
		L_FINGER_INDEX_ANG,
		L_FINGER_MIDDLE_ANG,
		L_FINGER_RING_ANG,
		L_FINGER_PINKY_ANG,

		NUM_INPUTS
	};

	using FINGER_INDEX = enum
	{
		THUMB = 0,
		INDEX,
		MIDDLE,
		RING,
		PINKY
	};

	const TCHAR *GetInputName(INPUT_TYPE it);

	using EFFECT_TYPE = enum
	{
		FREQUENCY = 0,
		VOLUME,
		DISTORTION,
		REVERB,
		PITCH_BEND,

		NUM_EFFECTS
	};

	const TCHAR *GetEffectName(EFFECT_TYPE et);

	// Control Values for HAND_TARGET items, all in the range of [0..1]
	CSyncVar<float> m_ControlValue[INPUT_TYPE::NUM_INPUTS];

	typedef std::map<EFFECT_TYPE, INPUT_TYPE> TEffectInputMap;
	TEffectInputMap m_EffectMap;

	void AssociateInputWithEffect(EFFECT_TYPE eff, INPUT_TYPE inp);

	ma_device_config m_DeviceConfig;
	ma_device m_Device;

	ma_waveform_config m_WaveConfig;
	ma_waveform m_Wave;

	//ma_decoder m_Decoder;

public:
	CTheremineApp();
	virtual ~CTheremineApp();

	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};

extern CTheremineApp theApp;
