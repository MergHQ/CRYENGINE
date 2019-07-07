// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
struct ILipSyncPhonemeRecognizer
{
public:
	struct SPhoneme
	{
		enum { MAX_PHONEME_LENGTH = 8 };

		int   startTime;
		int   endTime;
		int   nPhonemeCode;
		char  sPhoneme[MAX_PHONEME_LENGTH];
		float intensity;
	};

	struct SWord
	{
		int   startTime;
		int   endTime;
		char* sWord;
	};

	struct SSentance
	{
		char*     sSentence;

		int       nWordCount;
		SWord*    pWords; // Array of words.

		int       nPhonemeCount;
		SPhoneme* pPhonemes; // Array of phonemes.
	};

	virtual ~ILipSyncPhonemeRecognizer() {}
	virtual void        Release() = 0;
	virtual bool        RecognizePhonemes(const char* wavfile, const char* text, SSentance** pOutSetence) = 0;
	virtual const char* GetLastError() = 0;
};

//////////////////////////////////////////////////////////////////////////
class CPhonemesAnalyzer
{
public:
	CPhonemesAnalyzer();
	~CPhonemesAnalyzer();

	// Analyze wav file and extract phonemes out of it.
	bool        Analyze(const char* wavfile, const char* text, ILipSyncPhonemeRecognizer::SSentance** pOutSetence);
	const char* GetLastError();

private:
	HMODULE                    m_hDLL;
	ILipSyncPhonemeRecognizer* m_pPhonemeParser;
	string                     m_LastError;
};
