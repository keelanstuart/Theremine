// **************************************************************
// Theremine Source File
// An optical theremin for the Leap Motion controller series of devices
//
// Copyright © 2020-2025, Keelan Stuart

#pragma once


// CMeterCtl

class CMeterCtl : public CWnd
{
	DECLARE_DYNAMIC(CMeterCtl)

public:
	CMeterCtl();
	virtual ~CMeterCtl();

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	void SetLimits(int lower, int upper, bool update = true);
	void SetValue(int val, bool update = true);

protected:
	int m_LowerLimit, m_UpperLimit, m_Value;

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
};


