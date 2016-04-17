// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   ICryUnknown.h
//  Version:     v1.00
//  Created:     02/25/2009 by CarstenW
//  Description: Part of CryEngine's extension framework.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef _ICRYUNKNOWN_H_
#define _ICRYUNKNOWN_H_

#pragma once

#include "CryTypeID.h"

struct ICryFactory;
struct ICryUnknown;

namespace InterfaceCastSemantics
{

template<class T>
const CryInterfaceID& cryiidof()
{
	return T::IID();
}

#define _BEFRIEND_CRYIIDOF() \
  template<class T> friend const CryInterfaceID &InterfaceCastSemantics::cryiidof();

template<class Dst, class Src>
Dst* cryinterface_cast(Src* p)
{
	return static_cast<Dst*>(p ? p->QueryInterface(cryiidof<Dst>()) : 0);
}

template<class Dst, class Src>
Dst* cryinterface_cast(const Src* p)
{
	return static_cast<const Dst*>(p ? p->QueryInterface(cryiidof<Dst>()) : 0);
}

namespace Internal
{
template<class Dst, class Src>
struct cryinterface_cast_helper
{
	static std::shared_ptr<Dst> Op(const std::shared_ptr<Src>& p)
	{
		Dst* dp = cryinterface_cast<Dst>(p.get());
		return dp ? std::shared_ptr<Dst>(p, dp) : std::shared_ptr<Dst>();
	}
};

template<class Src>
struct cryinterface_cast_helper<ICryUnknown, Src>
{
	static std::shared_ptr<ICryUnknown> Op(const std::shared_ptr<Src>& p)
	{
		ICryUnknown* dp = cryinterface_cast<ICryUnknown>(p.get());
		return dp ? std::shared_ptr<ICryUnknown>(*((const std::shared_ptr<ICryUnknown>*) & p), dp) : std::shared_ptr<ICryUnknown>();
	}
};

template<class Src>
struct cryinterface_cast_helper<const ICryUnknown, Src>
{
	static std::shared_ptr<const ICryUnknown> Op(const std::shared_ptr<Src>& p)
	{
		const ICryUnknown* dp = cryinterface_cast<const ICryUnknown>(p.get());
		return dp ? std::shared_ptr<const ICryUnknown>(*((const std::shared_ptr<const ICryUnknown>*) & p), dp) : std::shared_ptr<const ICryUnknown>();
	}
};
}

template<class Dst, class Src>
std::shared_ptr<Dst> cryinterface_cast(const std::shared_ptr<Src>& p)
{
	return Internal::cryinterface_cast_helper<Dst, Src>::Op(p);
}

#define _BEFRIEND_CRYINTERFACE_CAST()                                                                \
  template<class Dst, class Src> friend Dst * InterfaceCastSemantics::cryinterface_cast(Src*);       \
  template<class Dst, class Src> friend Dst * InterfaceCastSemantics::cryinterface_cast(const Src*); \
  template<class Dst, class Src> friend std::shared_ptr<Dst> InterfaceCastSemantics::cryinterface_cast(const std::shared_ptr<Src> &);

}

using InterfaceCastSemantics::cryiidof;
using InterfaceCastSemantics::cryinterface_cast;

template<class S, class T>
bool CryIsSameClassInstance(S* p0, T* p1)
{
	return static_cast<const void*>(p0) == static_cast<const void*>(p1) || cryinterface_cast<const ICryUnknown>(p0) == cryinterface_cast<const ICryUnknown>(p1);
}

template<class S, class T>
bool CryIsSameClassInstance(const std::shared_ptr<S>& p0, T* p1)
{
	return CryIsSameClassInstance(p0.get(), p1);
}

template<class S, class T>
bool CryIsSameClassInstance(S* p0, const std::shared_ptr<T>& p1)
{
	return CryIsSameClassInstance(p0, p1.get());
}

template<class S, class T>
bool CryIsSameClassInstance(const std::shared_ptr<S>& p0, const std::shared_ptr<T>& p1)
{
	return CryIsSameClassInstance(p0.get(), p1.get());
}

namespace CompositeQuerySemantics
{

template<class Src>
std::shared_ptr<ICryUnknown> crycomposite_query(Src* p, const char* name, bool* pExposed = 0)
{
	void* pComposite = p ? p->QueryComposite(name) : 0;
	pExposed ? *pExposed = pComposite != 0 : 0;
	return pComposite ? *static_cast<std::shared_ptr<ICryUnknown>*>(pComposite) : std::shared_ptr<ICryUnknown>();
}

template<class Src>
std::shared_ptr<const ICryUnknown> crycomposite_query(const Src* p, const char* name, bool* pExposed = 0)
{
	void* pComposite = p ? p->QueryComposite(name) : 0;
	pExposed ? *pExposed = pComposite != 0 : 0;
	return pComposite ? *static_cast<std::shared_ptr<const ICryUnknown>*>(pComposite) : std::shared_ptr<const ICryUnknown>();
}

template<class Src>
std::shared_ptr<ICryUnknown> crycomposite_query(const std::shared_ptr<Src>& p, const char* name, bool* pExposed = 0)
{
	return crycomposite_query(p.get(), name, pExposed);
}

template<class Src>
std::shared_ptr<const ICryUnknown> crycomposite_query(const std::shared_ptr<const Src>& p, const char* name, bool* pExposed = 0)
{
	return crycomposite_query(p.get(), name, pExposed);
}

#define _BEFRIEND_CRYCOMPOSITE_QUERY()                                                                                                                   \
  template<class Src> friend std::shared_ptr<ICryUnknown> CompositeQuerySemantics::crycomposite_query(Src*, const char*, bool*);                         \
  template<class Src> friend std::shared_ptr<const ICryUnknown> CompositeQuerySemantics::crycomposite_query(const Src*, const char*, bool*);             \
  template<class Src> friend std::shared_ptr<ICryUnknown> CompositeQuerySemantics::crycomposite_query(const std::shared_ptr<Src> &, const char*, bool*); \
  template<class Src> friend std::shared_ptr<const ICryUnknown> CompositeQuerySemantics::crycomposite_query(const std::shared_ptr<const Src> &, const char*, bool*);

}

using CompositeQuerySemantics::crycomposite_query;

#define _BEFRIEND_DELETER(iname) \
  friend struct std::default_delete<iname>;

//! Prevent explicit destruction from client side (exception is std::checked_delete which gets befriended).
#define _PROTECTED_DTOR(iname) \
  protected:                   \
    virtual ~iname() {}

//! Befriending cryinterface_cast<T>() and crycomposite_query() via CRYINTERFACE_DECLARE is actually only needed for ICryUnknown
//! since QueryInterface() and QueryComposite() are usually not redeclared in derived interfaces but it doesn't hurt either
#define CRYINTERFACE_DECLARE(iname, iidHigh, iidLow)                                     \
  _BEFRIEND_CRYIIDOF()                                                                   \
  _BEFRIEND_CRYINTERFACE_CAST()                                                          \
  _BEFRIEND_CRYCOMPOSITE_QUERY()                                                         \
  _BEFRIEND_DELETER(iname)                                                               \
  _PROTECTED_DTOR(iname)                                                                 \
                                                                                         \
private:                                                                                 \
  static const CryInterfaceID& IID()                                                     \
  {                                                                                      \
    static const CryInterfaceID iid = { (uint64) iidHigh ## LL, (uint64) iidLow ## LL }; \
    return iid;                                                                          \
  }                                                                                      \
public:

struct ICryUnknown
{
	CRYINTERFACE_DECLARE(ICryUnknown, 0x1000000010001000, 0x1000100000000000);

	virtual ICryFactory* GetFactory() const = 0;

protected:
	virtual void* QueryInterface(const CryInterfaceID& iid) const = 0;
	virtual void* QueryComposite(const char* name) const = 0;
};

DECLARE_SHARED_POINTERS(ICryUnknown);

#endif // #ifndef _ICRYUNKNOWN_H_
