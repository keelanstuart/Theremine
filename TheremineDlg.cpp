
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
	if (posc && *posc)
	{
		int nSamples = dwAmountRequested / sizeof(int16_t);

		float freq = theApp.m_curFrequency.Get();
		(*posc)->SetFrequency(freq);

		float vol = theApp.m_curVolume.Get();
		(*posc)->SetAmplitude(vol);

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

	int m = rc.right - rpl.right;
	CRect rb(m, m, rpl.left - m, rpl.bottom);

	m /= 2;
	CRect rvb(rb);
	rvb.right = (rb.Width() / 2) + m;
	m_VolBar.Create(WS_CHILD | WS_VISIBLE, rvb, this, IDC_VOLBAR);
	m_VolBar.SetLimits(0, 100);

	CRect rfb(rb);
	rfb.left = rvb.right + m + m;
	m_FreqBar.Create(WS_CHILD | WS_VISIBLE, rfb, this, IDC_FREQBAR);

	m_pDynamicLayout->AddItem(m_VolBar.GetSafeHwnd(), CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeVertical(100));
	m_pDynamicLayout->AddItem(m_FreqBar.GetSafeHwnd(), CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeVertical(100));

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
		float frmin = theApp.prop_freqmin->AsFloat();
		float frmax = theApp.prop_freqmax->AsFloat();
		float fr = theApp.m_curFrequency.Get();
		float vol = theApp.m_curVolume.Get();

		m_FreqBar.SetLimits((int)frmin, (int)frmax, false);
		m_FreqBar.SetValue((int)fr, true);

		m_VolBar.SetValue((int)(vol * 100.0f), true);
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CTheremineDlg::OnOK()
{
//	CDialogEx::OnOK();
}


void CTheremineDlg::OnCancel()
{
	CDialogEx::OnCancel();
}


BOOL CTheremineDlg::OnEraseBkgnd(CDC *pDC)
{
	return FALSE; //CDialogEx::OnEraseBkgnd(pDC);
}


void CTheremineDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	CRect r;
	m_PropList.GetClientRect(r);
}
