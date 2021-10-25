#include "stdafx.h"
#include "blockchainpeer.h"


CBlockChainPeer::CBlockChainPeer()
{
}


CBlockChainPeer::~CBlockChainPeer()
{
}

void CBlockChainPeer::OnResponseServerIP(SBlockChainMsg& msg_)
{
	if (strcmp(m_txClient.m_szServerIP, (char*)msg_.param) != 0)
	{
		DbgLog("server ip:%s", (char*)msg_.param);
		strcpy_s(m_txClient.m_szServerIP, (const char*)msg_.param);
	}
}
