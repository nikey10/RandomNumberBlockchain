#pragma once
#include "../blockchainbase/transaction.h"

//a transaction module of peer node.
class CTransactionClient : public CActiveTransaction
{
public:
	CTransactionClient();
	~CTransactionClient();

private:
	bool Connect();	//try to connect to server and retrieve token.
	bool Request();	//create a new transaction.
public:
	char	m_szServerIP[32] = {0};
};
