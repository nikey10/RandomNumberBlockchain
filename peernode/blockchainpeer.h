#pragma once
#include "../blockchainbase/blockchainbase.h"
#include "transactionclient.h"

class CBlockChainPeer : public CBlockChainBase
{
public:
	CBlockChainPeer();
	~CBlockChainPeer();
protected:
	virtual void OnResponseServerIP(SBlockChainMsg& msg_);
public:
	CTransactionClient	m_txClient;
};

