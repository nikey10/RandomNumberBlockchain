#pragma once
#include "type.h"


namespace crypto
{
	//calculate new OTP in format 4 digits(* * * *).
	static uint32 otp4(const char* prev_, const uint32 counter_, char* next_)
	{
		uint32 newOtp = 0;
		char	newOtpString[OTP_LENGTH];
		for (int i = 0; i < OTP_LENGTH; i++)
		{
			char	c = (prev_[i] - '0' + counter_) % 10;
			newOtpString[i] = c + '0';

			for (int j = 0; j < OTP_LENGTH - 1 - i; j++)
				c *= 10;

			newOtp += c;
		}
		if (newOtp < 1000)
		{
			newOtp += 1000;
			newOtpString[0] = '1';
		}

		if (next_ != nullptr)
			memcpy(next_, newOtpString, OTP_LENGTH);

		return newOtp;
	}

	//calculate 32bit HASH.
	static uint32	hash32(const uint8* buffer_, uint32 len_)
	{
		uint32	hash = 0;
		if (buffer_ != nullptr)
		{
			for (uint32 i = 0; i < len_; i++)
			{
				hash = buffer_[i] + (hash << 6) + (hash << 16) - hash;
			}
		}
		return hash;
	}

	//calculate 32bit CRC.
	static uint32 crc32(const uint8* buffer_, uint32 len)
	{
		uint32	reg = 0xFFFFFFFF;

		if (buffer_ != nullptr)
		{
			for (uint32 i=0; i<len; i++)
			{
				reg ^= buffer_[i];
				for (uint32 j=0; j<8; j++)
				{
					uint32 tmp = reg & 0x01;
					reg >>= 1;
					if (tmp)
					{
						reg ^= 0xEBD88320;
					}
				}
			}
		}

		return reg;
	}

	// RSA Public Key Cipher
	const uint32 p = 17;
	const uint32 q = 23;
	const uint32 n = p * q;//391
	const uint32 fn = (p - 1)*(q - 1);//352
	const uint32 e = 3;
	const uint32 d = 235;


	static uint16 rsa_enc8(const uint8 m)
	{
		uint32 p = 1;
		for (uint32 i = 0; i < e; i++)
		{
			p *= m;
			p %= n;
		}

		return (uint16)p;
	}

	static uint8 rsa_dec8(const uint16 c)
	{
		uint32 p = 1;
		for (uint32 i = 0; i < d; i++)
		{
			p *= c;
			p %= n;
		}

		return (uint8)p;
	}

	static void rsa_enc8_buffer(const uint8* m, uint16* c, uint32 len)
	{
		if (m != 0 && c != 0)
		{
			for (uint32 i = 0; i < len; i++)
			{
				c[i] = rsa_enc8(m[i]);
			}
		}
	}

	static void rsa_dec8_buffer(const uint16* cry, uint8* msg, uint32 len)
	{
		if (cry != 0 && msg != 0)
		{
			for (uint32 i = 0; i < len; i++)
			{
				msg[i] = rsa_dec8(cry[i]);
			}
		}
	}

	static double normal_random(uint64 seed_)
	{
		double dbl = (double)seed_;
		dbl /= 7777.7777f;
		dbl -= floor(dbl);
		dbl *= 100000000;
		int32 n = (int32)dbl;
		n %= 1000000;
		dbl = n;
		dbl /= 1000000.0f;
		return dbl;
	}

}//end namespace crypto

//////////////////////////////////////////////////////////////////////////
