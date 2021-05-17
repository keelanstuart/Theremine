// Source\TrackTimeline.cpp : implementation file
//

#include "pch.h"
#include "Theremine.h"
#include "AnimListCtrl.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// CAnimListCtrl

#define ANIM_ITEM_HEIGHT		40

IMPLEMENT_DYNAMIC(CAnimListCtrl, CListCtrl)

CAnimListCtrl::CAnimListCtrl()
{
}

CAnimListCtrl::~CAnimListCtrl()
{
}

BEGIN_MESSAGE_MAP(CAnimListCtrl, CListCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_MEASUREITEM_REFLECT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DRAWITEM()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()


void MyAppendMenuItem(HMENU menu, UINT32 type, const TCHAR *text = NULL, BOOL enabled = true, HMENU submenu = NULL, UINT id = -1, DWORD_PTR data = NULL);

void MyAppendMenuItem(HMENU menu, UINT32 type, const TCHAR *text, BOOL enabled, HMENU submenu, UINT id, DWORD_PTR data)
{
	MENUITEMINFO mii;
	memset(&mii, 0, sizeof(MENUITEMINFO));

	mii.cbSize = sizeof(MENUITEMINFO);

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_DATA;

	mii.wID = (id == -1) ? GetMenuItemCount(menu) : id;

	mii.fType = type;

	if ((type == MFT_STRING) || text)
	{
		mii.dwTypeData = (LPWSTR)text;
		mii.cch = (UINT)_tcslen(text);
	}

	if (!enabled)
	{
		mii.fMask |= MIIM_STATE;
		mii.fState = MFS_GRAYED;
	}

	if (submenu)
	{
		mii.hSubMenu = submenu;
		mii.fMask |= MIIM_SUBMENU;
	}

	mii.dwItemData = data;

	InsertMenuItem(menu, GetMenuItemCount(menu), true, &mii);
}

bool CAnimListCtrl::AddItem(CTheremineApp::EFFECT_TYPE effect_type, CTheremineApp::INPUT_TYPE input_type)
{
	const TCHAR *s = _T("");
	int ic = GetItemCount();

	int ii = InsertItem(ic, s);

	SetItemData(ii, (DWORD_PTR)(effect_type));

	return true;
}

void CAnimListCtrl::MeasureItem(LPMEASUREITEMSTRUCT pmi)
{
	CRect r;
	GetClientRect(r);
	pmi->itemHeight = ANIM_ITEM_HEIGHT;
	pmi->itemWidth = r.Width();
}


BOOL CAnimListCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= LVS_OWNERDRAWFIXED | LVS_NOCOLUMNHEADER | LVS_REPORT;
	//cs.dwExStyle |= LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT;// | LVS_EX_ONECLICKACTIVATE;

	return CListCtrl::PreCreateWindow(cs);
}


int CAnimListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}


void CAnimListCtrl::OnSize(UINT nType, int cx, int cy)
{
	CListCtrl::OnSize(nType, cx, cy);

	int bw = ((cx - GetColumnWidth(0)) / 2) - 2;
	
	SetColumnWidth(1, bw);
	SetColumnWidth(2, bw);

	RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
}

#if defined(UNICODE)
#define GetLogFont	GetLogFontW
#else
#define GetLogFont	GetLogFontA
#endif

void CAnimListCtrl::DrawItem(LPDRAWITEMSTRUCT pdi)
{
	CPaintDC *pdc = (CPaintDC *)CDC::FromHandle(pdi->hDC);

	INT th = ANIM_ITEM_HEIGHT;

	Gdiplus::Graphics gr(*pdc);

	Gdiplus::SolidBrush br_bg(Gdiplus::Color(255, 75, 75, 75));
	Gdiplus::SolidBrush br_text(Gdiplus::Color(255, 255, 255, 255));
	Gdiplus::SolidBrush br_acc(Gdiplus::Color(255, 16, 16, 16));
	Gdiplus::LinearGradientBrush br_scmd(Gdiplus::Rect(0, 0, th, th), Gdiplus::Color(255, 128, 128, 100), Gdiplus::Color(255, 32, 32, 10), Gdiplus::LinearGradientMode::LinearGradientModeVertical);
	Gdiplus::Pen pn_acc(&br_acc, 1);

	CRect ritem = pdi->rcItem, rcol[3], rcli;

	GetClientRect(rcli);
	GetSubItemRect(pdi->itemID, 0, LVIR_BOUNDS, rcol[0]);
	GetSubItemRect(pdi->itemID, 1, LVIR_BOUNDS, rcol[1]);
	GetSubItemRect(pdi->itemID, 2, LVIR_BOUNDS, rcol[2]);

	if (!(pdi->itemID & 1))
		gr.FillRectangle(&br_bg, ritem.left, ritem.top, rcli.Width(), ritem.Height());

	Gdiplus::RectF tnr;

	Gdiplus::Font f_list(pdi->hDC);// , Gdiplus::FontFamily(_T("Arial")), 12);

	CString effect_name = theApp.GetEffectName((CTheremineApp::EFFECT_TYPE)pdi->itemData);
	gr.MeasureString((LPCTSTR)effect_name, effect_name.GetLength(), &f_list, Gdiplus::PointF(0.0, 0.0), &tnr);
	gr.DrawString(effect_name, effect_name.GetLength(), &f_list, Gdiplus::PointF((Gdiplus::REAL)rcol[1].left + 10.0f, (Gdiplus::REAL)(ritem.CenterPoint().y - (tnr.Height / 2.0))), &br_text);

	CTheremineApp::TEffectInputMap::const_iterator it = theApp.m_EffectMap.find((CTheremineApp::EFFECT_TYPE)pdi->itemData);
	CString input_name = theApp.GetInputName(it->second);
	gr.MeasureString((LPCTSTR)input_name, input_name.GetLength(), &f_list, Gdiplus::PointF(0.0f, 0.0f), &tnr);
	gr.DrawString(input_name, input_name.GetLength(), &f_list, Gdiplus::PointF((Gdiplus::REAL)rcol[2].left + 10.0f, (Gdiplus::REAL)(ritem.CenterPoint().y - (tnr.Height / 2.0))), &br_text);

	rcol[0].right = rcol[1].left - 1;
	gr.SetClip(Gdiplus::Rect(rcol[0].left, rcol[0].top, rcol[0].Width(), rcol[0].Height()));

	rcol[0].DeflateRect(10, 12, 4, 12);
	Gdiplus::SolidBrush br_r(Gdiplus::Color(255, 0, 0, 0));
	Gdiplus::Pen pn_r(Gdiplus::Color(255, 255, 255, 255));
	gr.DrawRectangle(&pn_r, rcol[0].left, rcol[0].top, rcol[0].Width(), rcol[0].Height());

	rcol[0].DeflateRect(1, 1, 1, 1);
	gr.FillRectangle(&br_r, rcol[0].left, rcol[0].top, rcol[0].Width(), rcol[0].Height());

	if (theApp.m_Enabled.Get())
	{
		Gdiplus::LinearGradientBrush br_bar(Gdiplus::Rect(0, 0, rcol[1].left, 1), Gdiplus::Color(255, 255, 64, 128), Gdiplus::Color(255, 64, 255, 128), Gdiplus::LinearGradientMode::LinearGradientModeHorizontal);

		rcol[0].DeflateRect(1, 1, 1, 0);
		INT w = (INT)((float)rcol[0].Width() * theApp.m_ControlValue[it->second].Get());
		gr.FillRectangle(&br_bar, rcol[0].left, rcol[0].top, w, rcol[0].Height());
	}
}



BOOL CAnimListCtrl::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;//__super::OnEraseBkgnd(pDC);
}


void CAnimListCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CMemDC mdc(dc, this);

	Gdiplus::Graphics gr(mdc.GetDC());

	Gdiplus::SolidBrush br_bg(Gdiplus::Color(255, 40, 40, 40));

	CRect r;
	GetClientRect(r);
	gr.FillRectangle(&br_bg, r.left, r.top, r.Width(), r.Height());

	DefWindowProc(WM_PAINT, (WPARAM)(mdc.GetDC().GetSafeHdc()), (LPARAM)0);

	dc.BitBlt(0, 0, r.right, r.bottom, &(mdc.GetDC()), 0, 0, SRCCOPY);
}

