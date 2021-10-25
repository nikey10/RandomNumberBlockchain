
// PeerNodeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "peernode.h"
#include "peernodedlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPeerNodeDlg dialog



CPeerNodeDlg::CPeerNodeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PEERNODE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPeerNodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LOG, m_listLog);
}

BEGIN_MESSAGE_MAP(CPeerNodeDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(RCNT_LOG_MSG, OnLog)
	ON_BN_CLICKED(IDCANCEL, &CPeerNodeDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, &CPeerNodeDlg::OnBnClickedButtonClear)
	ON_BN_CLICKED(IDC_BUTTON_RANDOM, &CPeerNodeDlg::OnBnClickedButtonRandom)
END_MESSAGE_MAP()


// CPeerNodeDlg message handlers

BOOL CPeerNodeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	g_hwndMain = m_hWnd;
	m_peer.Start();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPeerNodeDlg::OnOK()
{

}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPeerNodeDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPeerNodeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CPeerNodeDlg::OnLog(WPARAM wParam, LPARAM lParam)
{
	char * szMsg = (char*)wParam;
	if (strlen(szMsg) > 0)
	{
		m_listLog.AddString(szMsg);
		m_listLog.SetCurSel(m_listLog.GetCount() - 1);
	}
	return 0;
}

void CPeerNodeDlg::OnBnClickedCancel()
{
	m_peer.Stop();
	::Sleep(1000);
	CDialogEx::OnCancel();
}

void CPeerNodeDlg::OnBnClickedButtonClear()
{
	while (m_listLog.GetCount())
	{
		m_listLog.DeleteString(0);
	}
}


void CPeerNodeDlg::OnBnClickedButtonRandom()
{
	m_peer.RequestRandom();
}
