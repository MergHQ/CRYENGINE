// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "IConsole.h"// ICVar
#include "ISystem.h" // CryWarning

class CTCVarBase : public ICVar
{
public:

	virtual ~CTCVarBase()
	{
		if (m_szName && (m_nFlags & VF_COPYNAME))
		{
			delete[] m_szName;
		}
#if defined(DEDICATED_SERVER)
		if (m_pDataProbeString)
		{
			delete[] m_pDataProbeString;
		}
#endif
	}

	virtual void SetConsole(IConsole* pConsole)
	{
		m_pConsole = pConsole;
	}

	virtual bool IsOwnedByConsole() const override { return false; }

protected:

	CTCVarBase() {}

	void RegisterBase(IConsole* pConsole, const char* sName, int nFlags, const char* help)
	{
		m_psHelp = help;
		m_pConsole = pConsole;
		m_nFlags = nFlags;
		if (nFlags & VF_COPYNAME)
		{
			char* sCopyName = new char[strlen(sName) + 1];
			strcpy(sCopyName, sName);
			m_szName = sCopyName;
		}
		else
		{
			m_szName = sName;
		}
		pConsole->Register(this);
	}

	virtual void        Release() override                                                            { m_pConsole->UnregisterVariable(m_szName, false); }
	virtual void        Set(const char* s) override                                                   { CRY_ASSERT(false); }
	virtual void        Set(float f) override                                                         { CRY_ASSERT(false); }
	virtual void        Set(int i) override                                                           { CRY_ASSERT(false); }
	virtual void        Set(int64 i) override                                                         { CRY_ASSERT(false); }
	virtual void        ForceSet(const char* s) override                                              { CRY_ASSERT(false); }
	virtual int         SetFlags(int flags) override                                                  { m_nFlags = flags; return m_nFlags; }
	virtual void        SetMinValue(int min) override                                                 { CRY_ASSERT(false); }
	virtual void        SetMinValue(int64 min) override                                               { CRY_ASSERT(false); }
	virtual void        SetMinValue(float min) override                                               { CRY_ASSERT(false); }
	virtual void        SetMaxValue(int max) override                                                 { CRY_ASSERT(false); }
	virtual void        SetMaxValue(int64 max) override                                               { CRY_ASSERT(false); }
	virtual void        SetMaxValue(float max) override                                               { CRY_ASSERT(false); }

	virtual int         GetIVal() const override                                                      { CRY_ASSERT(false); return 0; }
	virtual int64       GetI64Val() const override                                                    { CRY_ASSERT(false); return 0; }
	virtual float       GetFVal() const override                                                      { CRY_ASSERT(false); return 0; }
	virtual const char* GetString() const override                                                    { CRY_ASSERT(false); return "NULL"; }
	virtual int         GetRealIVal() const override                                                  { return GetIVal(); }
	virtual ECVarType   GetType() const override                                                      { CRY_ASSERT(false); return ECVarType::Int; }
	virtual const char* GetName() const override final                                                { return m_szName; }
	virtual const char* GetHelp() const override                                                      { return m_psHelp; }
	virtual int         GetFlags() const override                                                     { return m_nFlags; }
	virtual bool        IsConstCVar() const override                                                  { return (m_nFlags & VF_CONST_CVAR) != 0; }
	virtual void        ClearFlags(int flags) override                                                { m_nFlags &= ~flags; }

	virtual void        SetAllowedValues(std::initializer_list<int> values) override                  { CRY_ASSERT(false); }
	virtual void        SetAllowedValues(std::initializer_list<int64> values) override                { CRY_ASSERT(false); }
	virtual void        SetAllowedValues(std::initializer_list<float> values) override                { CRY_ASSERT(false); }
	virtual void        SetAllowedValues(std::initializer_list<string> values) override               { CRY_ASSERT(false); }
	virtual void        GetMemoryUsage(class ICrySizer* pSizer) const override                        { CRY_ASSERT(false); }
	virtual void        DebugLog(const int iExpectedValue, const EConsoleLogMode mode) const override { CRY_ASSERT(false); }

	virtual uint64      AddOnChange(SmallFunction<void()> changeFunctor) override
	{
		uint64 functorId = m_changeFunctorIdTotal;
		m_onChangeCallbacks.emplace_back(SCallback{ std::move(changeFunctor), functorId });
		++m_changeFunctorIdTotal;
		return functorId;
	}

	virtual bool RemoveOnChangeFunctor(const uint64 id) override
	{
		bool erased = stl::find_and_erase_if(m_onChangeCallbacks, [id](const SCallback& callback)
		{
			return callback.id == id;
		});
		return erased;
	}

	virtual uint64 GetNumberOfOnChangeFunctors() const override
	{
		return m_onChangeCallbacks.size();
	}

	virtual const SmallFunction<void()>& GetOnChangeFunctor(uint64 id) const override
	{
		CRY_ASSERT_MESSAGE(id < m_changeFunctorIdTotal, "[CTCVarBase::GetOnChangeFunctor] Functor id out of range");

		const auto iter = std::find_if(m_onChangeCallbacks.begin(), m_onChangeCallbacks.end(), [id](const SCallback& callback)
		{
			return callback.id == id;
		});

		CRY_ASSERT(iter != m_onChangeCallbacks.end());
		return iter->function;
	}

	void CallOnChangeFunctions()
	{
		for (SCallback& callback : m_onChangeCallbacks)
		{
			callback.function();
		}
	}

#if defined(DEDICATED_SERVER)
	virtual void SetDataProbeString(const char* pDataProbeString) override
	{
		CRY_ASSERT(m_pDataProbeString == NULL);
		m_pDataProbeString = new char[strlen(pDataProbeString) + 1];
		strcpy(m_pDataProbeString, pDataProbeString);
	}
#endif

	virtual const char* GetDataProbeString() const override
	{
#if defined(DEDICATED_SERVER)
		if (m_pDataProbeString)
		{
			return m_pDataProbeString;
		}
#endif
		return GetString();
	}

protected:

	const char* m_szName = nullptr;
	const char* m_psHelp = nullptr;
#if defined(DEDICATED_SERVER)
	char*       m_pDataProbeString = nullptr;
#endif
	int         m_nFlags = 0;

	struct SCallback
	{
		SmallFunction<void()> function;
		uint64                id;
	};

	std::vector<SCallback> m_onChangeCallbacks;
	uint64                 m_changeFunctorIdTotal = 0;

	IConsole*              m_pConsole = nullptr;

	int& m_sys_cvar_logging()
	{
		static int m_sys_cvar_logging = 1;
		return m_sys_cvar_logging;
	}
};

template<typename T>
class TCVar;

template<>
class TCVar<int> : public CTCVarBase
{
public:

	TCVar()
		: m_type(ECVarTypeHelper::GetTypeForT<int>())
		, m_min(std::numeric_limits<int>::lowest())
		, m_max(std::numeric_limits<int>::max())
	{}

	virtual ~TCVar()
	{
		m_pConsole->UnregisterVariable(GetName(), false);
	}

	void Register(const char* sName, IConsole* pConsole, int val, int nFlags, const char* help)
	{
		RegisterBase(pConsole, sName, nFlags, help);
		Set(val);
	}

	virtual void SetFromString(const char* s) override { Set(atoi(s)); }
	virtual void Set(int val) override
	{
		if (val == m_val && !(m_nFlags & VF_ALWAYSONCHANGE))
		{
			return;
		}

		if (m_pConsole->OnBeforeVarChange(this, string().Format("%i", val)))
		{
			m_nFlags |= VF_MODIFIED;
			if (!m_allowedValues.empty())
			{
				if (std::find(m_allowedValues.begin(), m_allowedValues.end(), val) != m_allowedValues.end())
				{
					m_val = val;
				}
				else if (m_sys_cvar_logging() > 0)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "'%d' is not a valid value of '%s'", val, GetName());
				}
			}
			else
			{
				if (m_sys_cvar_logging() > 0 && (val > m_max || val < m_min))
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "'%d' is not in the allowed range of '%s' (%d-%d)", val, GetName(), m_min, m_max);
				}
				m_val = std::min(std::max(val, m_min), m_max);
			}
			CallOnChangeFunctions();
			m_pConsole->OnAfterVarChange(this);
		}
	}
	virtual void SetAllowedValues(std::initializer_list<int> allowed) override
	{
		m_allowedValues = allowed;
		if (!m_allowedValues.empty() && std::find(m_allowedValues.begin(), m_allowedValues.end(), m_val) == m_allowedValues.end())
		{
			Set(m_allowedValues[0]);
		}
	}
	virtual void        SetMinValue(int min) override { m_min = min; if (m_val < m_min) Set(m_min); }
	virtual void        SetMaxValue(int max) override { m_max = max; if (m_val > m_max) Set(m_max); }

	virtual int         GetIVal() const override      { return m_val; }
	virtual const char* GetString() const override
	{
		static thread_local string str;
		str.Format("%i", m_val);
		return str.c_str();
	}

	virtual ECVarType GetType() const override { return m_type; }

	TCVar<int>&       operator=(int val)       { Set(val); return *this; }
	TCVar<int>&       operator*=(int val)      { Set(m_val * val); return *this; }
	TCVar<int>&       operator/=(int val)      { Set(m_val / val); return *this; }
	TCVar<int>&       operator+=(int val)      { Set(m_val + val); return *this; }
	TCVar<int>&       operator-=(int val)      { Set(m_val - val); return *this; }
	TCVar<int>&       operator&=(int val)      { Set(m_val & val); return *this; }
	TCVar<int>&       operator|=(int val)      { Set(m_val | val); return *this; }
	TCVar<int>&       operator++()             { Set(m_val + 1); return *this; }
	TCVar<int>&       operator--()             { Set(m_val - 1); return *this; }
	operator int() const { return m_val; }

private:

	const ECVarType  m_type;

	int              m_val;
	int              m_min;
	int              m_max;
	std::vector<int> m_allowedValues;
};

template<>
class TCVar<int64> : public CTCVarBase
{
public:

	TCVar()
		: m_type(ECVarTypeHelper::GetTypeForT<int64>())
		, m_min(std::numeric_limits<int64>::lowest())
		, m_max(std::numeric_limits<int64>::max())
	{}

	virtual ~TCVar()
	{
		m_pConsole->UnregisterVariable(GetName(), false);
	}

	void Register(const char* sName, IConsole* pConsole, int64 val, int nFlags, const char* help)
	{
		RegisterBase(pConsole, sName, nFlags, help);
		Set(val);
	}

	virtual void SetFromString(const char* s) override { Set(atoll(s)); }

	virtual void Set(int val) override                 { Set((int64)val); }
	virtual void Set(int64 val) override
	{
		if (val == m_val && !(m_nFlags & VF_ALWAYSONCHANGE))
		{
			return;
		}

		if (m_pConsole->OnBeforeVarChange(this, string().Format("%i", val)))
		{
			m_nFlags |= VF_MODIFIED;
			if (!m_allowedValues.empty())
			{
				if (std::find(m_allowedValues.begin(), m_allowedValues.end(), val) != m_allowedValues.end())
				{
					m_val = val;
				}
				else if (m_sys_cvar_logging() > 0)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "'%d' is not a valid value of '%s'", val, GetName());
				}
			}
			else
			{
				if (m_sys_cvar_logging() > 0 && (val > m_max || val < m_min))
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "'%d' is not in the allowed range of '%s' (%d-%d)", val, GetName(), m_min, m_max);
				}
				m_val = std::min(std::max(val, m_min), m_max);
			}
			CallOnChangeFunctions();
			m_pConsole->OnAfterVarChange(this);
		}
	}
	virtual void SetAllowedValues(std::initializer_list<int64> allowed) override
	{
		m_allowedValues = allowed;
		if (!m_allowedValues.empty() && std::find(m_allowedValues.begin(), m_allowedValues.end(), m_val) == m_allowedValues.end())
		{
			Set(m_allowedValues[0]);
		}
	}
	virtual void        SetMinValue(int64 min) override { m_min = min; if (m_val < m_min) Set(m_min); }
	virtual void        SetMaxValue(int64 max) override { m_max = max; if (m_val > m_max) Set(m_max); }

	virtual int         GetIVal() const override        { return (int)m_val; } // Might eventually want remove this?
	virtual int64       GetI64Val() const override      { return m_val; }
	virtual const char* GetString() const override
	{
		static thread_local string str;
		str.Format("%lli", m_val);
		return str.c_str();
	}

	virtual ECVarType GetType() const override { return m_type; }

	TCVar<int64>&     operator=(int64 val)     { Set(val); return *this; }
	TCVar<int64>&     operator*=(int64 val)    { Set(m_val * val); return *this; }
	TCVar<int64>&     operator/=(int64 val)    { Set(m_val / val); return *this; }
	TCVar<int64>&     operator+=(int64 val)    { Set(m_val + val); return *this; }
	TCVar<int64>&     operator-=(int64 val)    { Set(m_val - val); return *this; }
	TCVar<int64>&     operator&=(int64 val)    { Set(m_val & val); return *this; }
	TCVar<int64>&     operator|=(int64 val)    { Set(m_val | val); return *this; }
	TCVar<int64>&     operator++()             { Set(m_val + 1); return *this; }
	TCVar<int64>&     operator--()             { Set(m_val - 1); return *this; }
	operator int64() const { return m_val; }

private:

	const ECVarType    m_type;

	int64              m_val;
	int64              m_min;
	int64              m_max;
	std::vector<int64> m_allowedValues;
};

template<>
class TCVar<float> : public CTCVarBase
{
public:

	TCVar()
		: m_type(ECVarTypeHelper::GetTypeForT<float>())
		, m_min(std::numeric_limits<float>::lowest())
		, m_max(std::numeric_limits<float>::max())
	{}

	virtual ~TCVar()
	{
		m_pConsole->UnregisterVariable(GetName(), false);
	}

	void Register(const char* sName, IConsole* pConsole, float val, int nFlags, const char* help)
	{
		RegisterBase(pConsole, sName, nFlags, help);
		Set(val);
	}

	virtual void SetFromString(const char* s) override { Set((float)atof(s)); }
	virtual void Set(float val) override
	{
		if (val == m_val && !(m_nFlags & VF_ALWAYSONCHANGE))
		{
			return;
		}
		if (m_pConsole->OnBeforeVarChange(this, string().Format("%i", val)))
		{
			m_nFlags |= VF_MODIFIED;
			if (m_sys_cvar_logging() > 0 && (val > m_max || val < m_min))
			{
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "'%f' is not in the allowed range of '%s' (%f-%f)", val, GetName(), m_min, m_max);
			}
			m_val = std::min(std::max(val, m_min), m_max);
			CallOnChangeFunctions();
			m_pConsole->OnAfterVarChange(this);
		}
	}
	virtual void        SetMinValue(float min) override { m_min = min; if (m_val < m_min) Set(m_min); }
	virtual void        SetMaxValue(float max) override { m_max = max; if (m_val > m_max) Set(m_max); }

	virtual float       GetFVal() const override        { return m_val; }
	virtual const char* GetString() const override
	{
		static thread_local string str;
		str.Format("%.8f", m_val);
		return str.c_str();
	}
	virtual ECVarType GetType() const override { return m_type; }

	TCVar<float>&     operator=(float val)     { Set(val);  return *this; }
	TCVar<float>&     operator*=(float val)    { Set(m_val * val); return *this; }
	TCVar<float>&     operator/=(float val)    { Set(m_val / val); return *this; }
	TCVar<float>&     operator+=(float val)    { Set(m_val + val); return *this; }
	TCVar<float>&     operator-=(float val)    { Set(m_val - val); return *this; }
	operator float() const { return m_val; }

private:

	const ECVarType m_type;

	float           m_val;
	float           m_min;
	float           m_max;
};

template<>
class TCVar<const char*> : public CTCVarBase
{
public:

	TCVar()
		: m_type(ECVarTypeHelper::GetTypeForT<const char*>())
	{}
	virtual ~TCVar()
	{
		m_pConsole->UnregisterVariable(GetName(), false);
	}

	void Register(const char* sName, IConsole* pConsole, const char* val, int nFlags, const char* help)
	{
		RegisterBase(pConsole, sName, nFlags, help);
		Set(val);
	}

	virtual void SetFromString(const char* s) override { Set(s); }
	virtual void Set(const char* val) override
	{
		if (string(val) == m_val && !(m_nFlags & VF_ALWAYSONCHANGE))
		{
			return;
		}
		if (m_pConsole->OnBeforeVarChange(this, val))
		{
			m_nFlags |= VF_MODIFIED;
			if (m_allowedValues.empty() || (!m_allowedValues.empty() && std::find(m_allowedValues.begin(), m_allowedValues.end(), string(val)) != m_allowedValues.end()))
			{
				m_val = val;
			}
			CallOnChangeFunctions();
			m_pConsole->OnAfterVarChange(this);
		}
	}
	virtual const char* GetString() const override { return m_val.c_str(); }
	virtual void        SetAllowedValues(std::initializer_list<string> allowed) override
	{
		m_allowedValues = allowed;
		if (!m_allowedValues.empty() && std::find(m_allowedValues.begin(), m_allowedValues.end(), m_val) == m_allowedValues.end())
		{
			Set(m_allowedValues[0]);
		}
	}
	virtual ECVarType   GetType() const override   { return m_type; }

	TCVar<const char*>& operator=(const char* val) { Set(val); return *this; }
	operator const string&() { return m_val; }

private:

	const ECVarType     m_type;

	string              m_val;
	std::vector<string> m_allowedValues;
};

// std overloads to allow for min(int, TCVar<int>) etc for compatibility
namespace std
{
////////// min //////////
template<class WrappedLeft, class Right>
constexpr typename std::common_type<WrappedLeft, Right>::type min(const TCVar<WrappedLeft>& lhs, const Right& rhs)
{
	return ((WrappedLeft)lhs < rhs) ? (WrappedLeft)lhs : rhs;
}

template<class Left, class WrappedRight>
constexpr typename std::common_type<Left, WrappedRight>::type min(const Left& lhs, const TCVar<WrappedRight>& rhs)
{
	return (lhs < (WrappedRight)rhs) ? lhs : (WrappedRight)rhs;
}

template<class T>
constexpr const TCVar<T>& min(const TCVar<T>& lhs, const TCVar<T>& rhs)
{
	return (lhs < rhs) ? lhs : rhs;
}

////////// max //////////
template<class WrappedLeft, class Right>
constexpr typename std::common_type<WrappedLeft, Right>::type max(const TCVar<WrappedLeft>& lhs, const Right& rhs)
{
	return ((WrappedLeft)lhs > rhs) ? (WrappedLeft)lhs : rhs;
}

template<class Left, class WrappedRight>
constexpr typename std::common_type<Left, WrappedRight>::type max(const Left& lhs, const TCVar<WrappedRight>& rhs)
{
	return (lhs > (WrappedRight)rhs) ? lhs : (WrappedRight)rhs;
}

template<class T>
constexpr const TCVar<T>& max(const TCVar<T>& lhs, const TCVar<T>& rhs)
{
	return (lhs > rhs) ? lhs : rhs;
}
}
