#pragma once

#include "PropertyGrid.h"
#include "MeterCtl.h"


// CTheremineDlg dialog
class CTheremineDlg : public CDialogEx
{
// Construction
public:
	CTheremineDlg(CWnd* pParent = nullptr);	// standard constructor
	virtual ~CTheremineDlg();

	enum {
		IDC_VOLBAR = 500,
		IDC_FREQBAR,
	};

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_THEREMINE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:

	static BOOL WINAPI AudioCallback(PBYTE pBuffer, DWORD dwAmountRequested, DWORD &dwAmountRead, DWORD_PTR InstanceData);

	HICON m_hIcon;

	CPropertyGrid m_PropList;
	CMeterCtl m_FreqBar;
	CMeterCtl m_VolBar;

	CDSStreamPlay *m_pStream;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
