
// Theremine.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "Theremine.h"
#include "TheremineDlg.h"
#include "Vec3.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTheremineApp theApp;

BEGIN_MESSAGE_MAP(CTheremineApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

int operator < (const LEAP_DEVICE_REF &a, const LEAP_DEVICE_REF &b)
{
	return memcmp(&a, &b, sizeof(LEAP_DEVICE_REF));
}

// CTheremineApp construction

CTheremineApp::CTheremineApp() : 
	m_pOscillator(nullptr), m_Enabled(false)
{
	m_ControlValue[CTheremineApp::INPUT_TYPE::L_PALM_NPOS].Set(0.0f);
	m_ControlValue[CTheremineApp::INPUT_TYPE::R_PALM_NPOS].Set(0.0f);
	m_ControlValue[CTheremineApp::INPUT_TYPE::L_PALM_DDOWN].Set(0.0f);
	m_ControlValue[CTheremineApp::INPUT_TYPE::R_PALM_DDOWN].Set(0.0f);

	m_pProps = nullptr;
	m_pLeapPool = nullptr;
	m_LeapConn = nullptr;
	m_iSampleRate = 44100;

	QueryPerformanceFrequency(&m_Frequency);
}

CTheremineApp::~CTheremineApp()
{
	if (m_pLeapPool)
	{
		m_pLeapPool->PurgeAllPendingTasks();
		m_pLeapPool->Release();
		m_pLeapPool = nullptr;
	}

	COscillator **pposc = m_pOscillator.Lock();
	if (pposc && *pposc)
	{
		delete *pposc;
		*pposc = nullptr;
		m_pOscillator.Lock();
	}

	if (m_pProps)
	{
		m_pProps->DeleteAll();
		m_pProps->Release();
		m_pProps = nullptr;
	}
}


const LEAP_HAND *FindFirstHand(const LEAP_TRACKING_EVENT *evt, eLeapHandType ht)
{
	if (!evt || !evt->nHands)
		return nullptr;

	for (uint32_t i = 0; i < evt->nHands; i++)
	{
		if (evt->pHands[i].type == ht)
			return &(evt->pHands[i]);
	}

	return nullptr;
}

#define LEAP_MINDIST	80.0f
#define LEAP_MAXDIST	400.0f
#define LEAP_DISTRANGE	(LEAP_MAXDIST - LEAP_MINDIST)

float PalmNormal(const LEAP_HAND *phand)
{
	static vec3 down(0.0f, -1.0f, 0.0f);

	if (!phand)
		return 0.0f;

	vec3 palmnorm(phand->palm.normal.x, phand->palm.normal.y, phand->palm.normal.z);
	float d = palmnorm.dot(down);
	d = std::min<float>(std::max<float>(0.0f, d), 1.0f);

	return d;
}

float PalmNormDistance(const LEAP_HAND *phand)
{
	if (!phand)
		return -1.0f;

	float p = sqrtf(
		(phand->palm.position.x * phand->palm.position.x) +
		(phand->palm.position.y * phand->palm.position.y) +
		(phand->palm.position.z * phand->palm.position.z));

	p = std::min<float>(std::max<float>(LEAP_MINDIST, p), LEAP_MAXDIST) - LEAP_MINDIST;
	p /= LEAP_DISTRANGE;

	return p;
}

pool::IThreadPool::TASK_RETURN __cdecl PollLeapTask(void *param0, void *param1, size_t task_number)
{
	CTheremineApp *_this = (CTheremineApp *)param0;

	eLeapRS lr;

	if (_this->m_LeapConn == nullptr)
	{
		lr = LeapCreateConnection(NULL, &_this->m_LeapConn);

		if (LEAP_SUCCEEDED(lr))
		{
			lr = LeapOpenConnection(_this->m_LeapConn);
			if (LEAP_FAILED(lr))
			{
				LeapDestroyConnection(_this->m_LeapConn);
				_this->m_LeapConn = nullptr;
			}
		}

		return pool::IThreadPool::TASK_RETURN::TR_REQUEUE;
	}

	LEAP_CONNECTION_MESSAGE msg;

	lr = LeapPollConnection(_this->m_LeapConn, 1000, &msg);
	if (LEAP_FAILED(lr))
	{
		Sleep(3000);
		return pool::IThreadPool::TASK_RETURN::TR_REQUEUE;
	}

	switch (msg.type)
	{
		case eLeapEventType_Connection:
			break;

		case eLeapEventType_ConnectionLost:
			LeapCloseConnection(_this->m_LeapConn);
			_this->m_LeapConn = nullptr;
			_this->m_LeapDev.clear();
			break;

		// Poor documentation from Leap, but...
		// This is how the devices are returned after an initial poll
		case eLeapEventType_Device:
		{
			const LEAP_DEVICE_EVENT *devevt = msg.device_event;
			if (devevt->status & eLeapDeviceStatus_Streaming)
			{
				if (_this->m_LeapDev.find(devevt->device) == _this->m_LeapDev.end())
				{
					auto insret = _this->m_LeapDev.insert(CTheremineApp::TLeapDevMap::value_type(devevt->device, CTheremineApp::SLeapDevData()));
					
					LeapOpenDevice(devevt->device, &(insret.first->second.dev));

					insret.first->second.info.serial = &(insret.first->second.serial.at(0));
					lr = LeapGetDeviceInfo(insret.first->second.dev, &insret.first->second.info);

					if (lr == eLeapRS_InsufficientBuffer)
					{
						insret.first->second.serial.resize(insret.first->second.info.serial_length);
						insret.first->second.info.serial = &(insret.first->second.serial.at(0));
						lr = LeapGetDeviceInfo(insret.first->second.dev, &insret.first->second.info);
					}
				}
			}

			Sleep(100);

			break;
		}

		case eLeapEventType_DeviceLost:
			_this->m_LeapDev.clear();
			break;

		case eLeapEventType_DeviceFailure:
			break;

		case eLeapEventType_Tracking:
		{
			const LEAP_TRACKING_EVENT *trkevt = msg.tracking_event;

			bool b = (trkevt->nHands > 0);
			if (b)
			{
				const LEAP_HAND* phand = nullptr;

				phand = FindFirstHand(trkevt, eLeapHandType_Right);
				if (phand)
				{
					float d = PalmNormal(phand);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_PALM_DDOWN].Set(d);

					float p = PalmNormDistance(phand);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_PALM_NPOS].Set(p);

					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_FIST_TIGHTNESS].Set(1.0f - phand->grab_strength);
				}

				phand = FindFirstHand(trkevt, eLeapHandType_Left);
				if (phand)
				{
					float d = PalmNormal(phand);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_PALM_DDOWN].Set(d);

					float p = PalmNormDistance(phand);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_PALM_NPOS].Set(p);

					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_FIST_TIGHTNESS].Set(1.0f - phand->grab_strength);
				}
			}

			_this->m_Enabled.Set(b);
			break;
		}

		case eLeapEventType_LogEvent:
			break;

		case eLeapEventType_Policy:
			break;

		case eLeapEventType_ConfigChange:
			break;

		case eLeapEventType_ConfigResponse:
			break;

		default:
			break;
	}

	return pool::IThreadPool::TASK_RETURN::TR_REQUEUE;
}


class CPropChangeListener : public props::IPropertyChangeListener
{
public:
	virtual void PropertyChanged(const props::IProperty *pprop)
	{
		if (!pprop)
			return;

		switch (pprop->GetID())
		{
			case 'OSCI':
			{
				COscillator **pposc = theApp.m_pOscillator.Lock();
				if (pposc && pposc)
				{
					delete *pposc;
					*pposc = nullptr;
				}

				if (!_tcsicmp(pprop->AsString(), _T("Sine")))
				{
					*pposc = new CSineWave();
				}
				else if (!_tcsicmp(pprop->AsString(), _T("Square")))
				{
					*pposc = new CSquareWave();
				}
				else if (!_tcsicmp(pprop->AsString(), _T("Triangle")))
				{
					*pposc = new CTriangleWave();
				}
				else if (!_tcsicmp(pprop->AsString(), _T("Sawtooth")))
				{
					*pposc = new CSawtoothWave();
				}

				if (pposc && *pposc)
					(*pposc)->Initialize(0.0f, theApp.m_iSampleRate, 0.0f);

				theApp.m_pOscillator.Unlock();
				break;
			}
		}
	}

};

CPropChangeListener pcl;


const TCHAR *CTheremineApp::GetInputName(CTheremineApp::INPUT_TYPE it)
{
	switch (it)
	{
		case R_PALM_NPOS:		return _T("Right Hand Position");
		case R_PALM_DDOWN:		return _T("Right Palm Normal");
		case R_FIST_TIGHTNESS:	return _T("Right Fist Tightness");

		case L_PALM_NPOS:		return _T("Left Hand Position");
		case L_PALM_DDOWN:		return _T("Left Palm Normal");
		case L_FIST_TIGHTNESS:	return _T("Left Fist Tightness");
	};

	return nullptr;
}


const TCHAR *CTheremineApp::GetEffectName(CTheremineApp::EFFECT_TYPE et)
{
	switch (et)
	{
		case FREQUENCY:			return _T("Frequency");
		case VOLUME:			return _T("Volume");
		case DISTORTION:		return _T("Distortion");
		case REVERB:			return _T("Reverb");
	}

	return nullptr;
}


BOOL CTheremineApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);

	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::Status st = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	if (st != Gdiplus::Status::Ok)
		return FALSE;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Theremine"));

	m_EffectMap.insert(TEffectInputMap::value_type(CTheremineApp::EFFECT_TYPE::FREQUENCY, CTheremineApp::INPUT_TYPE::R_PALM_NPOS));
	m_EffectMap.insert(TEffectInputMap::value_type(CTheremineApp::EFFECT_TYPE::VOLUME, CTheremineApp::INPUT_TYPE::R_PALM_DDOWN));
	//m_EffectMap.insert(TEffectInputMap::value_type(CTheremineApp::EFFECT_TYPE::DISTORTION, CTheremineApp::INPUT_TYPE::L_PALM_DDOWN));

	m_pProps = props::IPropertySet::CreatePropertySet();
	if (!m_pProps)
		return FALSE;

	m_pProps->SetChangeListener(&pcl);

	prop_osctype = m_pProps->CreateProperty(_T("Oscillator Type"), 'OSCI');
	if (prop_osctype)
	{
		prop_osctype->SetEnumStrings(_T("Sine,Square,Triangle,Sawtooth"));
		prop_osctype->SetEnumVal(0);
	}

	prop_freqmin = m_pProps->CreateProperty(_T("Frequency (Min)"), 'MINF');
	if (prop_freqmin)
		prop_freqmin->SetFloat(50.0f);

	prop_freqmax = m_pProps->CreateProperty(_T("Frequency (Max)"), 'MAXF');
	if (prop_freqmax)
		prop_freqmax->SetFloat(1500.0f);

	prop_freqsteps = m_pProps->CreateProperty(_T("Frequency (Steps)"), 'STPF');
	if (prop_freqsteps)
		prop_freqsteps->SetInt(500);

	m_pLeapPool = pool::IThreadPool::Create(1);
	if (!m_pLeapPool)
		return FALSE;

	m_pLeapPool->RunTask(PollLeapTask, this);

	CTheremineDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();

	if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
	}

	// Delete the shell manager created above.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

