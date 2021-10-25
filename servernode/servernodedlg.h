
// ServerNodeDlg.h : header file
//

#pragma once
#include "blockchainminer.h"


// CServerNodeDlg dialog
class CServerNodeDlg : public CDialogEx
{
// Construction
public:
	CServerNodeDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVERNODE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	CBlockChainMiner	m_miner;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnLog(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	CListBox m_listLog;
	afx_msg void OnBnClickedButtonClear();
	afx_msg void OnBnClickedCancel();
};
