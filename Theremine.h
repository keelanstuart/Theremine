
// Theremine.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#include "DSStreamPlay.h"
#include "Oscillator.h"
#include "SineWave.h"
#include "SquareWave.h"
#include "TriangleWave.h"
#include "SawtoothWave.h"

class CAutolock
{
public:
	CAutolock(CRITICAL_SECTION *cs) : m_cs(cs)
	{
		EnterCriticalSection(m_cs);
	}

	virtual ~CAutolock()
	{
		LeaveCriticalSection(m_cs);
	}

protected:
	CRITICAL_SECTION *m_cs;
};

template <typename T> class CSyncVar
{
public:
	CSyncVar()
	{
		InitializeCriticalSection(&m_cs);
	}

	CSyncVar(const T init_val)
	{
		m_Val = init_val;
		InitializeCriticalSection(&m_cs);
	}

	virtual ~CSyncVar()
	{
		DeleteCriticalSection(&m_cs);
	}

	T &Get()
	{
		CAutolock lock(&m_cs);
		return m_Val;
	}

	void Set(T val)
	{
		CAutolock lock(&m_cs);
		m_Val = val;
	}

	T *Lock()
	{
		EnterCriticalSection(&m_cs);
		return &m_Val;
	}

	void Unlock()
	{
		LeaveCriticalSection(&m_cs);
	}

protected:
	T m_Val;
	CRITICAL_SECTION m_cs;
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
		CAutolock lock(&m_cs);
		return m_MinVal;
	}

	void SetMin(T &minval)
	{
		CAutolock lock(&m_cs);
		m_MinVal = minval;
	}

	T &GetMax()
	{
		CAutolock lock(&m_cs);
		return m_MinVax;
	}

	void SetMax(T &maxval)
	{
		CAutolock lock(&m_cs);
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
	CSyncVar<COscillator *> m_pOscillator;
	CSyncVar<bool> m_Enabled;

	props::IProperty *prop_osctype;
	props::IProperty *prop_freqmin;
	props::IProperty *prop_freqmax;
	props::IProperty *prop_freqsteps;

	CSyncVar<float> m_curVolume;
	CSyncVar<float> m_curFrequency;

	typedef enum
	{
		R_PALM_NPOS = 0,	// Normalized Position of the Right Palm
		R_PALM_DDOWN,		// Dot product of [DOWN] and the Right Palm normal

		L_PALM_NPOS,		// Normalized Position of the Left Palm
		L_PALM_DDOWN,		// Dot product of [DOWN] and the Left Palm normal

		NUMVALS
	} INPUT_TYPE;

	typedef enum
	{
		FREQUENCY = 0,
		VOLUME,

		NUMTYPES
	} MODULATOR_TYPE;

	// Control Values for HAND_TARGET items, all in the range of [0..1]
	CSyncVar<float> m_ControlValue[INPUT_TYPE::NUMVALS];

	LARGE_INTEGER m_Frequency;

public:
	CTheremineApp();
	virtual ~CTheremineApp();

	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

extern CTheremineApp theApp;
