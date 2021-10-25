#pragma once
#include "type.h"
#include <fstream>
#include <mutex>

static const char*	RCNT_DB_RECORD_PRIFIX = "RCNT_DB_RECORD_";//length 15+1
static const char*	RNCT_DB_NAME = "bnct.db";
static const char*	RNCT_DB_FILE = "RNCT_DB_FILE";

//unit that stored in key-value database.
//size must be equals to the size of physical disk's sector.
//size:512 bytes
struct SRecord
{
	char	key[32];
	uint8	value[480];
};

static const uint32		RNCT_DB_TABLE_SIZE = 500;
static const uint32		RNCT_DB_RECORDS_MAX = 500*8;
static const uint32		RNCT_DB_FILE_SIZE = (1 + RNCT_DB_RECORDS_MAX) * 512;
//size must be equals to the size of physical disk's sector.
//one bit of table represents that a record is used.
//maximum count of records is TABLE_SIZE * 8.
//size:512 bytes
struct SRecordAllocationTable
{
	char	sig[12] = {'R', 'N', 'C', 'T', '_', 'R', 'A', 'T', '_', '1', '2', '3'};
	uint8	table[RNCT_DB_TABLE_SIZE];	//primary record table
	SRecordAllocationTable()
	{
		memcpy(sig, RNCT_DB_FILE, strlen(RNCT_DB_FILE));
		memset(table, 0, sizeof(table));
	}
};

#define GET_BIT0(u8)	(((uint8)u8 & 1) != 0)
#define GET_BIT1(u8)	(((uint8)u8 & 2) != 0)
#define GET_BIT2(u8)	(((uint8)u8 & 4) != 0)
#define GET_BIT3(u8)	(((uint8)u8 & 8) != 0)
#define GET_BIT4(u8)	(((uint8)u8 & 16) != 0)
#define GET_BIT5(u8)	(((uint8)u8 & 32) != 0)
#define GET_BIT6(u8)	(((uint8)u8 & 64) != 0)
#define GET_BIT7(u8)	(((uint8)u8 & 128) != 0)
#define GET_BITn(u8, n)	(((uint8)u8 & (1<<n)) != 0)

#define SET_BIT0(u8)	(u8 |= 1)
#define SET_BIT1(u8)	(u8 |= 2)
#define SET_BIT2(u8)	(u8 |= 4)
#define SET_BIT3(u8)	(u8 |= 8)
#define SET_BIT4(u8)	(u8 |= 16)
#define SET_BIT5(u8)	(u8 |= 32)
#define SET_BIT6(u8)	(u8 |= 64)
#define SET_BIT7(u8)	(u8 |= 128)
#define SET_BITn(u8, n)	(u8 |= (1<<n))

//this is a bridge between memory and file and stores data in key-value type record units.
class CDB
{
public:
	CDB() {}
	virtual ~CDB() {}
public:

	//search and get the record which has key_ as its key.
	//when success returns record number on table, -1 means fail.
	int32 Get(SRecord& record_, const char* key_)
	{
		//DbgLog("[CDB::Get] key:%s", key_);

		if (Verify() == false)
			return -1;

		FILE*	f;
		fopen_s(&f, m_name, "rb");
		SRecordAllocationTable	rat;
		fread(&rat, sizeof(rat), 1, f);

		for (int i = 0; i < RNCT_DB_TABLE_SIZE; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				if (GET_BITn(rat.table[i], j) == true)
				{
					SRecord		tempRecord;
					fseek(f, sizeof(SRecordAllocationTable) + sizeof(SRecord)*(i * 8 + j), SEEK_SET);
					fread(&tempRecord, sizeof(tempRecord), 1, f);
					if (strcmp(key_, tempRecord.key) == 0)
					{
						record_ = tempRecord;
						fclose(f);

						//DbgLog("[CDB::Get] record:%d", i * 8 + j);
						return i * 8 + j;
					}
				}
			}
		}

		fclose(f);

		//DbgLog("[CDB::Get] no record");
		return -1;
	}

	bool Put(const SRecord& record_)
	{
		//DbgLog("[CDB::Put] record:%s", record_.key);

		if (Verify() == false)
			return false;


		SRecord		tempRecord;
		int32	n = Get(tempRecord, record_.key);

		FILE*	f;
		fopen_s(&f, m_name, "r+b");

		if (n >= 0)
		{
			tempRecord = record_;
			fseek(f, sizeof(SRecordAllocationTable) + sizeof(SRecord)*(n), SEEK_SET);
			fwrite(&tempRecord, sizeof(tempRecord), 1, f);
			fclose(f);

			//DbgLog("[CDB::Put] record:%s modified.", record_.key);
			return true;
		}

		SRecordAllocationTable	rat;
		fseek(f, 0, SEEK_SET);
		fread(&rat, sizeof(rat), 1, f);

		for (int i = 0; i < RNCT_DB_TABLE_SIZE; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				if (GET_BITn(rat.table[i], j) == false)
				{
					tempRecord = record_;
					fseek(f, sizeof(SRecordAllocationTable) + sizeof(SRecord)*(i * 8 + j), SEEK_SET);
					fwrite(&tempRecord, sizeof(tempRecord), 1, f);
					SET_BITn(rat.table[i], j);
					fseek(f, 0, SEEK_SET);
					fwrite(&rat, sizeof(rat), 1, f);
					fclose(f);

					//DbgLog("[CDB::Put] record:%S added at:%d", record_.key, i*8 + j);
					return true;
				}
			}
		}

		fclose(f);

		//DbgLog("[CDB::Put] fail.");
		return false;
	}

private:
	//
	bool Verify()
	{
		FILE*	f;
		fopen_s(&f, m_name, "r+b");

		if (f == nullptr)
		{
			//DbgLog("[CDB::Verify] file does not exist, try create new file.");
			fopen_s(&f, m_name, "w+b");
			if (f == nullptr)
			{
				//DbgLog("[CDB::Verify] file can not be open to create, return false");
				return false;
			}
		}
		fseek(f, 0, SEEK_END);
		auto s = ftell(f);
		//DbgLog("[CDB::Verify] file size:%d", s);

		bool	flagRebuild = false;

		if (s != RNCT_DB_FILE_SIZE)
		{
			//DbgLog("[CDB::Verify] file size is mismatch, try rebuild it.");
			flagRebuild = true;
		}
		else
		{
			SRecordAllocationTable	tempRat;
			fseek(f, 0, SEEK_SET);
			fread(&tempRat, sizeof(tempRat), 1, f);
			if (memcmp(tempRat.sig, "RNCT_DB_FILE", strlen(RNCT_DB_FILE)) != 0)
			{
				//DbgLog("[CDB::Verify] file sig mismatch, try rebuild it.");
				flagRebuild = true;
			}
		}

		if (flagRebuild)
		{
			fseek(f, 0, SEEK_SET);
			SRecordAllocationTable	tempRat;
			fwrite(&tempRat, sizeof(tempRat), 1, f);
			char	szTemp[512];
			memset(szTemp, 0, 512);
			for (int i=0; i< RNCT_DB_RECORDS_MAX; i++)
			{
				fwrite(szTemp, 512, 1, f);
			}
		}

		fclose(f);
		//DbgLog("[CDB::Verify] file verification ok, return true");
		return true;
	}

private:
	const char*				m_name = RNCT_DB_NAME;
//	std::mutex				m_mutex;
};
