// **************************************************************
// Theremine Source File
// An optical theremin for the Leap Motion controller series of devices
//
// Copyright © 2020-2025, Keelan Stuart

#include "pch.h"
#include "Theremine.h"
#include "MeterCtl.h"


// CMeterCtl

IMPLEMENT_DYNAMIC(CMeterCtl, CWnd)

CMeterCtl::CMeterCtl()
{
	m_LowerLimit = m_UpperLimit = m_Value = 0;
}

CMeterCtl::~CMeterCtl()
{
}


BEGIN_MESSAGE_MAP(CMeterCtl, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


BOOL CMeterCtl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	return CWnd::Create(GetGlobalData()->RegisterWindowClass(_T("MeterCtl")), _T(""), dwStyle, rect, pParentWnd, nID, NULL);
}

// CMeterCtl message handlers

void CMeterCtl::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CMemDC mdc(dc, this);

	CRect r;
	GetClientRect(r);

	Gdiplus::Graphics gr(mdc.GetDC());
	Gdiplus::SolidBrush br_bg(Gdiplus::Color(255, 0, 0, 0));
	gr.FillRectangle(&br_bg, r.left, r.top, r.Width(), r.Height());
	dc.Draw3dRect(r, RGB(200, 200, 200), RGB(80, 80, 80));

	if (m_UpperLimit == m_LowerLimit)
		return;

	r.DeflateRect(3, 3, 3, 3);

	float ul = (float)m_UpperLimit;
	float ll = (float)m_LowerLimit;

	float p = ((float)m_Value - ll) / (ul - ll);
	int dh = (int)((float)r.Height() * (1.0f - p));
	r.DeflateRect(0, dh, 0, 0);

	Gdiplus::LinearGradientBrush br_bar(Gdiplus::Rect(0, 0, r.right, r.bottom), Gdiplus::Color(255, 255, 64, 128), Gdiplus::Color(255, 64, 255, 128), Gdiplus::LinearGradientMode::LinearGradientModeVertical);
	gr.FillRectangle(&br_bar, r.left, r.top, r.Width(), r.Height());

	dc.BitBlt(0, 0, r.right, r.bottom, &(mdc.GetDC()), 0, 0, SRCCOPY);
}


void CMeterCtl::SetLimits(int lower, int upper, bool update)
{
	m_LowerLimit = lower;
	m_UpperLimit = upper;

	if (update)
		RedrawWindow(nullptr, nullptr, RDW_NOERASE | RDW_UPDATENOW);
}


void CMeterCtl::SetValue(int val, bool update)
{
	m_Value = val;

	if (update)
		RedrawWindow(nullptr, nullptr, RDW_NOERASE | RDW_UPDATENOW | RDW_INVALIDATE);
}


BOOL CMeterCtl::OnEraseBkgnd(CDC *pDC)
{
	return FALSE;
}
