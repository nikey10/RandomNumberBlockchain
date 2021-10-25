#include "stdafx.h"
#include "transactionserver.h"
#include <random>
#include <thread>


CTransactionServer::CTransactionServer()
{
	char	name[256] = "";
	char*	address = "";
	hostent* pHostEnt = nullptr;
	in_addr	in;

	//DbgLog("[CTransactionServer::CTransactionServer] begin");
	gethostname(name, sizeof(name));
	//DbgLog("[CTransactionServer::CTransactionServer] name=%s", name);

	pHostEnt = gethostbyname(name);
	//DbgLog("[CTransactionServer::CTransactionServer] pHostEnt=0x%X", pHostEnt);
	if (pHostEnt)
	{
		for (int i = 0; i < pHostEnt->h_length; i++)
		{
			in.s_addr = ((in_addr*)pHostEnt->h_addr_list[i])->s_addr;
			address = inet_ntoa(in);
			if (strncmp(address, "192.168.", 8) == 0)
			{
				break;
			}
		}

	}
	DbgLog("[CTransactionServer::CTransactionServer] address:%s", address);
	strcpy_s(m_szServerIP, address);

	m_sockTxServer = socket(AF_INET, SOCK_STREAM, 0);
	//DbgLog("[CTransactionServer::CTransactionServer] m_sockTxServer:%d", m_sockTxServer);

	if (m_sockTxServer != INVALID_SOCKET)
	{
		sockaddr_in	server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(RNCT_TX_SERVER_PORT);
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		
		if (bind(m_sockTxServer, (struct sockaddr*)&server_addr, sizeof(server_addr)) != SOCKET_ERROR)
		{
			//DbgLog("[CTransactionServer::CTransactionServer] bind success");
			m_isServerRunable = true;
			static std::thread s_txServerThread = std::thread([&]()
			{
				//DbgLog("[CTransactionServer::MainThread] begin");
				sockaddr_in	client_addr;
				if (listen(m_sockTxServer, SOMAXCONN) == SOCKET_ERROR)
				{
					//DbgLog("[CTransactionServer::MainThread] listen fail");
					m_isServerRunable = false;
				}
				else
				{
					//DbgLog("[CTransactionServer::MainThread] listen success");
					while (m_isServerRunable)
					{
						int	nAddrLen = sizeof(client_addr);
						uint32 sockAccept = accept(m_sockTxServer, (sockaddr*)&client_addr, &nAddrLen);
						//DbgLog("[CTransactionServer::MainThread] sockAccept=%d", sockAccept);
						if (sockAccept != INVALID_SOCKET)
						{
							CTansactionConn*	txConn = new CTansactionConn(sockAccept, this);
							m_listConn.push_back(txConn);
							//DbgLog("[CTransactionServer::MainThread] new connection added, txConn:0x%X", txConn);
						}
						else
						{
							//DbgLog("[CTransactionServer::MainThread] accept fail");
						}
					}
				}

				for (auto it = m_listConn.begin(); it != m_listConn.end(); it++)
				{
					(*it)->Close();
					//delete *it;
				}
				m_listConn.clear();
			});
		}

	}
}

CTransactionServer::~CTransactionServer()
{
	m_isServerRunable = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void CTransactionServer::RemoveConn(CTansactionConn* conn_)
{
	for (auto it = m_listConn.begin(); it != m_listConn.end(); it++)
	{
		if ((*it) == conn_)
		{
			//DbgLog("[CTransactionServer::RemoveConn] -1");
			//delete *it;
			//DbgLog("[CTransactionServer::RemoveConn] -2");
			m_listConn.erase(it);
			//DbgLog("[CTransactionServer::RemoveConn] -3");
			break;
		}
	}
}

void CTransactionServer::AddTranaction(STransaction& tx_)
{
	//DbgLog("[CTransactionServer::AddTranaction] new tx added");
	if (m_listTx.size() == RNCT_TX_MAX_COUNT)
	{
		m_listTx.pop_front();
	}
	m_listTx.emplace_back(tx_);
}
