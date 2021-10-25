#include "stdafx.h"
#include "transactionclient.h"


CTransactionClient::CTransactionClient()
{
	//DbgLog("[CTransactionClient::CTransactionClient]");
	//main thread will start.
	m_isRunable = true;

	//following code is C++11 standard's Lambda expression.
	//when thread is valued, it will immediately start.
	m_thread = std::thread([&]()
	{
		//DbgLog("[CTransactionClient::Thread] begin");
		while (m_isRunable)
		{
			//DbgLog("[CTransactionClient::Thread] try to connect");
			if (Connect())
			{
				//DbgLog("[CTransactionClient::Thread] begin connection success");
				while (Request())
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
				}
				Close();
			}
			//DbgLog("[CTransactionClient::Thread] connetion closed.");
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	});
}

CTransactionClient::~CTransactionClient()
{
	m_isRunable = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

bool CTransactionClient::Connect()
{
	//DbgLog("[CTransactionClient::Connect] begin");
	//create socket
	if (m_sock != INVALID_SOCKET)
		Close();

	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//DbgLog("[CTransactionClient::Connect] m_sock:%d", m_sock);

	if (m_sock == INVALID_SOCKET)
	{
		return false;
	}

	//DbgLog("[CTransactionClient::Connect] m_szServerIP=%s", m_szServerIP);
	//connect to server
	sockaddr_in	addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = inet_addr(m_szServerIP);// inet_addr(RNCT_TX_SERVER_ADDR);
	addrServer.sin_port = htons(RNCT_TX_SERVER_PORT);

	if (connect(m_sock, (sockaddr*)&addrServer, sizeof(addrServer)) == SOCKET_ERROR)
	{
		//DbgLog("[CTransactionClient::Connect] connet to server fail.");
		Close();
		return false;
	}

	//DbgLog("[CTransactionClient::Connect] connet to server success.");

	//get token
	char	buffer[64];
	memset(buffer, 0, sizeof(buffer));
	int nRecv = recv(m_sock, buffer, sizeof(buffer), 0);

	if (nRecv <= 0)
	{
		//DbgLog("[CTransactionClient::Connect] recv fail.");
		Close();
		return false;
	}
	//DbgLog("[CTransactionClient::Connect] recv success, nRecv:%d", nRecv);

	int		nn = strlen(RNCT_TX_ACK_CONN) + 1;
	char*	pp = buffer;
	
	if (memcmp(pp, RNCT_TX_ACK_CONN, nn) != 0)
	{
		//DbgLog("[CTransactionClient::Connect] RNCT_TX_ACK_CONN fail, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X", pp[0], pp[1], pp[2], pp[3], pp[4], pp[5], pp[6], pp[7], pp[8], pp[9]);
		Close();
		return false;
	}
	//DbgLog("[CTransactionClient::Connect] RNCT_TX_ACK_CONN success");

	pp += nn;
	memcpy(token, pp, sizeof(token));

	if (token[0] <= '0' || token[0] > '9')
	{
		//DbgLog("[CTransactionClient::Connect] token fail");
		Close();
		return false;
	}

	//DbgLog("[CTransactionClient::Connect] connection success");
	return true;
}

bool CTransactionClient::Request()
{
	//DbgLog("[CTransactionClient::Request] begin");
	//calculate next otp
	NextOtp();
	if (counter > MAX_OTP_COUNTER)
	{
		//DbgLog("[CTransactionClient::Request] counter over");
		return false;
	}
	//DbgLog("[CTransactionClient::Request] token:%c%c%c%c, counter:%d, otpString:%c%c%c%c", token[0], token[1], token[2], token[3], counter, otpString[0], otpString[1], otpString[2], otpString[3]);

	//make packet(otp) to send to server
	char	buffer[64];
	int		nn = strlen(RNCT_TX_REQ_00) + 1;
	char*	pp = buffer;
	memcpy(pp, RNCT_TX_REQ_00, nn);
	pp += nn;
	memcpy(pp, &otpString, sizeof(otpString));
	pp += sizeof(otpString);

	//send packet to server
	if (send(m_sock, buffer, pp - buffer, 0) == SOCKET_ERROR)
	{
		//DbgLog("[CTransactionClient::Request] send fail");
		Close();
		return false;
	}
	//DbgLog("[CTransactionClient::Request] send RNCT_TX_REQ_00 success");

	//recv result form server
	int nRecv = recv(m_sock, buffer, sizeof(buffer), 0);

	if (nRecv <= 0)
	{
		//DbgLog("[CTransactionClient::Request] recv fail");
		Close();
		return false;
	}
	//DbgLog("[CTransactionClient::Request] recv success, nRecv:%d", nRecv);

	nn = strlen(RNCT_TX_ACK_00) + 1;
	pp = buffer;
	
	if (memcmp(pp, RNCT_TX_ACK_00, nn) != 0)
	{
		//DbgLog("[CTransactionClient::Request] cmp RNCT_TX_ACK_00 fail");
		Close();
		return false;
	}

	//DbgLog("[CTransactionClient::Request] new transaction requested");

	return true;
}
