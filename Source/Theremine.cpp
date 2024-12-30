// **************************************************************
// Theremine Source File
// An optical theremin for the Leap Motion controller series of devices
//
// Copyright © 2020-2025, Keelan Stuart

#include "pch.h"
#include "framework.h"
#include "Theremine.h"
#include "TheremineDlg.h"

#include <glm/glm.hpp>						// https://github.com/g-truc/glm				==> third-party/glm
#include <glm/ext.hpp>						// https://github.com/g-truc/glm				==> third-party/glm
#include <glm/gtx/matrix_decompose.hpp>		// https://github.com/g-truc/glm				==> third-party/glm
#include <cmath>

#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define DEVICE_FORMAT			ma_format_f32
#define DEVICE_CHANNELS			1
#define DEVICE_SAMPLE_RATE		48000


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTheremineApp theApp;
static bool s_Exiting = false;

BEGIN_MESSAGE_MAP(CTheremineApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

int operator < (const LEAP_DEVICE_REF &a, const LEAP_DEVICE_REF &b)
{
	return memcmp(&a, &b, sizeof(LEAP_DEVICE_REF));
}

// CTheremineApp construction

CTheremineApp::CTheremineApp() : 
	m_Enabled(false)
{
	for (size_t i = 0; i < CTheremineApp::INPUT_TYPE::NUM_INPUTS; i++)
		m_ControlValue[i].Set(0);

	m_pProps = nullptr;
	m_pLeapPool = nullptr;
	m_LeapConn = nullptr;
	m_iSampleRate = 44100;
}

CTheremineApp::~CTheremineApp()
{
	if (m_pLeapPool)
	{
		m_pLeapPool->PurgeAllPendingTasks();
		m_pLeapPool->Release();
		m_pLeapPool = nullptr;
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
	static glm::fvec3 down(0.0f, -1.0f, 0.0f);

	if (!phand)
		return 0.0f;

	glm::fvec3 palmnorm(phand->palm.normal.x, phand->palm.normal.y, phand->palm.normal.z);
	float d = glm::dot(palmnorm, down);
	d = std::min<float>(std::max<float>(0.0f, d), 1.0f);

	return d;
}

float PalmNormDistance(const LEAP_HAND *phand, bool height_only = false);

float PalmNormDistance(const LEAP_HAND *phand, bool height_only)
{
	if (!phand)
		return -1.0f;

	glm::fvec3 pos = *(glm::fvec3 *)&(phand->palm.position);
	if (height_only)
	{
		pos.x = 0;
		pos.z = 0;
	}

	float p = sqrtf((pos.x * pos.x) + (pos.y * pos.y) +	(pos.z * pos.z));

	p = std::min<float>(std::max<float>(LEAP_MINDIST, p), LEAP_MAXDIST) - LEAP_MINDIST;
	p /= LEAP_DISTRANGE;

	return p;
}

float FingerAngle(const LEAP_HAND *phand, CTheremineApp::FINGER_INDEX finger)
{
	if (!phand)
		return -1.0f;

	glm::fquat q, qi = glm::identity<glm::fquat>();

	q = *(glm::fquat *)&(phand->digits[finger].proximal.rotation);
	float d = 1.0f - std::min<float>(std::max<float>(0.0f, glm::dot(q, qi)), 1.0f);

	return d;
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

				// if the app is exiting, then don't requeue this task
				if (s_Exiting)
					return pool::IThreadPool::TASK_RETURN::TR_OK;

				// wait before trying to re-initialize
				::Sleep(2000);
			}
		}

		return pool::IThreadPool::TASK_RETURN::TR_REQUEUE;
	}

	LEAP_CONNECTION_MESSAGE msg;

	lr = LeapPollConnection(_this->m_LeapConn, 100, &msg);
	if (LEAP_FAILED(lr))
	{
		if (s_Exiting)
			return pool::IThreadPool::TASK_RETURN::TR_OK;

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
			for (size_t i = 0; i < CTheremineApp::INPUT_TYPE::NUM_INPUTS; i++)
				_this->m_ControlValue[i].Set(0);
			break;

		case eLeapEventType_DeviceFailure:
			for (size_t i = 0; i < CTheremineApp::INPUT_TYPE::NUM_INPUTS; i++)
				_this->m_ControlValue[i].Set(0);
			break;

		case eLeapEventType_Tracking:
		{
			const LEAP_TRACKING_EVENT *trkevt = msg.tracking_event;

			bool b = (trkevt->nHands > 0);
			if (b)
			{
				const LEAP_HAND* phand = nullptr;

				float d, p, f, g;

				phand = FindFirstHand(trkevt, eLeapHandType_Right);
				if (phand)
				{
					d = PalmNormal(phand);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_PALM_DDOWN].Set(d);

					p = PalmNormDistance(phand);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_PALM_NPOS].Set(p);

					g = 1.0f - (phand->grab_strength * phand->grab_strength);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_FIST_TIGHTNESS].Set(g);

					f = FingerAngle(phand, CTheremineApp::FINGER_INDEX::THUMB);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_FINGER_THUMB_ANG].Set(f);

					f = FingerAngle(phand, CTheremineApp::FINGER_INDEX::INDEX);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_FINGER_INDEX_ANG].Set(f);

					f = FingerAngle(phand, CTheremineApp::FINGER_INDEX::MIDDLE);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_FINGER_MIDDLE_ANG].Set(f);

					f = FingerAngle(phand, CTheremineApp::FINGER_INDEX::RING);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_FINGER_RING_ANG].Set(f);

					f = FingerAngle(phand, CTheremineApp::FINGER_INDEX::PINKY);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::R_FINGER_PINKY_ANG].Set(f);
				}

				phand = FindFirstHand(trkevt, eLeapHandType_Left);
				if (phand)
				{
					d = PalmNormal(phand);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_PALM_DDOWN].Set(d);

					p = PalmNormDistance(phand);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_PALM_NPOS].Set(p);

					g = 1.0f - (phand->grab_strength * phand->grab_strength);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_FIST_TIGHTNESS].Set(g);

					f = FingerAngle(phand, CTheremineApp::FINGER_INDEX::THUMB);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_FINGER_THUMB_ANG].Set(f);

					f = FingerAngle(phand, CTheremineApp::FINGER_INDEX::INDEX);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_FINGER_INDEX_ANG].Set(f);

					f = FingerAngle(phand, CTheremineApp::FINGER_INDEX::MIDDLE);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_FINGER_MIDDLE_ANG].Set(f);

					f = FingerAngle(phand, CTheremineApp::FINGER_INDEX::RING);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_FINGER_RING_ANG].Set(f);

					f = FingerAngle(phand, CTheremineApp::FINGER_INDEX::PINKY);
					_this->m_ControlValue[CTheremineApp::INPUT_TYPE::L_FINGER_PINKY_ANG].Set(f);
				}
			}
			else
			{
				for (size_t i = 0; i < CTheremineApp::INPUT_TYPE::NUM_INPUTS; i++)
					_this->m_ControlValue[i].Set(0);
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
				if (!_tcsicmp(pprop->AsString(), _T("Sine")))
				{
					ma_waveform_set_type(&theApp.m_Wave, ma_waveform_type_sine);
				}
				else if (!_tcsicmp(pprop->AsString(), _T("Square")))
				{
					ma_waveform_set_type(&theApp.m_Wave, ma_waveform_type_square);
				}
				else if (!_tcsicmp(pprop->AsString(), _T("Triangle")))
				{
					ma_waveform_set_type(&theApp.m_Wave, ma_waveform_type_triangle);
				}
				else if (!_tcsicmp(pprop->AsString(), _T("Sawtooth")))
				{
					ma_waveform_set_type(&theApp.m_Wave, ma_waveform_type_sawtooth);
				}

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
		case R_PALM_NPOS:			return _T("Right Hand Position");
		case R_PALM_DDOWN:			return _T("Right Palm Normal");
		case R_FIST_TIGHTNESS:		return _T("Right Fist Tightness");
		case R_PINCH_TIGHTNESS:		return _T("Right Pinch Tightness");
		case R_FINGER_THUMB_ANG:	return _T("Right Thumb Angle");
		case R_FINGER_INDEX_ANG:	return _T("Right Index Finger Angle");
		case R_FINGER_MIDDLE_ANG:	return _T("Right Middle Finger Angle");
		case R_FINGER_RING_ANG:		return _T("Right Ring Finger Angle");
		case R_FINGER_PINKY_ANG:	return _T("Right Pinky Finger Angle");

		case L_PALM_NPOS:			return _T("Left Hand Position");
		case L_PALM_DDOWN:			return _T("Left Palm Normal");
		case L_FIST_TIGHTNESS:		return _T("Left Fist Tightness");
		case L_PINCH_TIGHTNESS:		return _T("Left Pinch Tightness");
		case L_FINGER_THUMB_ANG:	return _T("Left Thumb Angle");
		case L_FINGER_INDEX_ANG:	return _T("Left Index Finger Angle");
		case L_FINGER_MIDDLE_ANG:	return _T("Left Middle Finger Angle");
		case L_FINGER_RING_ANG:		return _T("Left Ring Finger Angle");
		case L_FINGER_PINKY_ANG:	return _T("Left Pinky Finger Angle");
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
		case PITCH_BEND:		return _T("Pitch Bend");
	}

	return nullptr;
}


void WaveformDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	ma_waveform *pwave;

	CTheremineApp::TEffectInputMap::const_iterator it;

	double frmin = theApp.prop_freqmin->AsFloat();
	double frmax = theApp.prop_freqmax->AsFloat();
	double frdiff = frmax - frmin;
	double frsteps = theApp.prop_freqsteps->AsFloat();
	double frstepsize = frdiff / frsteps;

	it = theApp.m_EffectMap.find(CTheremineApp::EFFECT_TYPE::FREQUENCY);
	double p = theApp.m_ControlValue[it->second].Get();

	it = theApp.m_EffectMap.find(CTheremineApp::EFFECT_TYPE::VOLUME);

	static float last_vol = 0.0f;
	float vol = theApp.m_ControlValue[it->second].Get();
	float sustain = theApp.prop_sustain->AsFloat();
	float mix_vol = glm::mix<float>(vol, last_vol, sustain);
	last_vol = mix_vol;

	switch (theApp.prop_intmode->AsInt())
	{
		case 0:
			// linear
			break;

		case 1:
			// exponential
			p = p * p;
			break;
	}

	if (vol != 0)
	{
		double frmul = p * frsteps;
		double fr = frstepsize * floor(frmul) + frmin;

		it = theApp.m_EffectMap.find(CTheremineApp::EFFECT_TYPE::PITCH_BEND);
		double b = theApp.m_ControlValue[it->second].Get();
		fr += b * (frdiff / 12.0f);

		ma_waveform_set_frequency(&theApp.m_Wave, fr);
	}

	ma_waveform_set_amplitude(&theApp.m_Wave, mix_vol);

	pwave = (ma_waveform *)pDevice->pUserData;
	ma_waveform_read_pcm_frames(pwave, pOutput, frameCount, NULL);
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

	AssociateInputWithEffect(CTheremineApp::EFFECT_TYPE::FREQUENCY, CTheremineApp::INPUT_TYPE::R_PALM_NPOS);
//	AssociateInputWithEffect(CTheremineApp::EFFECT_TYPE::VOLUME, CTheremineApp::INPUT_TYPE::R_PALM_DDOWN);
	AssociateInputWithEffect(CTheremineApp::EFFECT_TYPE::VOLUME, CTheremineApp::INPUT_TYPE::R_FIST_TIGHTNESS);
//	AssociateInputWithEffect(CTheremineApp::EFFECT_TYPE::PITCH_BEND, CTheremineApp::INPUT_TYPE::R_FINGER_INDEX_ANG);
	AssociateInputWithEffect(CTheremineApp::EFFECT_TYPE::PITCH_BEND, CTheremineApp::INPUT_TYPE::R_PALM_DDOWN);
//	AssociateInputWithEffect(CTheremineApp::EFFECT_TYPE::DISTORTION, CTheremineApp::INPUT_TYPE::L_PALM_DDOWN);

	m_pProps = props::IPropertySet::CreatePropertySet();
	if (!m_pProps)
		return FALSE;

	m_pProps->SetChangeListener(&pcl);

	prop_osctype = m_pProps->CreateProperty(_T("Oscillator Type"), 'OSCI');
	if (prop_osctype)
	{
		prop_osctype->SetEnumStrings(_T("Sine,Square,Triangle,Sawtooth"));
		prop_osctype->SetEnumVal(1);
	}

	prop_freqmin = m_pProps->CreateProperty(_T("Frequency (Min)"), 'MINF');
	if (prop_freqmin)
		prop_freqmin->SetFloat(440.0f);

	prop_freqmax = m_pProps->CreateProperty(_T("Frequency (Max)"), 'MAXF');
	if (prop_freqmax)
		prop_freqmax->SetFloat(880.0f);

	prop_freqsteps = m_pProps->CreateProperty(_T("Frequency (Steps)"), 'STPF');
	if (prop_freqsteps)
		prop_freqsteps->SetInt(120);

	prop_intmode = m_pProps->CreateProperty(_T("Interpolation Mode"), 'INTM');
	if (prop_intmode)
	{
		prop_intmode->SetEnumStrings(_T("Linear,Exponetial"));
		prop_intmode->SetEnumVal(0);
	}

	prop_sustain = m_pProps->CreateProperty(_T("Sustain"), 'SSTN');
	if (prop_sustain)
		prop_sustain->SetFloat(0.9f);

	m_pLeapPool = pool::IThreadPool::Create(1);
	if (!m_pLeapPool)
		return FALSE;

	m_pLeapPool->RunTask(PollLeapTask, this);

	m_DeviceConfig = ma_device_config_init(ma_device_type_playback);
	m_DeviceConfig.playback.format   = DEVICE_FORMAT;
	m_DeviceConfig.playback.channels = DEVICE_CHANNELS;
	m_DeviceConfig.sampleRate        = DEVICE_SAMPLE_RATE;
	m_DeviceConfig.dataCallback      = WaveformDataCallback;
	m_DeviceConfig.pUserData         = &m_Wave;

	if (ma_device_init(NULL, &m_DeviceConfig, &m_Device) != MA_SUCCESS) {
		return FALSE;
	}

	m_WaveConfig = ma_waveform_config_init(m_Device.playback.format, m_Device.playback.channels, m_Device.sampleRate, ma_waveform_type_sine, 0.0, 50);
	ma_waveform_init(&m_WaveConfig, &m_Wave);

	if (ma_device_start(&m_Device) != MA_SUCCESS) {
		ma_device_uninit(&m_Device);
		return FALSE;
	}

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



int CTheremineApp::ExitInstance()
{
	s_Exiting = true;
	ma_device_uninit(&m_Device);

	return CWinApp::ExitInstance();
}

void CTheremineApp::AssociateInputWithEffect(EFFECT_TYPE eff, INPUT_TYPE inp)
{
	m_EffectMap.insert_or_assign(eff, inp);
}
