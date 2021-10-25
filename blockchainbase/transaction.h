#pragma once
#include "type.h"
#include "crypto.h"
#include <string.h>
#include <math.h>
#include <thread>

//a server/client connection has one active transaction, called an object of class CActiveTransaction.
//its core data is STransaction.
class CActiveTransaction : protected STransaction
{
public:
	CActiveTransaction() { Reset(); }
	virtual ~CActiveTransaction()
	{
		Stop();
	}

	//stop main thread.
	void Stop()
	{
		m_isRunable = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
protected:
	//make all data of transaction to zero.
	void Reset()
	{
		memset(token, 0, sizeof(token));
		otp = 0;
		memset(otpString, 0, sizeof(otpString));
		counter = 0;
	}

	//make next OTP by using inversed method.
	//the i(th) password will calculated by calling function CalcOtp() n-i times.
	void NextOtp()
	{
		counter++;

		char*	prev = token;
		uint32	newOtp;
		char	newOtpString[OTP_LENGTH];

		for (uint32 i=0; i< MAX_OTP_COUNTER-counter+1; i++)
		{
			newOtp = crypto::otp4(prev, counter, newOtpString);
			prev = newOtpString;
		}

		otp = newOtp;
		memcpy(otpString, newOtpString, sizeof(otpString));
	}
public:
	//get copy of core data of a transaction
	STransaction	data()
	{
		STransaction	tx;
		memcpy(tx.token, token, sizeof(token));
		tx.otp = otp;
		memcpy(tx.otpString, otpString, sizeof(otpString));
		tx.counter = counter;
		return tx;
	}

	//configure new transaction data from another.
	void fromData(const STransaction& tx)
	{
		memcpy(token, tx.token, sizeof(token));
		otp = tx.otp;
		memcpy(otpString, tx.otpString, sizeof(otpString));
		counter = tx.counter;
	}

	//close its connection and finalize.
	void Close()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		Reset();
	}
public:
	uint32	m_sock = INVALID_SOCKET;	//a network socket of connection.
protected:
	std::thread	m_thread;					//main network thread.
	bool	m_isRunable = false;		//switching flag of thread, when this flag is set as false, thread will terminated.
};
