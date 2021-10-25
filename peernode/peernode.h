
// PeerNode.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CPeerNodeApp:
// See PeerNode.cpp for the implementation of this class
//

class CPeerNodeApp : public CWinApp
{
public:
	CPeerNodeApp();
	~CPeerNodeApp();

// Overrides
public:
	virtual BOOL InitInstance();
private:
	int m_wsa;

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CPeerNodeApp theApp;
