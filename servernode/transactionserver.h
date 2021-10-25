#pragma once
#include "../blockchainbase/transaction.h"
#include <list>
#include <random>

//a transaction module of server node.
class CTransactionServer
{
public:
	//server node and peer node have 1:n connections.
	//in server node side one connection is class CTansactionConn
	class CTansactionConn : public CActiveTransaction
	{
	public:
		CTansactionConn(const int32 sock_, CTransactionServer* server_)
		{
			m_sock = sock_;
			m_server = server_;
			
			//generate a random token.
			std::random_device	randDevice;
			uint32	total = 0;

			for (int i = 0; i < OTP_LENGTH; i++)
			{
				auto n = randDevice() % 10;
				token[i] = n + '0';
				for (int j = 0; j < OTP_LENGTH - 1 - i; j++)
				{
					n *= 10;
				}
				total += n;
			}

			if (total < 100000)
			{
				total += 100000;
				token[0] = '1';
			}
			
			//following code is C++11 standard's Lambda expression.
			//when thread is valued, it will immediately start.
			m_thread = std::thread([&]()
			{
				//DbgLog("[CTansactionConn::thread] begin");
				while (m_isInitialized == false)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}

				char	buffer[64];
				int		nn = strlen(RNCT_TX_ACK_CONN)+1;
				char*	pp = buffer;

				memcpy(pp, RNCT_TX_ACK_CONN, nn);
				pp += nn;
				memcpy(pp, token, sizeof(token));
				pp += sizeof(token);

				int nSend = send(m_sock, buffer, pp - buffer, 0);
				if (nSend <= 0)
				{
					//DbgLog("[CTansactionConn::thread] send RNCT_TX_ACK_CONN fail");
					Disconnect();
					return;
				}
				//DbgLog("[CTansactionConn::thread] send RNCT_TX_ACK_CONN success, nSend:%d", nSend);


				//process transaction request from client.
				while (m_isRunable)
				{
					if (counter > MAX_OTP_COUNTER)
					{
						//DbgLog("[CTansactionConn::thread] conter over, close");
						Disconnect();
						return;
					}

					int	nResult = recv(m_sock, buffer, sizeof(buffer), 0);

					if (nResult <= 0)
					{
						//error part
						//DbgLog("[CTansactionConn::thread] recv fail");
						Disconnect();
						return;
					}
					//DbgLog("[CTansactionConn::thread] recv success, nResult:%d", nResult);

					pp = buffer;
					nn = strlen(RNCT_TX_REQ_00) + 1;
					pp += nn;

					if (memcmp(buffer, RNCT_TX_REQ_00, nn) != 0)
					{
						//DbgLog("[CTansactionConn::thread] RNCT_TX_REQ_00 fail");
						Disconnect();
						return;
					}
					//DbgLog("[CTansactionConn::thread] RNCT_TX_REQ_00 success");

					NextOtp();
					//DbgLog("[CTansactionConn::thread] token:%c%c%c%c, conter:%d, otpString:%c%c%c%c", token[0], token[1], token[2], token[3], counter, otpString[0], otpString[1], otpString[2], otpString[3]);

					if (memcmp(otpString, pp, sizeof(otpString)) != 0)
					{
						//DbgLog("[CTansactionConn::thread] otpString fail");
						Disconnect();
						return;
					}
					//DbgLog("[CTansactionConn::thread] otpString success");

					pp = buffer;
					nn = strlen(RNCT_TX_ACK_00) + 1;
					memcpy(buffer, RNCT_TX_ACK_00, nn);
					pp += nn;

					nResult = send(m_sock, buffer, nn, 0);
					if (nResult <= 0)
					{
						//DbgLog("[CTansactionConn::thread] send RNCT_TX_ACK_00 fail");
						Disconnect();
						return;
					}

					//DbgLog("[CTansactionConn::thread] send RNCT_TX_ACK_00 success, nResult:%d", nResult);

					m_server->AddTranaction(data());

					std::this_thread::sleep_for(std::chrono::milliseconds(500));
				}
			});

			m_isRunable = true;
			m_isInitialized = true;
		}

		virtual ~CTansactionConn() {}
	protected:
		void Disconnect()
		{
			m_isRunable = false;
			Close();
			m_server->RemoveConn(this);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

	private:
		bool	m_isInitialized = false;
		CTransactionServer*	m_server = nullptr;
	};

public:
	CTransactionServer();
	~CTransactionServer();
public:
	void RemoveConn(CTansactionConn* conn_);					//remove one connection from list
	void AddTranaction(STransaction& tx_);			//add new transaction to list
public:
	std::list<CTansactionConn*>			m_listConn;		//active connection list
	std::list<STransaction>				m_listTx;		//transaction data list
	char								m_szServerIP[32];
	uint32	m_sockTxServer = INVALID_SOCKET;	//a network socket of server.
	bool	m_isServerRunable = false;
};

