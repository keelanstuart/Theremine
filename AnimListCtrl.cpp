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

#define ANIM_ITEM_HEIGHT		100

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
	cs.dwExStyle |= LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT;// | LVS_EX_ONECLICKACTIVATE;

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

	int vsw = 0;

	if (cy < (GetItemCount() * ANIM_ITEM_HEIGHT))
	{
		//vsw = GetSystemMetrics(SM_CXVSCROLL);
	}

	SetColumnWidth(1, cx - GetColumnWidth(0) - vsw);

	RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
	RedrawItems(0, GetItemCount());
	//((CReactorDocView *)GetParent())->RedrawActiveTimeline();
	//Invalidate(FALSE);
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

	Gdiplus::SolidBrush br_bg(Gdiplus::Color(255, 45, 45, 45));
	Gdiplus::SolidBrush br_text(Gdiplus::Color(255, 255, 255, 255));
	Gdiplus::SolidBrush br_acc(Gdiplus::Color(255, 16, 16, 16));
	Gdiplus::LinearGradientBrush br_scmd(Gdiplus::Rect(0, 0, th, th), Gdiplus::Color(255, 128, 128, 100), Gdiplus::Color(255, 32, 32, 10), Gdiplus::LinearGradientMode::LinearGradientModeVertical);
	Gdiplus::Pen pn_acc(&br_acc, 1);

	CRect ritem = pdi->rcItem, rcol[2], rcli;

	GetClientRect(rcli);
	GetSubItemRect(pdi->itemID, 0, LVIR_BOUNDS, rcol[0]);
	GetSubItemRect(pdi->itemID, 1, LVIR_BOUNDS, rcol[1]);

//	if (!(pdi->itemID & 1))
//		gr.FillRectangle(&br_bg, ritem.left, ritem.top, ritem.Width(), ritem.Height());

	Gdiplus::Font f_list(pdi->hDC);// , Gdiplus::FontFamily(_T("Arial")), 12);
	CString name = GetItemText(pdi->itemID, 0);
	Gdiplus::RectF tnr;
	gr.MeasureString((LPCTSTR)name, name.GetLength(), &f_list, Gdiplus::PointF(0.0, 0.0), &tnr);
	gr.DrawString(name, name.GetLength(), &f_list, Gdiplus::PointF(10.0, (Gdiplus::REAL)(ritem.CenterPoint().y - (tnr.Height / 2.0))), &br_text);

	LOGFONT lf_cmd;
	f_list.GetLogFont(&gr, &lf_cmd);
	lf_cmd.lfWeight = FW_EXTRALIGHT;
	lf_cmd.lfHeight--;
	lf_cmd.lfItalic = FALSE;// TRUE;
	Gdiplus::Font f_cmd(pdi->hDC, &lf_cmd);

	Gdiplus::Point p1(rcol[1].left, ritem.CenterPoint().y);
	Gdiplus::Point p2(rcol[1].right, p1.Y);

	gr.DrawLine(&pn_acc, p1, p2);

	gr.SetClip(Gdiplus::Rect(rcol[1].left, rcol[1].top, rcol[1].Width(), rcol[1].Height()));

	Gdiplus::LinearGradientBrush br_cmd(Gdiplus::Rect(0, 0, rcol[1].Width(), rcol[1].Height()), Gdiplus::Color(255, 0, 0, 100), Gdiplus::Color(255, 100, 200, 255), Gdiplus::LinearGradientMode::LinearGradientModeHorizontal);

//	gr.FillRectangle(&br_cmd, rcol[1]);

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

