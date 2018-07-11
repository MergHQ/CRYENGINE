// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/IDescExtension.h>
#include <CryReflection/Function/FunctionDelegate.h>

#include <tuple>

namespace Cry {
namespace Reflection {

struct ITypeDesc;

enum class EFunctionParamFlags : uint32
{
	Empty       = 0,
	IsReference = BIT(0),
	IsConst     = BIT(1),
};
typedef CEnumFlags<EFunctionParamFlags> FunctionParamFlags;

struct IFunctionParameterDesc
{
	virtual ~IFunctionParameterDesc() {}

	virtual CTypeId            GetTypeId() const = 0;
	virtual uint32             GetIndex() const = 0;

	virtual FunctionParamFlags GetFlags() const = 0;
	virtual void               SetFlags(FunctionParamFlags flags) = 0;

	virtual bool               IsConst() const = 0;
	virtual bool               IsReference() const = 0;
};

struct IFunctionDesc : public IExtensibleDesc
{
	virtual ~IFunctionDesc() {}

	virtual CGuid                         GetGuid() const = 0;
	virtual FunctionIndex                 GetIndex() const = 0;

	virtual const char*                   GetFullQualifiedName() const = 0;
	virtual const char*                   GetLabel() const = 0;
	virtual void                          SetLabel(const char* szLabel) = 0;

	virtual const char*                   GetDescription() const = 0;
	virtual void                          SetDescription(const char* szDescription) = 0;

	virtual bool                          IsMemberFunction() const = 0;

	virtual const CFunctionDelegate&      GetFunctionDelegate() const = 0;

	virtual const ITypeDesc*              GetObjectTypeDesc() const = 0;
	virtual CTypeId                       GetReturnTypeId() const = 0;

	virtual size_t                        GetParamCount() const = 0;
	virtual const IFunctionParameterDesc* GetParam(size_t index) const = 0;

	virtual const SSourceFileInfo&        GetSourceInfo() const = 0;
};

struct SFunctionParameterDesc
{
	SFunctionParameterDesc(CTypeId typeId_, size_t index_)
		: typeId(typeId_)
		, index(static_cast<uint32>(index_))
	{
		CRY_ASSERT_MESSAGE(index_ <= std::numeric_limits<uint32>::max(), "Index must be smaller or equal to uint32.");
	}

	FunctionParamFlags flags;
	CTypeId            typeId;
	uint32             index;
};

typedef std::vector<SFunctionParameterDesc> ParamArray;

class CFunction
{
public:
	CFunction()
		: m_isConst(false)
	{}

	bool                     IsValid() const              { return m_delegate.IsValid(); }
	bool                     IsConst() const              { return m_isConst; }

	const string&            GetFullQualifiedName() const { return m_fullQualifiedName; }
	const CTypeId            GetReturnTypeId() const      { return m_returnTypeId; }
	const CFunctionDelegate& GetFunctionDelegate() const  { return m_delegate; }

	ParamArray&              GetParams()                  { return m_params; }
	const ParamArray&        GetParams() const            { return m_params; }

protected:
	string            m_fullQualifiedName;
	CTypeId           m_returnTypeId;
	CFunctionDelegate m_delegate;
	ParamArray        m_params;
	bool              m_isConst;
};

namespace Utils {

template<typename FUNCTION_TYPE>
struct CFunctionDesc
{
	CFunctionDesc()
	{
		CRY_ASSERT_MESSAGE(false, "Missing CFunctionDesc specialization.");
	}
};

template<typename RETURN_TYPE, typename OBJECT_TYPE, typename ... PARAM_TYPES>
struct CFunctionDesc<RETURN_TYPE (OBJECT_TYPE::*)(PARAM_TYPES ...)>
{
	typedef RETURN_TYPE (OBJECT_TYPE::* FuncPtrType)(PARAM_TYPES ...);

	typedef RETURN_TYPE                 ReturnType;
	typedef OBJECT_TYPE                 ObjectType;
	typedef std::tuple<PARAM_TYPES ...> ParameterTypes;

	template<size_t INDEX>
	using ParamType = typename std::tuple_element<INDEX, ParameterTypes>::type;

	constexpr static bool IsConst()          { return false; }
	constexpr static bool IsMemberFunction() { return true; }
};

template<typename RETURN_TYPE, typename OBJECT_TYPE, typename ... PARAM_TYPES>
struct CFunctionDesc<RETURN_TYPE (OBJECT_TYPE::*)(PARAM_TYPES ...) const>
{
	typedef RETURN_TYPE (OBJECT_TYPE::* FuncPtrType)(PARAM_TYPES ...) const;

	typedef RETURN_TYPE                 ReturnType;
	typedef OBJECT_TYPE                 ObjectType;
	typedef std::tuple<PARAM_TYPES ...> ParameterTypes;

	template<size_t INDEX>
	using ParamType = typename std::tuple_element<INDEX, ParameterTypes>::type;

	constexpr static bool IsConst()          { return true; }
	constexpr static bool IsMemberFunction() { return true; }
};

template<typename RETURN_TYPE, typename ... PARAM_TYPES>
struct CFunctionDesc<RETURN_TYPE (*)(PARAM_TYPES ...)>
{
	typedef RETURN_TYPE (*              FuncPtrType)(PARAM_TYPES ...);

	typedef RETURN_TYPE                 ReturnType;
	typedef std::tuple<PARAM_TYPES ...> ParameterTypes;

	template<size_t INDEX>
	using ParamType = typename std::tuple_element<INDEX, ParameterTypes>::type;

	constexpr static bool IsConst()          { return false; }
	constexpr static bool IsMemberFunction() { return false; }
};

template<typename RETURN_TYPE>
class SReturnType
{
public:
	SReturnType()
		: m_typeId(Type::IdOf<RETURN_TYPE>())
	{}

	operator CTypeId() const
	{
		return m_typeId;
	}

private:
	CTypeId m_typeId;
};

template<>
class SReturnType<void>
{
public:
	SReturnType() {}

	operator CTypeId() const
	{
		return CTypeId();
	}
};

template<typename FUNCTION_DESC, size_t INDEX, bool IS_END>
struct ParamIterator
{
	typedef FUNCTION_DESC FunctionDesc;

	static void Append(ParamArray& params)
	{
		params.emplace_back(Type::IdOf<typename FunctionDesc::template ParamType<INDEX>>(), INDEX);
		SFunctionParameterDesc& param = params.back();

		if (std::is_const<typename FunctionDesc::template ParamType<INDEX>>::value)
			param.flags.Add(EFunctionParamFlags::IsConst);
		if (std::is_reference<typename FunctionDesc::template ParamType<INDEX>>::value || std::is_pointer<typename FunctionDesc::template ParamType<INDEX>>::value)
			param.flags.Add(EFunctionParamFlags::IsReference);

		ParamIterator < FUNCTION_DESC, INDEX + 1, (INDEX + 1) < std::tuple_size<typename FunctionDesc::ParameterTypes>::value > ::Append(params);
	}
};

template<typename FUNCTION_DESC, size_t INDEX>
struct ParamIterator<FUNCTION_DESC, INDEX, false>
{
	static void Append(ParamArray& params) {}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE, size_t PARAM_COUNT>
struct SProxy
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		//static_assert(false, "Unsupported amount of parameters.");
	}
};

template<typename FUNCTION_DESC, bool IS_MEMBER = FUNCTION_DESC::IsMemberFunction()>
struct SExecutionSelection
{
	template<typename ... PARAM_TYPES>
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context, PARAM_TYPES&& ... params)
	{
		funcPtr.Get<typename FUNCTION_DESC::FuncPtrType>()(params ...);
	}

	template<typename ... PARAM_TYPES>
	static typename FUNCTION_DESC::ReturnType ExecuteWithReturn(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context, PARAM_TYPES&& ... params)
	{
		return funcPtr.Get<typename FUNCTION_DESC::FuncPtrType>()(params ...);
	}
};

template<typename FUNCTION_DESC>
struct SExecutionSelection<FUNCTION_DESC, true>
{
	template<typename ... PARAM_TYPES>
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context, PARAM_TYPES&& ... params)
	{
		(context.GetObject<typename FUNCTION_DESC::ObjectType>()->*funcPtr.Get<typename FUNCTION_DESC::FuncPtrType>())(params ...);
	}

	template<typename ... PARAM_TYPES>
	static typename FUNCTION_DESC::ReturnType ExecuteWithReturn(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context, PARAM_TYPES&& ... params)
	{
		return (context.GetObject<typename FUNCTION_DESC::ObjectType>()->*funcPtr.Get<typename FUNCTION_DESC::FuncPtrType>())(params ...);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 0>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		CRY_ASSERT_MESSAGE(context.GetParamCount() == 0, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context);
		context.SetResult<RETURN_TYPE>(result);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 1>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 1 && pParam0, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context, *pParam0);
		context.SetResult<RETURN_TYPE>(result);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 2>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 2 && pParam0 && pParam1, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context, *pParam0, *pParam1);
		context.SetResult<RETURN_TYPE>(result);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 3>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 3 && pParam0 && pParam1 && pParam2, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context, *pParam0, *pParam1, *pParam2);
		context.SetResult<RETURN_TYPE>(result);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 4>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 4 && pParam0 && pParam1 && pParam2 && pParam3, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3);
		context.SetResult<RETURN_TYPE>(result);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 5>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 5 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4);
		context.SetResult<RETURN_TYPE>(result);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 6>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);
		auto* const pParam5 = context.GetParam<typename FUNCTION_DESC::template ParamType<5>>(5);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 6 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4 && pParam5, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4, *pParam5);
		context.SetResult<RETURN_TYPE>(result);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 7>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);
		auto* const pParam5 = context.GetParam<typename FUNCTION_DESC::template ParamType<5>>(5);
		auto* const pParam6 = context.GetParam<typename FUNCTION_DESC::template ParamType<6>>(6);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 7 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4 && pParam5 && pParam6, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4, *pParam5, *pParam6);
		context.SetResult<RETURN_TYPE>(result);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 8>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);
		auto* const pParam5 = context.GetParam<typename FUNCTION_DESC::template ParamType<5>>(5);
		auto* const pParam6 = context.GetParam<typename FUNCTION_DESC::template ParamType<6>>(6);
		auto* const pParam7 = context.GetParam<typename FUNCTION_DESC::template ParamType<7>>(7);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 8 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4 && pParam5 && pParam6 && pParam7, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4, *pParam5, *pParam6, *pParam7);
		context.SetResult<RETURN_TYPE>(result);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 9>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);
		auto* const pParam5 = context.GetParam<typename FUNCTION_DESC::template ParamType<5>>(5);
		auto* const pParam6 = context.GetParam<typename FUNCTION_DESC::template ParamType<6>>(6);
		auto* const pParam7 = context.GetParam<typename FUNCTION_DESC::template ParamType<7>>(7);
		auto* const pParam8 = context.GetParam<typename FUNCTION_DESC::template ParamType<8>>(8);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 9 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4 && pParam5 && pParam6 && pParam7 && pParam8, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4, *pParam5, *pParam6, *pParam7, *pParam8);
		context.SetResult<RETURN_TYPE>(result);
	}
};

template<typename FUNCTION_DESC, typename RETURN_TYPE>
struct SProxy<FUNCTION_DESC, RETURN_TYPE, 10>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);
		auto* const pParam5 = context.GetParam<typename FUNCTION_DESC::template ParamType<5>>(5);
		auto* const pParam6 = context.GetParam<typename FUNCTION_DESC::template ParamType<6>>(6);
		auto* const pParam7 = context.GetParam<typename FUNCTION_DESC::template ParamType<7>>(7);
		auto* const pParam8 = context.GetParam<typename FUNCTION_DESC::template ParamType<8>>(8);
		auto* const pParam9 = context.GetParam<typename FUNCTION_DESC::template ParamType<9>>(9);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 10 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4 && pParam5 && pParam6 && pParam7 && pParam8 && pParam9, "Parameter missmatch.");

		const RETURN_TYPE result = SExecutionSelection<FUNCTION_DESC>::ExecuteWithReturn(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4, *pParam5, *pParam6, *pParam7, *pParam8, *pParam9);
		context.SetResult<RETURN_TYPE>(result);
	}
};

//////////////////////////////////////////////////////////////////////////
// RETURN_TYPE == void
//////////////////////////////////////////////////////////////////////////

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 0>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		CRY_ASSERT_MESSAGE(context.GetParamCount() == 0, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context);
	}
};

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 1>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 1 && pParam0, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context, *pParam0);
	}
};

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 2>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 2 && pParam0 && pParam1, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context, *pParam0, *pParam1);
	}
};

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 3>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 3 && pParam0 && pParam1 && pParam2, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context, *pParam0, *pParam1, *pParam2);
	}
};

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 4>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 4 && pParam0 && pParam1 && pParam2 && pParam3, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3);
	}
};

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 5>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 5 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4);
	}
};

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 6>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);
		auto* const pParam5 = context.GetParam<typename FUNCTION_DESC::template ParamType<5>>(5);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 6 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4 && pParam5, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4, *pParam5);
	}
};

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 7>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);
		auto* const pParam5 = context.GetParam<typename FUNCTION_DESC::template ParamType<5>>(5);
		auto* const pParam6 = context.GetParam<typename FUNCTION_DESC::template ParamType<6>>(6);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 7 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4 && pParam5 && pParam6, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4, *pParam5, *pParam6);
	}
};

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 8>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);
		auto* const pParam5 = context.GetParam<typename FUNCTION_DESC::template ParamType<5>>(5);
		auto* const pParam6 = context.GetParam<typename FUNCTION_DESC::template ParamType<6>>(6);
		auto* const pParam7 = context.GetParam<typename FUNCTION_DESC::template ParamType<7>>(7);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 8 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4 && pParam5 && pParam6 && pParam7, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4, *pParam5, *pParam6, *pParam7);
	}
};

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 9>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);
		auto* const pParam5 = context.GetParam<typename FUNCTION_DESC::template ParamType<5>>(5);
		auto* const pParam6 = context.GetParam<typename FUNCTION_DESC::template ParamType<6>>(6);
		auto* const pParam7 = context.GetParam<typename FUNCTION_DESC::template ParamType<7>>(7);
		auto* const pParam8 = context.GetParam<typename FUNCTION_DESC::template ParamType<8>>(8);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 9 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4 && pParam5 && pParam6 && pParam7 && pParam8, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4, *pParam5, *pParam6, *pParam7, *pParam8);
	}
};

template<typename FUNCTION_DESC>
struct SProxy<FUNCTION_DESC, void, 10>
{
	static void Execute(const CFunctionPtr& funcPtr, CFunctionExecutionContext& context)
	{
		auto* const pParam0 = context.GetParam<typename FUNCTION_DESC::template ParamType<0>>(0);
		auto* const pParam1 = context.GetParam<typename FUNCTION_DESC::template ParamType<1>>(1);
		auto* const pParam2 = context.GetParam<typename FUNCTION_DESC::template ParamType<2>>(2);
		auto* const pParam3 = context.GetParam<typename FUNCTION_DESC::template ParamType<3>>(3);
		auto* const pParam4 = context.GetParam<typename FUNCTION_DESC::template ParamType<4>>(4);
		auto* const pParam5 = context.GetParam<typename FUNCTION_DESC::template ParamType<5>>(5);
		auto* const pParam6 = context.GetParam<typename FUNCTION_DESC::template ParamType<6>>(6);
		auto const pParam7 = context.GetParam<typename FUNCTION_DESC::template ParamType<7>>(7);
		auto const pParam8 = context.GetParam<typename FUNCTION_DESC::template ParamType<8>>(8);
		auto const pParam9 = context.GetParam<typename FUNCTION_DESC::template ParamType<9>>(9);

		CRY_ASSERT_MESSAGE(context.GetParamCount() == 10 && pParam0 && pParam1 && pParam2 && pParam3 && pParam4 && pParam5 && pParam6 && pParam7 && pParam8 && pParam9, "Parameter missmatch.");

		SExecutionSelection<FUNCTION_DESC>::Execute(funcPtr, context, *pParam0, *pParam1, *pParam2, *pParam3, *pParam4, *pParam5, *pParam6, *pParam7, *pParam8, *pParam9);
	}
};

}   // ~Utils namespace

template<typename FUNCTION_DESC>
class CFunctionCreator : public CFunction
{
public:
	typedef FUNCTION_DESC FunctionDesc;

	template<typename FUNCTION_PTR>
	CFunctionCreator(FUNCTION_PTR pFunction)
	{
		// TODO: This is currently missing the function name.
		m_fullQualifiedName.assign(Cry::Type::Utils::SCompileTime_TypeInfo<FUNCTION_PTR>::GetName().GetBegin(), Cry::Type::Utils::SCompileTime_TypeInfo<FUNCTION_PTR>::GetName().GetLength());
		// ~TODO
		m_returnTypeId = Utils::SReturnType<typename FunctionDesc::ReturnType>();

		m_params.reserve(std::tuple_size<typename FunctionDesc::ParameterTypes>::value);
		Utils::ParamIterator < FunctionDesc, 0, 0 < std::tuple_size<typename FunctionDesc::ParameterTypes>::value > ::Append(m_params);

		m_delegate = CFunctionDelegate(pFunction, &Utils::SProxy<FunctionDesc, typename FunctionDesc::ReturnType, std::tuple_size<typename FunctionDesc::ParameterTypes>::value>::Execute);
	}
};

} // ~Reflection namespace
} // ~Cry namespace
