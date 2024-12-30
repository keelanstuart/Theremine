// **************************************************************
// Theremine Source File
// An optical theremin for the Leap Motion controller series of devices
//
// Copyright © 2020-2025, Keelan Stuart

#include "pch.h"
#include "framework.h"
#include "Theremine.h"
#include "TheremineDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTheremineDlg dialog


CTheremineDlg::CTheremineDlg(CWnd* pParent) : CDialogEx(IDD_THEREMINE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CTheremineDlg::~CTheremineDlg()
{
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
	{
		m_EffectList.AddItem(it->first, it->second);
	}

	SetTimer('PING', 10, nullptr);

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
