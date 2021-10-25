#pragma once
#include "type.h"
#include "db.h"
#include "transaction.h"
#include <thread>
#include <queue>
#include <mutex>
#include <vector>


enum EBlockChainMsgKind : uint32
{
	BCMK_REQUEST_BLOCK,
	BCMK_RESPONSE_BLOCK,
	BCMK_REQUEST_RANDOM,
	BCMK_RESPONSE_RANDOM,
	BCMK_RESPONSE_SERVER_IP,
};

static const uint32		RNCT_MSG_MAX = 500;
static const uint16		RNCT_BC_BROADCAST_PORT = (uint16)12879;

//a structure of network packet
//size:504 bytes
struct SBlockChainMsg
{
	EBlockChainMsgKind		kind;
	uint8					param[RNCT_MSG_MAX];
};

class CBlockChainBase
{
public:
	CBlockChainBase()
	{
		//Startup();
	}
	virtual	~CBlockChainBase()
	{
		Stop();
	}
public:
	void Stop()
	{
		//DbgLog("[CBlockChainBase::Stop]");
		m_flagMainThread = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	void Start()
	{
		//DbgLog("[CBlockChainBase::Start]");
		if (m_flagMainThread == true)
		{
			return;
		}
		Startup();
		m_flagMainThread = true;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	void RequestRandom()
	{
		SBlockChainMsg	msg;
		msg.kind = BCMK_REQUEST_RANDOM;
		m_qSendBuffer.push(msg);
		DbgLog("\n");
		DbgLog("send request of random number, current last block[%d]", m_chain.size()-1);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
protected:
	void Startup()
	{
		//DbgLog("[CBlockChainBase::Startup]");

		m_threadMain = std::thread([this]()
		{
			while (m_flagMainThread == false)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
			MainThreadProc();
		});
	}
	void	MainThreadProc()
	{
		//DbgLog("[CBlockChainBase::MainThreadProc] - begin");

		if (InitializeNetwork() == false)
		{
			m_flagMainThread = false;
			return;
		}

		//DbgLog("[CBlockChainBase::MainThreadProc] - Initialize Network OK");


		uint32 nBlocks;
		for (nBlocks=0; nBlocks<RNCT_DB_RECORDS_MAX; nBlocks++)
		{
			//DbgLog("[CBlockChainBase::MainThreadProc] [%d/%d]", nBlocks, RNCT_DB_RECORDS_MAX);
			char	szRecordKey[32];
			sprintf_s(szRecordKey, 32, "%s%0000d", RCNT_DB_RECORD_PRIFIX, nBlocks);
			SRecord		tempRecord;
			
			if (m_db.Get(tempRecord, szRecordKey) < 0)
				break;

			SBlock	newBlock;
			newBlock = *(SBlock*)tempRecord.value;

			if (CheckBlock(newBlock) == false)
				break;

			m_chain.emplace_back(newBlock);
		}

		//DbgLog("[CBlockChainBase::MainThreadProc] - make chain from DB blocks:%d", nBlocks);

		m_threadSend = std::thread([this]()
		{
			m_flagSendThread = true;
			SendThreadProc();
		});

		m_threadRecv = std::thread([this]()
		{
			m_flagRecvThread = true;
			RecvThreadProc();
		});

		//DbgLog("[CBlockChainBase::MainThreadProc] loop start");

		while (m_flagMainThread)
		{
			SBlockChainMsg msg;
			if (GetMessage(msg))
			{
				switch (msg.kind)
				{
				case BCMK_REQUEST_BLOCK:
					//DbgLog("[CBlockChainBase::MainThreadProc] msg:BCMK_REQUEST_BLOCK");
					OnRequestBlock(msg);
					break;
				case BCMK_RESPONSE_BLOCK:
					//DbgLog("[CBlockChainBase::MainThreadProc] msg:BCMK_RESPONSE_BLOCK");
					OnResponseBlock(msg);
					break;
				case BCMK_REQUEST_RANDOM:
					//DbgLog("[CBlockChainBase::MainThreadProc] msg:BCMK_REQUEST_RANDOM");
					OnRequestRandom(msg);
					break;
				case BCMK_RESPONSE_RANDOM:
					//DbgLog("[CBlockChainBase::MainThreadProc] msg:BCMK_RESPONSE_RANDOM");
					OnResponseRandom(msg);
					break;
				case BCMK_RESPONSE_SERVER_IP:
					//DbgLog("[CBlockChainBase::MainThreadProc] msg:BCMK_RESPONSE_SERVER_IP");
					OnResponseServerIP(msg);
					break;
				default:
					//DbgLog("[CBlockChainBase::MainThreadProc] unknown msg:%d", msg.kind);
					break;
				}
			}

			OnIdle();
		}

		FinalizeNetwork();
		//DbgLog("[CBlockChainBase::MainThreadProc] end");
	}

	void	FinalizeNetwork()
	{
		//DbgLog("[CBlockChainBase::FinalizeNetwork] begin");
		m_flagRecvThread = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		m_flagSendThread = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		if (m_sockSend != INVALID_SOCKET)
		{
			closesocket(m_sockSend);
			m_sockSend = INVALID_SOCKET;
		}
		if (m_sockRecv != INVALID_SOCKET)
		{
			closesocket(m_sockRecv);
			m_sockRecv = INVALID_SOCKET;
		}
		//DbgLog("[CBlockChainBase::FinalizeNetwork] end");
	}

	void	BroadcastLastBlock(bool flagAnytime_)
	{
		static std::chrono::system_clock::time_point tp;
		std::chrono::system_clock::time_point tpNow = std::chrono::system_clock::now();
		std::chrono::duration<double> sec = tpNow - tp;
		if (flagAnytime_ || sec.count() > 3.0f)
		{
			tp = tpNow;
			if (m_chain.empty() == false)
			{
				SBlock lastBlock = m_chain.back();
				SBlockChainMsg	newMsg;
				newMsg.kind = BCMK_RESPONSE_BLOCK;
				memcpy(newMsg.param, &lastBlock, sizeof(lastBlock));

				//m_mtxSendBuffer.lock();
				m_qSendBuffer.push(newMsg);
				//m_mtxSendBuffer.unlock();
				//DbgLog("[CBlockChainBase::BroadcastLastBlock] block[%d] was broadcasted", lastBlock.nTimeStamp);
			}
		}
	}

	bool	InitializeNetwork()
	{
		m_sockSend = socket(PF_INET, SOCK_DGRAM, 0);
		if (m_sockSend == INVALID_SOCKET)
			return false;

		int	nOption = 1;
		if (setsockopt(m_sockSend, SOL_SOCKET, SO_BROADCAST, (char*)&nOption, sizeof(nOption)) == SOCKET_ERROR)
		{
			closesocket(m_sockSend);
			m_sockSend = INVALID_SOCKET;
			return false;
		}

		m_sockRecv = socket(PF_INET, SOCK_DGRAM, 0);
		if (m_sockRecv == INVALID_SOCKET)
		{
			closesocket(m_sockSend);
			m_sockSend = INVALID_SOCKET;
			return false;
		}

		sockaddr_in	addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(RNCT_BC_BROADCAST_PORT);

		if (bind(m_sockRecv, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			closesocket(m_sockSend);
			m_sockSend = INVALID_SOCKET;
			closesocket(m_sockRecv);
			m_sockRecv = INVALID_SOCKET;
			return false;
		}

		return true;
	}

	void	SendThreadProc()
	{
		//DbgLog("[CBlockChainBase::SendThreadProc] begin");
		char	name[256] = "localhost";
		char*	address = "";
		hostent* pHostEnt;
		in_addr	in;
		sockaddr_in	broadAddr;
		uint32	nMyIP, nMySubMask, nTemp1, nTemp2;

		gethostname(name, sizeof(name));
		//DbgLog("[CBlockChainBase::SendThreadProc] name=%s", name);
		pHostEnt = gethostbyname(name);
		//DbgLog("[CBlockChainBase::SendThreadProc] pHostEnt=0x%X", pHostEnt);
		for (int i = 0; i < pHostEnt->h_length; i++)
		{
			in.s_addr = ((in_addr*)pHostEnt->h_addr_list[i])->s_addr;
			address = inet_ntoa(in);
			if (strncmp(address, "192.168.", 8) == 0)
			{
				break;
			}
		}

		//DbgLog("[CBlockChainBase::SendThreadProc] address:%s", address);
		nMyIP = inet_addr(address);
		nMySubMask = inet_addr("255.255.255.0");
		nTemp1 = nMyIP & nMySubMask;
		nTemp2 = nMySubMask ^ 0xFFFFFFFF;

		memset(&broadAddr, 0, sizeof(broadAddr));
		broadAddr.sin_family = AF_INET;
		broadAddr.sin_port = htons(RNCT_BC_BROADCAST_PORT);
		broadAddr.sin_addr.s_addr = nTemp1 + nTemp2;
	
		while (m_flagSendThread)
		{
			while (m_qSendBuffer.empty() == false)
			{
				//m_mtxSendBuffer.lock();

				SBlockChainMsg	msg = m_qSendBuffer.front();
				m_qSendBuffer.pop();
				sendto(m_sockSend, (char*)&msg, sizeof(msg), 0, (sockaddr*)&broadAddr, sizeof(broadAddr));

				//m_mtxSendBuffer.unlock();
			}
		}
		//DbgLog("[CBlockChainBase::SendThreadProc] end");
	}

	void	RecvThreadProc()
	{
		//DbgLog("[CBlockChainBase::RecvThreadProc] begin");
		while (m_flagRecvThread)
		{
			SBlockChainMsg	msg;
			int packetLen = recvfrom(m_sockRecv, (char*)&msg, sizeof(msg), 0, nullptr, 0);
			if (packetLen > 0)
			{
				m_qRecvBuffer.push(msg);
			}
		}
		//DbgLog("[CBlockChainBase::RecvThreadProc] end");
	}

	bool	GetMessage(SBlockChainMsg& msg_)
	{

		if (m_qRecvBuffer.empty())
		{
			return false;
		}

		msg_ = m_qRecvBuffer.front();
		m_qRecvBuffer.pop();

		return true;
	}

	virtual void	OnIdle()
	{
		BroadcastLastBlock(false);
	}

	virtual void OnRequestBlock(SBlockChainMsg& msg_)
	{
		uint32	nBlock = *(uint32*)msg_.param;
		if (nBlock < m_chain.size())
		{
			SBlockChainMsg	newMsg;
			newMsg.kind = BCMK_RESPONSE_BLOCK;
			memcpy(newMsg.param, &m_chain[nBlock], sizeof(SBlock));

			//m_mtxSendBuffer.lock();
			m_qSendBuffer.push(newMsg);
			//m_mtxSendBuffer.unlock();
			DbgLog("send old block:%d", nBlock);
		}
	}

	virtual void OnResponseBlock(SBlockChainMsg& msg_)
	{
		SBlock*	pBlock = (SBlock*)msg_.param;
		//DbgLog("[CBlockChainBase::OnResponseBlock] new:0x%X, cur:%d", pBlock->nTimeStamp, m_chain.size());
		if (pBlock->nTimeStamp == m_chain.size())
		{
			if (CheckBlock(*pBlock) == true)
			{
				//m_mtxChain.lock();
				m_chain.emplace_back(*pBlock);
				//DbgLog("[CBlockChainBase::OnResponseBlock] new block added");
				//m_mtxChain.unlock();
			}
		}
		else if (pBlock->nTimeStamp > m_chain.size())
		{
			SBlockChainMsg	newMsg;
			newMsg.kind = BCMK_REQUEST_BLOCK;
			*(uint32*)newMsg.param = m_chain.size();

			//m_mtxSendBuffer.lock();
			m_qSendBuffer.push(newMsg);
			//m_mtxSendBuffer.unlock();
		}
		else
		{
			//nothing
		}
	}

	virtual void OnRequestRandom(SBlockChainMsg& msg_)
	{
		//nothing
	}

	virtual void OnResponseRandom(SBlockChainMsg& msg_)
	{
		uint8*	p = msg_.param;
		uint32	timestamp = *(uint32*)p;
		p += sizeof(uint32);
		double dblRandom = *(double*)p;
		//DbgLog("[CBlockChainBase::OnResponseRandom]  block:%d, random:%f", timestamp, dblRandom);

		auto lastBlock = m_chain.back();
		if (lastBlock.nTimeStamp+1 < timestamp)
		{
			SBlockChainMsg	newMsg;
			newMsg.kind = BCMK_REQUEST_BLOCK;
			*(uint32*)newMsg.param = m_chain.size();

			//m_mtxSendBuffer.lock();
			m_qSendBuffer.push(newMsg);
			//m_mtxSendBuffer.unlock();

			//DbgLog("[CBlockChainBase::OnResponseRandom] request block:%d, random:%f", m_chain.size());
			return;
		}
		double random = crypto::normal_random(lastBlock.cipherTx);
		DbgLog(" >> receive random: *** %f ***, block[%d]", dblRandom, timestamp);
	}

	virtual void OnResponseServerIP(SBlockChainMsg& msg_)
	{
		//DbgLog("[CBlockChainBase::OnResponseServerIP]");
	}
protected:
	bool CheckBlock(const SBlock& block_)
	{
		//DbgLog("[CBlockChainBase::CheckBlock] block: timestamp:0x%X, prev:0x%X, hash:0x%X, count:0x%X", block_.nTimeStamp, block_.hashPrev, block_.hashTx, block_.nTxCount);
		if (m_chain.empty())
		{
			//genesis block.
			//DbgLog("[CBlockChainBase::CheckBlock] this block is gemesis");
			if (block_.nTimeStamp != RNCT_BLK_GENESIS_TIMESTAMP ||
				block_.hashPrev != RNCT_BLK_GENESIS_HASH)
			{
				//DbgLog("[CBlockChainBase::CheckBlock] gemesis block check fail");
				return false;
			}
			//DbgLog("[CBlockChainBase::CheckBlock]  gemesis check success");
		}
		else
		{
			//check hashPrev
			uint32 nTimeStamp = block_.nTimeStamp - 1;
			auto	prevBlock = m_chain[nTimeStamp];
			uint32 hashPrev = crypto::hash32((const uint8*)&prevBlock, sizeof(prevBlock));
			if (block_.hashPrev != hashPrev)
			{
				//DbgLog("[CBlockChainBase::CheckBlock] check prev fail, hashPrev:0x%X", hashPrev);
				return false;
			}
			//DbgLog("[CBlockChainBase::CheckBlock] check prev success");
		}

		//check hashTx
		uint32	hashRoot = RNCT_BLK_ROOTHASH_INIT;
		struct
		{
			uint32			hash;
			STransaction	tx;
		}tempBuffer;
		for (uint32 nTxCount=0; nTxCount<block_.nTxCount; nTxCount++)
		{
			tempBuffer.hash = hashRoot;
			tempBuffer.tx = block_.tx[nTxCount];
			hashRoot = crypto::hash32((const uint8*)&tempBuffer, sizeof(tempBuffer));
		}
		if (hashRoot != block_.hashTx)
		{
			//DbgLog("[CBlockChainBase::CheckBlock] hash check fail, hashRoot:0x%X", hashRoot);
			return false;
		}
		//DbgLog("[CBlockChainBase::CheckBlock] hash check success");

		//check cipherTx
		uint64 cipherTx;
		crypto::rsa_enc8_buffer((const uint8*)&hashRoot, (uint16*)&cipherTx, sizeof(hashRoot));
		if (cipherTx != block_.cipherTx)
		{
			//DbgLog("[CBlockChainBase::CheckBlock] hash check fail");
			return false;
		}
																
		DbgLog("\n");
		DbgLog("receive new block[%d]", block_.nTimeStamp);
		DbgLog("          prev block hash: 0x%X", block_.hashPrev);
		DbgLog("          new block hash: 0x%X", block_.hashTx);
		DbgLog("          transaction count: %d", block_.nTxCount);
		DbgLog("          cipher of hash: 0x%llX", block_.cipherTx);
		DbgLog("          random number: %f", crypto::normal_random(cipherTx));
		return true;
	}
protected:
	CDB							m_db;			//blockchain on disk
	std::vector<SBlock>			m_chain;		//blockchain int memory
	//std::mutex					m_mtxChain;

	std::thread					m_threadMain;
	bool						m_flagMainThread = false;

	uint32						m_sockSend = INVALID_SOCKET;
	std::thread					m_threadSend;
	std::queue<SBlockChainMsg>	m_qSendBuffer;
	//std::mutex					m_mtxSendBuffer;
	bool						m_flagSendThread = false;

	uint32						m_sockRecv = INVALID_SOCKET;
	std::thread					m_threadRecv;
	std::queue<SBlockChainMsg>	m_qRecvBuffer;
	//std::mutex					m_mtxRecvBuffer;
	bool						m_flagRecvThread = false;
};

