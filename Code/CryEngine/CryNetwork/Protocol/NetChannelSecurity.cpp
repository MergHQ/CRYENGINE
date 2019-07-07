// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "NetChannelSecurity.h"

namespace ChannelSecurity
{
	//////////////////////////////////////////////////////////////////////////
	// CCipher
	//////////////////////////////////////////////////////////////////////////

	uint32 CCipher::GetBlockSize()
	{
		return TCipherImpl::GetBlockSize();
	}

	bool CCipher::InitForInput(const CNetProfileTokens::SToken& token)
	{
		return Init<true>(token);
	}

	bool CCipher::InitForOutput(const CNetProfileTokens::SToken& token)
	{
		return Init<false>(token);
	}

	template <bool isInput>
	bool CCipher::Init(const CNetProfileTokens::SToken& token)
	{
#if ALLOW_ENCRYPTION
		const CNetProfileTokens::SToken::SEncryptionToken& encToken = isInput
			? token.inputEncryption
			: token.outputEncryption;

#if ENCRYPTION_RIJNDAEL
		{
			const Rijndael::Direction dir = isInput ? Rijndael::Decrypt : Rijndael::Encrypt;
			const uint8* key = encToken.secret.data();
			const size_t keySize = encToken.secret.size();
			uint8* initVec = nullptr;

			Rijndael::KeyLength keyLength;
			switch (keySize)
			{
			case 16: keyLength = Rijndael::Key16Bytes; break;
			case 24: keyLength = Rijndael::Key24Bytes; break;
			case 32: keyLength = Rijndael::Key32Bytes; break;
			default: CryFatalError("Wrong Rijndael KeyLength");
			}
			int res = m_impl.init(Rijndael::CBC, dir, key, keyLength, initVec);
			return res == RIJNDAEL_SUCCESS;
		}
#elif ENCRYPTION_STREAMCIPHER
		{
			const uint8* key = encToken.secret.data();
			int keyLen = encToken.secret.size();

			m_impl.Init(key, keyLen);
			return true;
		}
#elif ENCRYPTION_CNG_AES
		{
			if (encToken.secret.size() < 16)
			{
				CryFatalError("Secret is too small");
			}

			SCngAesInitState state;
			state.key = CCngKey::CreateAesKeyFromSecret(encToken.secret.data(), encToken.secret.size());
			if (!state.key)
				return false;

			auto setIV = [](SCngAesInitState& state, const DynArray<uint8>& iv)
			{
				const size_t availabieIvSize = std::min<size_t>(state.initVec.size(), iv.size());
				const size_t zeroIvSize = state.initVec.size() - availabieIvSize;
				::memcpy(state.initVec.data(), iv.data(), availabieIvSize);
				::memset(state.initVec.data(), 0, zeroIvSize);
			};

			setIV(state, encToken.initVector);

			m_impl.Init(state);
			return true;
		}
#elif ENCRYPTION_TOMCRYPT_AES
		{
			if (encToken.secret.size() < 16)
			{
				CryFatalError("Secret is too small");
			}

			STomCryptAesInitState state;
			state.pState = CTomCryptAesState::CreateAesStateFromSecret(encToken.secret.data(), encToken.secret.size());
			if (!state.pState)
				return false;

			auto setIV = [](STomCryptAesInitState& state, const DynArray<uint8>& iv)
			{
				const size_t availabieIvSize = std::min<size_t>(state.initVec.size(), iv.size());
				const size_t zeroIvSize = state.initVec.size() - availabieIvSize;
				::memcpy(state.initVec.data(), iv.data(), availabieIvSize);
				::memset(state.initVec.data(), 0, zeroIvSize);
			};

			setIV(state, encToken.initVector);

			m_impl.Init(state);
			return true;
		}
#else
#error "Unknown configuration"
		return false;
#endif

#else
		return true;
#endif // ALLOW_ENCRYPTION
	}


	bool CCipher::EncryptInPlace(uint8* pBuf, const size_t len)
	{
#if ALLOW_ENCRYPTION
#if ENCRYPTION_RIJNDAEL
		// cppcheck-suppress allocaCalled
		PREFAST_SUPPRESS_WARNING(6255) uint8 * buf = (uint8*)alloca(len);
		NET_ASSERT(0 == (len & 15));
		const int res = m_impl.blockEncrypt(pBuf, len * 8, buf);
		memcpy(pBuf, buf, len);
		return (res >= 0);
#elif ENCRYPTION_STREAMCIPHER
		m_impl.Encrypt(pBuf, len, pBuf);
		return true;
#elif ENCRYPTION_CNG_AES
		return m_impl.EncryptInPlace(pBuf, len);
#elif ENCRYPTION_TOMCRYPT_AES
		return m_impl.EncryptInPlace(pBuf, len);
#else
#error "Unknown configuration"
#endif
#endif
		return true;
	}

	bool CCipher::DecryptInPlace(uint8* pBuf, const size_t len)
	{
#if ALLOW_ENCRYPTION
#if ENCRYPTION_RIJNDAEL
		// cppcheck-suppress allocaCalled
		PREFAST_SUPPRESS_WARNING(6255) uint8 * buf = (uint8*)alloca(len);
		NET_ASSERT(0 == (len & 15));
		const int res = m_impl.blockDecrypt(pBuf, len * 8, buf);
		memcpy(pBuf, buf, len);
		return (res >= 0);
#elif ENCRYPTION_STREAMCIPHER
		m_impl.Decrypt(pBuf, len, pBuf);
		return true;
#elif ENCRYPTION_CNG_AES
		return m_impl.DecryptInPlace(pBuf, len);
#elif ENCRYPTION_TOMCRYPT_AES
		return m_impl.DecryptInPlace(pBuf, len);
#else
#error "Unknown configuration"
#endif
#endif
		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// CHMac
	//////////////////////////////////////////////////////////////////////////

	void CHMac::FillDummyToken(CNetProfileTokens::SToken& token)
	{
#if ALLOW_HMAC
		token.hmacSecret.resize(4, eZeroInit);
#endif
	}
	
	bool CHMac::Init(const CNetProfileTokens::SToken& token)
	{
#if ALLOW_HMAC
		const uint8* pSecret = token.hmacSecret.data();
		size_t size = token.hmacSecret.size();

#if HMAC_CNG_SHA256
		m_pHmac = CCngSha256Hmac::CreateFromSecret(pSecret, size);
#elif HMAC_TOMCRYPT_SHA256
		m_pHmac = CTomCryptSha256Hmac::CreateFromSecret(pSecret, size);
#else
#error "Unknown configuration"
#endif
		return (m_pHmac != nullptr);
#endif
		return true;
	}

	bool CHMac::Hash(const uint8* pBuf, const size_t bufSize)
	{
#if DEBUG_HMAC_THREAD_ACCESS
		if (m_debugThreadId == THREADID_NULL)
		{
			m_debugThreadId = GetCurrentThreadId();
		}
		else
		{
			CRY_ASSERT(GetCurrentThreadId() == m_debugThreadId);
		}
#endif

#if ALLOW_HMAC
#if HMAC_CNG_SHA256 || HMAC_TOMCRYPT_SHA256
		CRY_ASSERT(m_pHmac, "HMAC is not initialized");
		return m_pHmac->Hash(pBuf, bufSize);
#else
#error "Unknown configuration"
#endif
#endif
		return true;
	}

	bool CHMac::Finish(HashResult& result)
	{
#if DEBUG_HMAC_THREAD_ACCESS
		CRY_ASSERT(GetCurrentThreadId() == m_debugThreadId);
#endif

#if ALLOW_HMAC
#if HMAC_CNG_SHA256 || HMAC_TOMCRYPT_SHA256
		CRY_ASSERT(m_pHmac, "HMAC is not initialized");
		bool res = m_pHmac->FinishAndRestartHash(result);
#else
#error "Unknown configuration"
#endif
#else
		bool res = true;
#endif

#if DEBUG_HMAC_THREAD_ACCESS
		m_debugThreadId = THREADID_NULL;
#endif
		return res;
	}

	bool CHMac::Finish(uint8* pResult)
	{
		return Finish(*reinterpret_cast<HashResult*>(pResult));
	}

	bool CHMac::HashAndFinish(const uint8* pBuf, const size_t bufSize, HashResult& result)
	{
		// Call Finish() even if Hash() fails to leave it in correct state
		const bool hashRes = Hash(pBuf, bufSize);
		const bool finishRes = Finish(result);
		return hashRes && finishRes;
	}

	bool CHMac::HashAndFinish(const uint8* pBuf, const size_t bufSize, uint8* pResult)
	{
		return HashAndFinish(pBuf, bufSize, *reinterpret_cast<HashResult*>(pResult));
	}

	bool CHMac::HashFinishAndVerify(const uint8* pBuf, const size_t bufSize, const HashResult& expectedResult)
	{
		ChannelSecurity::CHMac::HashResult actualHash;
		bool hmacSucceeded = HashAndFinish(pBuf, bufSize, actualHash);
		hmacSucceeded = hmacSucceeded && ChannelSecurity::CHMac::Verify(expectedResult, actualHash);
		return hmacSucceeded;
	}

	bool CHMac::HashFinishAndVerify(const uint8* pBuf, const size_t bufSize, const uint8* pExpectedResult)
	{
		return HashFinishAndVerify(pBuf, bufSize, *reinterpret_cast<const HashResult*>(pExpectedResult));
	}

	bool CHMac::Verify(const HashResult& expected, const HashResult& actual)
	{
		return expected == actual;
	}

	//////////////////////////////////////////////////////////////////////////
	// SInitState
	//////////////////////////////////////////////////////////////////////////

	SInitState::SInitState(CNetProfileTokens::SToken& token, const CHMac& hmac, bool& outRes)
		: hmac(hmac)
		, frameSequenceStart(token.frameSequenceStart)
	{
		if (!inputCipher.InitForInput(token))
		{
			NetWarning("Failed to initialize input cipher");
			outRes = false;
			return;
		}

		if (!outputCipher.InitForOutput(token))
		{
			NetWarning("Failed to initialize input cipher");
			outRes = false;
			return;
		}

		outRes = true;
	}
}
