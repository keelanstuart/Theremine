#pragma once

class CAnimListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CAnimListCtrl)

public:
	CAnimListCtrl();
	virtual ~CAnimListCtrl();

	bool AddItem(CTheremineApp::EFFECT_TYPE modulator_type, CTheremineApp::INPUT_TYPE input_type);

protected:


	DECLARE_MESSAGE_MAP()

public:
	virtual void MeasureItem(LPMEASUREITEMSTRUCT pmi);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void DrawItem(LPDRAWITEMSTRUCT pdi);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
};


