// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <CryReflection/Framework.h>

class CVault
{
public:
	static void ReflectVault(Cry::Reflection::ITypeDesc& typeDesc);

	CVault();
	~CVault();
	CVault(const CVault& other);

	bool Serialize(Serialization::IArchive& archive);
	string ToString() const;

	CVault& operator=(CVault&& other);
	CVault& operator=(const CVault& other);
	bool operator==(const CVault& other) const;

public:
	char   m_legacy[17];
	uint64 m_public;

private:
	char   m_garbage[7];
	uint64 m_secret;
};

enum EBankType
{
	eBankType_Retail = 3,
	eBankType_Buisness = 7,
	eBankType_Investment = 9
};

class CBank : public CVault
{
public:
	static void ReflectBank(Cry::Reflection::ITypeDesc& typeDesc);

	CBank();
	~CBank();
	CBank(CBank&& other);
	CBank(const CBank& other);

	bool Serialize(Serialization::IArchive& archive);
	string ToString() const;

	void    SetAccount(int64 account) { m_account = account; }
	int64_t GetAccount() const { return m_account; }

	CBank& operator=(const CBank& other);
	CBank& operator=(CBank&& other);
	bool operator==(const CBank& other) const;

public:
	EBankType           m_type;
	static const size_t s_documentCount = 3;
private:
	char                m_documents[CBank::s_documentCount];
	int64               m_account;
};
