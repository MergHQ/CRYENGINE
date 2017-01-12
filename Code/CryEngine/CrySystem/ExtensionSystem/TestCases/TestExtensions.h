// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

//#define EXTENSION_SYSTEM_INCLUDE_TESTCASES

#ifdef EXTENSION_SYSTEM_INCLUDE_TESTCASES

	#include <CryExtension/ICryUnknown.h>

struct ICryFactoryRegistryImpl;

void TestExtensions(ICryFactoryRegistryImpl* pReg);

struct IFoobar : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IFoobar, 0x539e9c672cad4a03, 0x9ecd8069c99a846b);

	virtual void Foo() = 0;
};

DECLARE_SHARED_POINTERS(IFoobar);

struct IRaboof : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IRaboof, 0x135ca25e634b4d13, 0x9e4467968a708822);

	virtual void Rab() = 0;
};

DECLARE_SHARED_POINTERS(IRaboof);

struct IA : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IA, 0xd93aaceb35ec427e, 0xb64bf8dec4997e67);

	virtual void A() = 0;
};

DECLARE_SHARED_POINTERS(IA);

struct IB : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IB, 0xe0d830c826424e11, 0x9eacfa19eaf31ffb);

	virtual void B() = 0;
};

DECLARE_SHARED_POINTERS(IB);

struct IC : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IC, 0x577509a20fc5477c, 0x893757c9ca88b27b);

	virtual void C() = 0;
};

DECLARE_SHARED_POINTERS(IC);

struct ICustomC : public IC
{
	CRYINTERFACE_DECLARE(ICustomC, 0x2ac769da4c7443bf, 0x80911033e21dfbcf);

	virtual void C1() = 0;
};

DECLARE_SHARED_POINTERS(ICustomC);

//////////////////////////////////////////////////////////////////////////
// use of extension system without any of the helper macros/templates

struct IDontLikeMacros : public ICryUnknown
{
	template<class T> friend constexpr CryInterfaceID InterfaceCastSemantics::cryiidof();
protected:
	virtual ~IDontLikeMacros() {}

private:
	// It's very important that this static function is implemented for each interface!
	// Otherwise the consistency of cryinterface_cast<T>() is compromised because
	// cryiidof<T>() = cryiidof<baseof<T>>() {baseof<T> = ICryUnknown in most cases}
	static constexpr CryInterfaceID IID()
	{
		return CryInterfaceID::Construct( 0x0f43b7e3f1364af0ull, 0xb4a16a975bea3ec4ull );
	}

public:
	virtual void CallMe() = 0;
};

DECLARE_SHARED_POINTERS(IDontLikeMacros);

#endif // #ifdef EXTENSION_SYSTEM_INCLUDE_TESTCASES
