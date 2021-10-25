
// PeerNodeDlg.h : header file
//

#pragma once
#include "blockchainpeer.h"


// CPeerNodeDlg dialog
class CPeerNodeDlg : public CDialogEx
{
// Construction
public:
	CPeerNodeDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PEERNODE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	CBlockChainPeer		m_peer;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnLog(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	CListBox m_listLog;
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonClear();
	afx_msg void OnBnClickedButtonRandom();
};
