// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// This file contains various helper classes to maintain state of cipher/hmac for various implementations


#if ENCRYPTION_RIJNDAEL
#include "Cryptography/rijndael.h"
#endif
#if ENCRYPTION_STREAMCIPHER
#include "Cryptography/StreamCipher.h"
#endif
#if ENCRYPTION_CNG_AES || HMAC_CNG_SHA256
#include "Cryptography/CngCrypto.h"
#endif
#if ENCRYPTION_TOMCRYPT_AES || HMAC_TOMCRYPT_SHA256
#include "Cryptography/TomCryptCrypto.h"
#endif

#include "Services/NetworkACL/NetProfileTokens.h"


#if !defined(RELEASE)
#define DEBUG_HMAC_THREAD_ACCESS 1
#endif

namespace ChannelSecurity
{
	//! Cipher for channel packet data encryption.
	//! Selects actual implementation depending on configuration flags.
	class CCipher
	{
	public:
		using TCipherImpl =
#if ENCRYPTION_RIJNDAEL
			Rijndael
#elif ENCRYPTION_STREAMCIPHER
			CStreamCipher
#elif ENCRYPTION_CNG_AES
			CCngAesCipher
#elif ENCRYPTION_TOMCRYPT_AES
			CTomCryptAesCipher
#else
			struct SNullCipher
			{
				static uint32 GetBlockSize() { return 1; }
			}
#endif
			;
	public:
		static uint32 GetBlockSize();

		bool InitForInput(const CNetProfileTokens::SToken& token);
		bool InitForOutput(const CNetProfileTokens::SToken& token);

		bool EncryptInPlace(uint8* pBuf, const size_t bufSize);
		bool DecryptInPlace(uint8* pBuf, const size_t bufSize);

	private:
		template <bool isInput>
		bool Init(const CNetProfileTokens::SToken& token);

	private:
		TCipherImpl m_impl;
	};

	//! HMAC for channel packet data authentication/validation.
	//! Selects actual implementation depending on configuration flags.
	class CHMac
	{
	public:
		using HashResult =
#if HMAC_CNG_SHA256
			CCngSha256Hmac::HashResult
#elif HMAC_TOMCRYPT_SHA256
			CTomCryptSha256Hmac::HashResult
#else
			std::array<uint8, 0>
#endif
			;

		enum { HashSize = std::tuple_size<HashResult>::value };

	public:
		bool Init(const CNetProfileTokens::SToken& token);

		// Individual operations
		bool Hash(const uint8* pBuf, const size_t bufSize);
		bool Finish(HashResult& result);
		bool Finish(uint8* pResult);
		static bool Verify(const HashResult& expected, const HashResult& actual);

		// Composite operations
		bool HashAndFinish(const uint8* pBuf, const size_t bufSize, HashResult& result);
		bool HashAndFinish(const uint8* pBuf, const size_t bufSize, uint8* pResult);

		bool HashFinishAndVerify(const uint8* pBuf, const size_t bufSize, const HashResult& expectedResult);
		bool HashFinishAndVerify(const uint8* pBuf, const size_t bufSize, const uint8* pExpectedResult);

		//! Initialize token with dummy secret to support Legacy profile tokens mode.
		static void FillDummyToken(CNetProfileTokens::SToken& token);
	private:
#if HMAC_CNG_SHA256
		CCngSha256HmacSharedPtr m_pHmac;
#elif HMAC_TOMCRYPT_SHA256
		CTomCryptSha256HmacSharedPtr m_pHmac;
#endif

#if DEBUG_HMAC_THREAD_ACCESS
		volatile threadID m_debugThreadId = THREADID_NULL;
#endif
	};

	//! Channel init state of ciphers and HMAC
	struct SInitState
	{
		SInitState(CNetProfileTokens::SToken& token, const CHMac& hmac, bool& outRes);
		
		CCipher inputCipher;
		CCipher outputCipher;
		CHMac hmac;
		uint32 frameSequenceStart;
	};
}

