
// TheremineDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Theremine.h"
#include "TheremineDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTheremineDlg dialog


BOOL WINAPI CTheremineDlg::AudioCallback(PBYTE pBuffer, DWORD dwAmountRequested, DWORD &dwAmountRead, DWORD_PTR InstanceData)
{
	BOOL ret = FALSE;
	dwAmountRead = 0;

	CTheremineDlg *_this = (CTheremineDlg *)InstanceData;

	COscillator **posc = theApp.m_pOscillator.Lock();
	if (posc && *posc && theApp.m_Enabled.Get())
	{
		int nSamples = dwAmountRequested / sizeof(int16_t);

		CTheremineApp::TEffectInputMap::const_iterator cit;
		
		cit = theApp.m_EffectMap.find(CTheremineApp::EFFECT_TYPE::FREQUENCY);
		if (cit != theApp.m_EffectMap.cend())
		{
			float freq_mul = theApp.m_ControlValue[cit->second].Get();

			float steps = (float)(std::max<int64_t>(2, theApp.prop_freqsteps->AsInt() - 1));

			float freq_range = theApp.prop_freqmax->AsFloat() - theApp.prop_freqmin->AsFloat();

			float pos_steps = floor(freq_mul * steps);

			float freq = (pos_steps * freq_range / steps) + theApp.prop_freqmin->AsFloat();
			(*posc)->SetFrequency(freq);
		}

		cit = theApp.m_EffectMap.find(CTheremineApp::EFFECT_TYPE::VOLUME);
		if (cit != theApp.m_EffectMap.cend())
		{
			float vol = theApp.m_ControlValue[cit->second].Get();
			(*posc)->SetAmplitude(vol);
		}

		cit = theApp.m_EffectMap.find(CTheremineApp::EFFECT_TYPE::DISTORTION);
		if (cit != theApp.m_EffectMap.cend())
		{
			float dis = theApp.m_ControlValue[cit->second].Get();
			(*posc)->SetDistortion(dis);
		}

		(*posc)->Generate((int16_t *)pBuffer, nSamples);

		dwAmountRead = dwAmountRequested;
		ret = TRUE;
	}

	theApp.m_pOscillator.Unlock();

	return ret;
}

CTheremineDlg::CTheremineDlg(CWnd* pParent) : CDialogEx(IDD_THEREMINE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pStream = nullptr;
}

CTheremineDlg::~CTheremineDlg()
{
	if (m_pStream)
	{
		m_pStream->StopBuffers();
		delete m_pStream;
		m_pStream = nullptr;
	}
}

void CTheremineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	if (!pDX->m_bSaveAndValidate)
	{
		DDX_Control(pDX, IDC_PROPERTYGRID, m_PropList);
	}
}

BEGIN_MESSAGE_MAP(CTheremineDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
END_MESSAGE_MAP()

// CTheremineDlg message handlers

BOOL CTheremineDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CRect rc;
	GetClientRect(rc);

	m_PropList.SetActiveProperties(theApp.m_pProps);
	CRect rpl;
	m_PropList.GetWindowRect(rpl);
 	ScreenToClient(rpl);
	m_PropList.AdjustLayout();

	CRect rb(rpl.left, rpl.bottom + rpl.top, rpl.right, rc.bottom - rpl.top);
	m_EffectList.Create(WS_CHILD | WS_VISIBLE, rb, this, IDC_EFFECTLIST);

	m_pDynamicLayout->AddItem(m_EffectList.GetSafeHwnd(), CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeHorizontal(100));

	m_EffectList.SetExtendedStyle(LVS_EX_DOUBLEBUFFER);
	m_EffectList.SetView(LV_VIEW_DETAILS);
	m_EffectList.SetBkColor(RGB(40, 40, 40));

	m_EffectList.InsertColumn(0, _T("Value"), LVCFMT_LEFT, 100);
	m_EffectList.InsertColumn(1, _T("Effect"), LVCFMT_LEFT, 100);
	m_EffectList.InsertColumn(2, _T("Input"), LVCFMT_LEFT, 100);

	for (CTheremineApp::TEffectInputMap::const_iterator it = theApp.m_EffectMap.cbegin(), last_it = theApp.m_EffectMap.cend(); it != last_it; it++)
		m_EffectList.AddItem(it->first, it->second);

	SetTimer('PING', 10, nullptr);

	WAVEFORMATEX wfx;
	wfx.cbSize = sizeof(WAVEFORMATEX);
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = theApp.m_iSampleRate;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

	m_pStream = new CDSStreamPlay(GetSafeHwnd(), &wfx, 50);
	m_pStream->SetCallback(AudioCallback, (DWORD_PTR)this);
	m_pStream->StartBuffers();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTheremineDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPaintDC dc(this); // device context for painting

		CMemDC mdc(dc, this);

		CRect r;
		GetClientRect(r);
		mdc.GetDC().FillRect(r, &m_brBkgr);

		DefWindowProc(WM_PAINT, (WPARAM)(mdc.GetDC().GetSafeHdc()), (LPARAM)0);

		dc.BitBlt(0, 0, r.right, r.bottom, &(mdc.GetDC()), 0, 0, SRCCOPY);
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTheremineDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CTheremineDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 'PING')
	{
		m_EffectList.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CTheremineDlg::OnOK()
{
}


void CTheremineDlg::OnCancel()
{
	CDialogEx::OnCancel();
}


BOOL CTheremineDlg::OnEraseBkgnd(CDC *pDC)
{
	return __super::OnEraseBkgnd(pDC);
}


void CTheremineDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
}
