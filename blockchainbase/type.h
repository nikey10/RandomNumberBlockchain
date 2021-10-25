#pragma once

using uint8  = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned long;
using uint64 = unsigned long long;

using int8  = signed char;
using int16 = signed short;
using int32 = signed long;
using int64 = signed long long;


static const char*	RNCT_TX_SERVER_ADDR = "127.0.0.1";
static const char*	RNCT_TX_ACK_CONN = "RNCT_TX_ACK_CONN";
static const char*	RNCT_TX_ACK_00 = "RNCT_TX_ACK_00";
static const char*	RNCT_TX_REQ_00 = "RNCT_TX_REQ_00";
static const uint16		RNCT_TX_SERVER_PORT = (uint16)12701;
static const uint32		RNCT_TX_MAX_COUNT = 25;

constexpr int	OTP_LENGTH = 4;
constexpr int	MAX_OTP_COUNTER = 10;

//definition of OTP(One-Time-Password) based transaction.
//why use OTP concept, the reason is that, someone who knows current password can not guess next password.
//so, our transaction will not be hacked by someone.
//size:16 bytes
struct STransaction
{
	char	token[OTP_LENGTH];
	char	otpString[OTP_LENGTH];
	uint32	otp;
	uint32	counter;
};

static const uint32		RNCT_BLK_GENESIS_HASH = 0x20190728;
static const uint32		RNCT_BLK_GENESIS_TIMESTAMP = 0;  //20190728;
static const uint32		RNCT_BLK_ROOTHASH_INIT = 0x88776655;


//definition of primary data structure of system, the block.
//size:424 bytes(up to 480 bytes)
struct SBlock
{
	uint32			nTimeStamp = 0;
	uint32			hashPrev = 0;
	uint32			nTxCount = 0;
	uint32			hashTx = 0;
	uint64			cipherTx = 0;
	STransaction	tx[RNCT_TX_MAX_COUNT];	//size:16*25=400 bytes
};

typedef void(*logfunc) (const char* format_, ...);
extern logfunc DbgLog;
