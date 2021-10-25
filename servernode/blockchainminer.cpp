#include "stdafx.h"
#include "blockchainminer.h"


CBlockChainMiner::CBlockChainMiner()
{
	//DbgLog("[CBlockChainMiner::CBlockChainMiner]");
}


CBlockChainMiner::~CBlockChainMiner()
{
}

//mine new block.
//the base data of the new block is transaction list.
uint32 CBlockChainMiner::Mine(bool flagRandomRequest_)
{
	if (flagRandomRequest_ == false && m_txServer.m_listTx.size() < RNCT_TX_MAX_COUNT /* timer */)
	{
		return -1;
	}
	//DbgLog("[CBlockChainMiner::Mine] begin");
	//m_mtxChain.lock();

	SBlock	newBlock;
	if (m_chain.empty())
	{
		//create genesis block.
		//DbgLog("[CBlockChainMiner::Mine] block is genesis");
		newBlock.nTimeStamp = RNCT_BLK_GENESIS_TIMESTAMP;
		newBlock.hashPrev = RNCT_BLK_GENESIS_HASH;
	}
	else
	{
		auto	prevBlock = m_chain.back();
		newBlock.nTimeStamp = prevBlock.nTimeStamp + 1;//crypto::crc32((const uint8*)&prevBlock.nTimeStamp, sizeof(prevBlock.nTimeStamp));
		newBlock.hashPrev = crypto::hash32((const uint8*)&prevBlock, sizeof(prevBlock));
		//DbgLog("[CBlockChainMiner::Mine] block is not genesis, timestamp:%d, prev:0x%X", newBlock.nTimeStamp, newBlock.hashPrev);
	}
	newBlock.nTxCount = 0;
	uint32	hashRoot = RNCT_BLK_ROOTHASH_INIT;
	struct
	{
		uint32			hash;
		STransaction	tx;
	}tempBuffer;
	//DbgLog("[CBlockChainMiner::Mine] txCount:%d", m_txServer.m_listTx.size());
	for (auto it = m_txServer.m_listTx.begin(); it!=m_txServer.m_listTx.end(); it++)
	{
		tempBuffer.hash = hashRoot;
		tempBuffer.tx = *it;
		hashRoot = crypto::hash32((const uint8*)&tempBuffer, sizeof(tempBuffer));
		newBlock.tx[newBlock.nTxCount] = *it;
		newBlock.nTxCount++;
	}

	m_txServer.m_listTx.clear();
	newBlock.hashTx = hashRoot;
	crypto::rsa_enc8_buffer((const uint8*)&hashRoot, (uint16*)&newBlock.cipherTx, sizeof(hashRoot));
	m_chain.push_back(newBlock);
	//m_mtxChain.unlock();
	//DbgLog("[CBlockChainMiner::Mine] new block - txCount:%d, hash:0x%X", newBlock.nTxCount, hashRoot);

	double	dblRandom = crypto::normal_random(newBlock.cipherTx);

	if (flagRandomRequest_)
	{
		SBlockChainMsg msg;
		msg.kind = BCMK_RESPONSE_RANDOM;
		uint8*	p = msg.param;
		memcpy(p, &newBlock.nTimeStamp, sizeof(newBlock.nTimeStamp));
		p += sizeof(newBlock.nTimeStamp);
		memcpy(p, &dblRandom, sizeof(dblRandom));
		m_qSendBuffer.push(msg);

		DbgLog("\n");
		DbgLog("new random number was generated");
		DbgLog("        block[%d]", newBlock.nTimeStamp);
		DbgLog("        prev block hash: 0x%X", newBlock.hashPrev);
		DbgLog("        current block hash: 0x%X", newBlock.hashTx);
		DbgLog("        transaction count: %d", newBlock.nTxCount);
		DbgLog("        cipher of hash: 0x%llX", newBlock.cipherTx);
		DbgLog("        *** random number:%f ***", dblRandom);
	}
	else
	{
		DbgLog("\n");
		DbgLog("send new block[%d]", newBlock.nTimeStamp);
		DbgLog("        prev block hash: 0x%X", newBlock.hashPrev);
		DbgLog("        current block hash: 0x%X", newBlock.hashTx);
		DbgLog("        transaction count: %d", newBlock.nTxCount);
		DbgLog("        cipher of hash: 0x%llX", newBlock.cipherTx);
		DbgLog("        random number: %f", dblRandom);
	}
	BroadcastLastBlock(true);

	return newBlock.nTimeStamp;
}

void	CBlockChainMiner::OnIdle()
{
	CBlockChainBase::OnIdle();
	BroadcastServerIP();
	Mine(false);
}

void CBlockChainMiner::BroadcastServerIP()
{
	static std::chrono::system_clock::time_point tp;
	std::chrono::system_clock::time_point tpNow = std::chrono::system_clock::now();
	std::chrono::duration<double> sec = tpNow - tp;

	if (sec.count() > 5.0f)
	{
		//DbgLog("[CBlockChainMiner::BroadcastServerIP] ip:%S", m_txServer.m_szServerIP);
		tp = tpNow;
		SBlockChainMsg	msg;
		msg.kind = BCMK_RESPONSE_SERVER_IP;
		strcpy_s((char*)msg.param, 32, m_txServer.m_szServerIP);
		m_qSendBuffer.push(msg);
	}
}

void CBlockChainMiner::OnRequestRandom(SBlockChainMsg& msg_)
{
	//DbgLog("[CBlockChainBase::OnRequestRandom] begin");
	Mine(true);

	//DbgLog("[CBlockChainBase::OnRequestRandom] end");
}
