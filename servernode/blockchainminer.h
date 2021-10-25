#pragma once
#include "../blockchainbase/blockchainbase.h"
#include "../blockchainbase/crypto.h"
#include "transactionserver.h"

class CBlockChainMiner : public CBlockChainBase
{
public:
	CBlockChainMiner();
	virtual ~CBlockChainMiner();
public:
	CTransactionServer	m_txServer;
protected:
	virtual uint32	Mine(bool flagRandomRequest_);
	virtual void	OnIdle();
	virtual void	OnRequestRandom(SBlockChainMsg& msg_);
	void			BroadcastServerIP();
};

