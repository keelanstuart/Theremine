
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

CTheremineApp::CTheremineApp() : m_curFrequency(50.0f), m_curVolume(0.0f), m_pOscillator(nullptr)
{
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

			float p = _this->m_curFrequency.Get(), v = _this->m_curVolume.Get();

			switch (trkevt->nHands)
			{
				case 0:
				{
					v *= 0.8f;
					p = std::max<float>(p * 0.8f, _this->prop_freqmin->AsFloat());
					break;
				}
				case 1:
				{
					static vec3 down(0.0f, -1.0f, 0.0f);

					vec3 palmnorm(trkevt->pHands[0].palm.normal.x, trkevt->pHands[0].palm.normal.y, trkevt->pHands[0].palm.normal.z);
					v = palmnorm.dot(down);
					break;
				}
				case 2:
				{
					break;
				}

				default:
					break;
			}

			v = std::min<float>(std::max<float>(0.0f, v), 1.0f);
			_this->m_curVolume.Set(v);

			if (trkevt->nHands)
			{
				p = sqrtf(
					(trkevt->pHands[0].palm.position.x * trkevt->pHands[0].palm.position.x) +
					(trkevt->pHands[0].palm.position.y * trkevt->pHands[0].palm.position.y) +
					(trkevt->pHands[0].palm.position.z * trkevt->pHands[0].palm.position.z));

#define LEAP_MINDIST	80.0f
#define LEAP_MAXDIST	400.0f

				float steps = (float)(std::max<int64_t>(1, _this->prop_freqsteps->AsInt() - 1));

				// step distance across entire range
				float ds = (LEAP_MAXDIST - LEAP_MINDIST) / steps;

				// clamped range
				float pos_clamped_range = std::min<float>(std::max<float>(LEAP_MINDIST, p), LEAP_MAXDIST) - LEAP_MINDIST;

				float pos_steps = floor(pos_clamped_range / ds);

				float freq_range = _this->prop_freqmax->AsFloat() - _this->prop_freqmin->AsFloat();

				float freq_stepsize = freq_range / steps;

				p = (pos_steps * freq_stepsize) + _this->prop_freqmin->AsFloat();

			}

			_this->m_curFrequency.Set(p);

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
					(*pposc)->Initialize(theApp.m_curFrequency.Get(), theApp.m_iSampleRate, theApp.m_curVolume.Get());

				theApp.m_pOscillator.Unlock();
				break;
			}
		}
	}

};

CPropChangeListener pcl;

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

	m_pProps = props::IPropertySet::CreatePropertySet();
	if (!m_pProps)
		return FALSE;

	m_pProps->SetChangeListener(&pcl);

	prop_osctype = m_pProps->CreateProperty(_T("Oscillator Type"), 'OSCI');
	if (prop_osctype)
	{
		prop_osctype->SetEnumStrings(_T("Square,Triangle,Sawtooth,Sine"));
		prop_osctype->SetEnumVal(1);
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
