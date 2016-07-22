// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CryAction.h"

#include <CryGame/IGameTokens.h>
#include <CryFlowGraph/IFlowBaseNode.h>

namespace
{
IGameTokenSystem* GetIGameTokenSystem()
{
	CCryAction* pCryAction = CCryAction::GetCryAction();
	return pCryAction ? pCryAction->GetIGameTokenSystem() : NULL;
}

void WarningGameTokenNotFound(const char* place, const char* tokenName)
{
	CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "[flow] %s: cannot find gametoken: '%s'.", place, tokenName);
}

IGameToken* GetGameToken(IFlowNode::SActivationInfo* pActInfo, const string& tokenName, bool bForceCreate)
{
	if (tokenName.empty())
		return NULL;

	IGameToken* pToken = GetIGameTokenSystem()->FindToken(tokenName.c_str());
	if (!pToken)
	{
		// try graph token instead:
		string name = pActInfo->pGraph->GetGlobalNameForGraphToken(tokenName);

		pToken = GetIGameTokenSystem()->FindToken(name.c_str());
		assert(!pToken || pToken->GetFlags() & EGAME_TOKEN_GRAPHVARIABLE);

		if (!pToken && bForceCreate)
			pToken = GetIGameTokenSystem()->SetOrCreateToken(tokenName.c_str(), TFlowInputData(string("<undefined>"), false));
	}

	if (!pToken)
	{
		WarningGameTokenNotFound("SetGameTokenFlowNode", tokenName.c_str());
	}

	return pToken;
}

const int bForceCreateDefault = false;   // create non-existing game tokens, default false for the moment
};

class CSetGameTokenFlowNode : public CFlowBaseNode<eNCT_Instanced>
{
public:
	CSetGameTokenFlowNode(SActivationInfo* pActInfo) : m_pCachedToken(NULL)
	{
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CSetGameTokenFlowNode(pActInfo);
	}

	virtual void Serialize(SActivationInfo*, TSerialize ser)
	{
		if (ser.IsReading()) // forces re-caching
			m_pCachedToken = 0;
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void("Trigger",            _HELP("Trigger this input to actually set the game token value"), _HELP("Trigger")),
			InputPortConfig<string>("gametoken_Token", _HELP("Game token to set"),                                       _HELP("Token")),
			InputPortConfig<string>("Value",           _HELP("Value to set")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_AnyType("OutValue", _HELP("Value which was set (debug)")),
			{ 0 }
		};
		config.sDescription = _HELP("Set the value of a game token");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			m_pCachedToken = 0;
			break;
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, 1))
				{
					CacheToken(pActInfo);
				}
				if (IsPortActive(pActInfo, 0))
				{
					if (m_pCachedToken == NULL)
						CacheToken(pActInfo);

					if (m_pCachedToken != NULL)
					{
						m_pCachedToken->SetValue(GetPortAny(pActInfo, 2));
						TFlowInputData data;
						m_pCachedToken->GetValue(data);
						ActivateOutput(pActInfo, 0, data);
					}
				}
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:
	void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
	{
		string tokenName = GetPortString(pActInfo, 1);
		m_pCachedToken = GetGameToken(pActInfo, tokenName, bForceCreate);
	}

private:
	IGameToken* m_pCachedToken;
};

class CGetGameTokenFlowNode : public CFlowBaseNode<eNCT_Instanced>
{
public:
	CGetGameTokenFlowNode(SActivationInfo* pActInfo) : m_pCachedToken(NULL)
	{
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CGetGameTokenFlowNode(pActInfo);
	}

	virtual void Serialize(SActivationInfo*, TSerialize ser)
	{
		if (ser.IsReading()) // forces re-caching
			m_pCachedToken = 0;
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void("Trigger",            _HELP("Trigger this input to actually get the game token value"), _HELP("Trigger")),
			InputPortConfig<string>("gametoken_Token", _HELP("Game token to set"),                                       _HELP("Token")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_AnyType("OutValue", _HELP("Value of the game token")),
			{ 0 }
		};
		config.sDescription = _HELP("Get the value of a game token");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			m_pCachedToken = 0;
			break;
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, 1))
				{
					CacheToken(pActInfo);
				}
				if (IsPortActive(pActInfo, 0))
				{
					if (m_pCachedToken == NULL)
						CacheToken(pActInfo);

					if (m_pCachedToken != NULL)
					{
						TFlowInputData data;
						m_pCachedToken->GetValue(data);
						ActivateOutput(pActInfo, 0, data);
					}
				}
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:
	void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
	{
		string name = GetPortString(pActInfo, 1);
		m_pCachedToken = GetGameToken(pActInfo, name, bForceCreate);
	}

private:
	IGameToken* m_pCachedToken;
};

class CCheckGameTokenFlowNode : public CFlowBaseNode<eNCT_Instanced>
{
public:
	CCheckGameTokenFlowNode(SActivationInfo* pActInfo) : m_pCachedToken(NULL)
	{
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CCheckGameTokenFlowNode(pActInfo);
	}

	virtual void Serialize(SActivationInfo*, TSerialize ser)
	{
		if (ser.IsReading()) // forces re-caching
			m_pCachedToken = 0;
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void("Trigger",            _HELP("Trigger this input to actually get the game token value"), _HELP("Trigger")),
			InputPortConfig<string>("gametoken_Token", _HELP("Game token to set"),                                       _HELP("Token")),
			InputPortConfig<string>("CheckValue",      _HELP("Value to compare the token with")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_AnyType("OutValue", _HELP("Value of the token")),
			OutputPortConfig<bool>("Result",     _HELP("TRUE if equals, FALSE otherwise")),
			OutputPortConfig_Void("True",        _HELP("Triggered if equal")),
			OutputPortConfig_Void("False",       _HELP("Triggered if not equal")),
			{ 0 }
		};
		config.sDescription = _HELP("Check if the value of a game token equals to a value");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			m_pCachedToken = 0;
			break;
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, 1))
				{
					CacheToken(pActInfo);
				}
				if (IsPortActive(pActInfo, 0))
				{
					if (m_pCachedToken == NULL)
						CacheToken(pActInfo);

					if (m_pCachedToken != NULL)
					{
						// now this is a messy thing.
						// we use TFlowInputData to do all the work, because the
						// game tokens uses them as well.
						// does for the same values we get the same converted strings
						TFlowInputData tokenData;
						m_pCachedToken->GetValue(tokenData);
						TFlowInputData checkData(tokenData);
						checkData.SetValueWithConversion(GetPortString(pActInfo, 2));
						string tokenString, checkString;
						tokenData.GetValueWithConversion(tokenString);
						checkData.GetValueWithConversion(checkString);
						ActivateOutput(pActInfo, 0, tokenData);
						bool equal = tokenString.compareNoCase(checkString) == 0;
						ActivateOutput(pActInfo, 1, equal);
						// trigger either the true or false port depending on comparism
						ActivateOutput(pActInfo, 3 - static_cast<int>(equal), true);
					}
				}
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:
	void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
	{
		string name = GetPortString(pActInfo, 1);
		m_pCachedToken = GetGameToken(pActInfo, name, bForceCreate);
	}

private:
	IGameToken* m_pCachedToken;
};

class CGameTokenCheckMultiFlowNode : public CFlowBaseNode<eNCT_Instanced>
{
public:
	CGameTokenCheckMultiFlowNode(SActivationInfo* pActInfo) : m_pCachedToken(NULL)
	{
	}

	enum
	{
		INP_TRIGGER = 0,
		INP_GAMETOKEN,
		INP_FIRST_VALUE,
		INP_NUM_VALUES = 8
	};

	enum
	{
		OUT_VALUE = 0,
		OUT_ONE_TRUE,
		OUT_ALL_FALSE
	};

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CGameTokenCheckMultiFlowNode(pActInfo);
	}

	virtual void Serialize(SActivationInfo*, TSerialize ser)
	{
		if (ser.IsReading()) // forces re-caching
			m_pCachedToken = 0;
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void("Trigger",            _HELP("Trigger this input to perform the check"), _HELP("Trigger")),
			InputPortConfig<string>("gametoken_Token", _HELP("Game token to check"),                     _HELP("Token")),
			InputPortConfig<string>("Value1",          _HELP("Value to compare the token with")),
			InputPortConfig<string>("Value2",          _HELP("Value to compare the token with")),
			InputPortConfig<string>("Value3",          _HELP("Value to compare the token with")),
			InputPortConfig<string>("Value4",          _HELP("Value to compare the token with")),
			InputPortConfig<string>("Value5",          _HELP("Value to compare the token with")),
			InputPortConfig<string>("Value6",          _HELP("Value to compare the token with")),
			InputPortConfig<string>("Value7",          _HELP("Value to compare the token with")),
			InputPortConfig<string>("Value8",          _HELP("Value to compare the token with")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_AnyType("OutValue", _HELP("Value of the token")),
			OutputPortConfig_Void("One_True",    _HELP("Triggered if the token is equal to at least one of the values")),
			OutputPortConfig_Void("All_False",   _HELP("Triggered if no given value is equal to the token")),
			{ 0 }
		};
		config.sDescription = _HELP("Check if a game token equals to any of a list of values");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			m_pCachedToken = 0;
			break;
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, INP_GAMETOKEN))
				{
					CacheToken(pActInfo);
				}
				if (IsPortActive(pActInfo, INP_TRIGGER))
				{
					if (m_pCachedToken == NULL)
						CacheToken(pActInfo);

					if (m_pCachedToken != NULL)
					{
						// now this is a messy thing.
						// we use TFlowInputData to do all the work, because the
						// game tokens uses them as well.
						// does for the same values we get the same converted strings
						TFlowInputData tokenData;
						m_pCachedToken->GetValue(tokenData);
						TFlowInputData checkData(tokenData);

						ActivateOutput(pActInfo, OUT_VALUE, tokenData);

						string tokenString, checkString;
						tokenData.GetValueWithConversion(tokenString);

						bool bAnyTrue = false;

						for (int i = 0; i < INP_NUM_VALUES; i++)
						{
							TFlowInputData chkData(tokenData);
							checkString = GetPortString(pActInfo, INP_FIRST_VALUE + i);
							if (!checkString.empty())
							{
								chkData.SetValueWithConversion(checkString);
								chkData.GetValueWithConversion(checkString);  // this double conversion is to correctly manage cases like "true" strings vs bool tokens
								bAnyTrue |= (tokenString.compareNoCase(checkString) == 0);
							}
						}

						if (bAnyTrue)
							ActivateOutput(pActInfo, OUT_ONE_TRUE, true);
						else
							ActivateOutput(pActInfo, OUT_ALL_FALSE, true);
					}
				}
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:
	void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
	{
		string name = GetPortString(pActInfo, 1);
		m_pCachedToken = GetGameToken(pActInfo, name, bForceCreate);
	}

private:
	IGameToken* m_pCachedToken;
};

class CGameTokenFlowNode : public CFlowBaseNode<eNCT_Instanced>, public IGameTokenEventListener
{
	enum EInputs
	{
		eIN_Gametoken = 0,
		eIn_CampareString
	};

	enum EOutputs
	{
		eOUT_GametokenValue = 0,
		eOUT_EqualsTrue,
		eOUT_EqualsFalse,
		eOUT_Equals
	};

public:
	CGameTokenFlowNode(SActivationInfo* pActInfo) : m_pCachedToken(0), m_actInfo(*pActInfo)
	{
	}

	~CGameTokenFlowNode()
	{
		Unregister();
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CGameTokenFlowNode(pActInfo);
	}

	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		if (ser.IsReading())
		{
			// recache and register at token
			CacheToken(pActInfo);
		}
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<string>("gametoken_Token", _HELP("GameToken to set/get"), _HELP("Token")),
			InputPortConfig<string>("compare_Value",   _HELP("Value to compare to"),  _HELP("CompareTo")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_AnyType("Out",         _HELP("Outputs the token's value on every change"),                      _HELP("Output")),
			OutputPortConfig_AnyType("Equal True",  _HELP("Outputs when the token's value equals the checked input."),       _HELP("Equal")),
			OutputPortConfig_AnyType("Equal False", _HELP("Outputs when the token's value is not equal the checked input."), _HELP("Not Equal")),
			OutputPortConfig<bool>("Equals",        _HELP("Outputs the result of the equality check."),                      _HELP("Equals")),
			{ 0 }
		};
		config.sDescription = _HELP("GameToken set/get");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIN_Gametoken)) // Token port
				{
					CacheToken(pActInfo);
					if (m_pCachedToken != 0)
					{
						TFlowInputData data;
						m_pCachedToken->GetValue(data);
						ActivateOutput(pActInfo, eOUT_GametokenValue, data);
					}
				}
				if (IsPortActive(pActInfo, eIn_CampareString)) // comparison string port
				{
					m_sComparisonString = GetPortString(pActInfo, eIn_CampareString);
				}
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
private:
	void OnGameTokenEvent(EGameTokenEvent event, IGameToken* pGameToken)
	{
		assert(pGameToken == m_pCachedToken);
		if (event == EGAMETOKEN_EVENT_CHANGE)
		{
			TFlowInputData data;
			m_pCachedToken->GetValue(data);
			ActivateOutput(&m_actInfo, eOUT_GametokenValue, data);

			// now this is a messy thing.
			// we use TFlowInputData to do all the work, because the
			// game tokens uses them as well.
			TFlowInputData checkData(data);
			string tokenString;
			data.GetValueWithConversion(tokenString);
			bool equal = (tokenString.compareNoCase(m_sComparisonString) == 0);
			ActivateOutput(&m_actInfo, equal ? eOUT_EqualsTrue : eOUT_EqualsFalse, true);
			ActivateOutput(&m_actInfo, eOUT_Equals, equal);
		}
		else if (event == EGAMETOKEN_EVENT_DELETE)
		{
			// no need to unregister
			m_pCachedToken = 0;
		}
	}

	void Register()
	{
		IGameTokenSystem* pGTSys = GetIGameTokenSystem();
		if (m_pCachedToken && pGTSys)
			pGTSys->RegisterListener(m_pCachedToken->GetName(), this);
	}

	void Unregister()
	{
		IGameTokenSystem* pGTSys = GetIGameTokenSystem();
		if (m_pCachedToken && pGTSys)
		{
			pGTSys->UnregisterListener(m_pCachedToken->GetName(), this);
			m_pCachedToken = 0;
		}
	}

	void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
	{
		Unregister();

		string tokenName = GetPortString(pActInfo, eIN_Gametoken);
		m_pCachedToken = GetGameToken(pActInfo, tokenName, bForceCreate);

		if (m_pCachedToken)
			Register();
	}

private:
	SActivationInfo m_actInfo;
	IGameToken*     m_pCachedToken;
	string          m_sComparisonString;
};

class CModifyGameTokenFlowNode : public CFlowBaseNode<eNCT_Instanced>
{
public:
	CModifyGameTokenFlowNode(SActivationInfo* pActInfo) : m_pCachedToken(NULL)
	{
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CModifyGameTokenFlowNode(pActInfo);
	}

	virtual void Serialize(SActivationInfo*, TSerialize ser)
	{
		if (ser.IsReading()) // forces re-caching
			m_pCachedToken = 0;
	}

	enum EOperation
	{
		EOP_SET = 0,
		EOP_ADD,
		EOP_SUB,
		EOP_MUL,
		EOP_DIV
	};

	enum EType
	{
		ET_BOOL = 0,
		ET_INT,
		ET_FLOAT,
		ET_VEC3,
		ET_STRING
	};

	template<class Type>
	struct Helper
	{
		Helper(TFlowInputData& value, const TFlowInputData& operand)
		{
			Init(value, operand);
		}

		bool Init(TFlowInputData& value, const TFlowInputData& operand)
		{
			m_ok = value.GetValueWithConversion(m_value);
			m_ok &= operand.GetValueWithConversion(m_operand);
			return m_ok;
		}

		bool m_ok;
		Type m_value;
		Type m_operand;

	};

	template<class Type> bool not_zero(Type& val)          { return true; }
	bool                      not_zero(int& val)           { return val != 0; }
	bool                      not_zero(float& val)         { return val != 0.0f; }

	template<class Type> void op_set(Helper<Type>& data)   { if (data.m_ok) data.m_value = data.m_operand; }
	template<class Type> void op_add(Helper<Type>& data)   { if (data.m_ok) data.m_value += data.m_operand; }
	template<class Type> void op_sub(Helper<Type>& data)   { if (data.m_ok) data.m_value -= data.m_operand; }
	template<class Type> void op_mul(Helper<Type>& data)   { if (data.m_ok) data.m_value *= data.m_operand; }
	template<class Type> void op_div(Helper<Type>& data)   { if (data.m_ok && not_zero(data.m_operand)) data.m_value /= data.m_operand; }

	void                      op_div(Helper<string>& data) {};
	void                      op_mul(Helper<string>& data) {};
	void                      op_sub(Helper<string>& data) {};
	void                      op_mul(Helper<Vec3>& data)   {};
	void                      op_div(Helper<Vec3>& data)   {};

	void                      op_add(Helper<bool>& data)   {};
	void                      op_sub(Helper<bool>& data)   {};
	void                      op_div(Helper<bool>& data)   {};
	void                      op_mul(Helper<bool>& data)   {};

	template<class Type> void DoOp(EOperation op, Helper<Type>& data)
	{
		switch (op)
		{
		case EOP_SET:
			op_set(data);
			break;
		case EOP_ADD:
			op_add(data);
			break;
		case EOP_SUB:
			op_sub(data);
			break;
		case EOP_MUL:
			op_mul(data);
			break;
		case EOP_DIV:
			op_div(data);
			break;
		}
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void("Trigger",            _HELP("Trigger this input to actually get the game token value"), _HELP("Trigger")),
			InputPortConfig<string>("gametoken_Token", _HELP("Game token to set"),                                       _HELP("Token")),
			InputPortConfig<int>("Op",                 EOP_SET,                                                          _HELP("Operation token = token OP value"),_HELP("Operation"),_UICONFIG("enum_int:Set=0,Add=1,Sub=2,Mul=3,Div=4")),
			InputPortConfig<int>("Type",               ET_STRING,                                                        _HELP("Type"),                            _HELP("Type"),  _UICONFIG("enum_int:Bool=0,Int=1,Float=2,Vec3=3,String=4")),
			InputPortConfig<string>("Value",           _HELP("Value to operate with")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_AnyType("OutValue", _HELP("Value of the token")),
			{ 0 }
		};
		config.sDescription = _HELP("Operate on a GameToken");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_AISEQUENCE_SUPPORTED;
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			m_pCachedToken = 0;
			break;
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, 1))
				{
					CacheToken(pActInfo);
				}
				if (IsPortActive(pActInfo, 0))
				{
					if (m_pCachedToken == NULL)
						CacheToken(pActInfo);

					if (m_pCachedToken != NULL)
					{
						TFlowInputData value;
						m_pCachedToken->GetValue(value);
						TFlowInputData operand;
						operand.Set(GetPortString(pActInfo, 4));

						EOperation op = static_cast<EOperation>(GetPortInt(pActInfo, 2));
						EType type = static_cast<EType>(GetPortInt(pActInfo, 3));
						switch (type)
						{
						case ET_BOOL:
							{ Helper<bool> h(value, operand); DoOp(op, h); if (h.m_ok) m_pCachedToken->SetValue(TFlowInputData(h.m_value)); }   break;
						case ET_INT:
							{ Helper<int> h(value, operand); DoOp(op, h); if (h.m_ok) m_pCachedToken->SetValue(TFlowInputData(h.m_value)); }    break;
						case ET_FLOAT:
							{ Helper<float> h(value, operand); DoOp(op, h); if (h.m_ok) m_pCachedToken->SetValue(TFlowInputData(h.m_value)); }  break;
						case ET_VEC3:
							{ Helper<Vec3> h(value, operand); DoOp(op, h); if (h.m_ok) m_pCachedToken->SetValue(TFlowInputData(h.m_value)); }   break;
						case ET_STRING:
							{ Helper<string> h(value, operand); DoOp(op, h); if (h.m_ok) m_pCachedToken->SetValue(TFlowInputData(h.m_value)); } break;
						}
						m_pCachedToken->GetValue(value);
						ActivateOutput(pActInfo, 0, value);
					}
				}
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:
	void CacheToken(SActivationInfo* pActInfo, bool bForceCreate = bForceCreateDefault)
	{
		string name = GetPortString(pActInfo, 1);
		m_pCachedToken = GetGameToken(pActInfo, name, bForceCreate);
	}

private:
	IGameToken* m_pCachedToken;
};

//////////////////////////////////////////////////////////////////////////

class CFlowNodeGameTokensLevelToLevelStore : public CFlowBaseNode<eNCT_Singleton>
{
	enum
	{
		INP_TRIGGER = 0,
		INP_TOKEN1,
		INP_TOKEN2,
		INP_TOKEN3,
		INP_TOKEN4,
		INP_TOKEN5,
		INP_TOKEN6,
		INP_TOKEN7,
		INP_TOKEN8,
		INP_TOKEN9,
		INP_TOKEN10
	};

public:
	CFlowNodeGameTokensLevelToLevelStore(SActivationInfo* pActInfo)
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Trigger",              _HELP("When activated, the values of the specified gametokens will be internally stored so that they can be restored in next level")),
			InputPortConfig<string>("gametoken_Token1",  _HELP("GameToken to be stored"),                                                                                                      _HELP("Token1")),
			InputPortConfig<string>("gametoken_Token2",  _HELP("GameToken to be stored"),                                                                                                      _HELP("Token2")),
			InputPortConfig<string>("gametoken_Token3",  _HELP("GameToken to be stored"),                                                                                                      _HELP("Token3")),
			InputPortConfig<string>("gametoken_Token4",  _HELP("GameToken to be stored"),                                                                                                      _HELP("Token4")),
			InputPortConfig<string>("gametoken_Token5",  _HELP("GameToken to be stored"),                                                                                                      _HELP("Token5")),
			InputPortConfig<string>("gametoken_Token6",  _HELP("GameToken to be stored"),                                                                                                      _HELP("Token6")),
			InputPortConfig<string>("gametoken_Token7",  _HELP("GameToken to be stored"),                                                                                                      _HELP("Token7")),
			InputPortConfig<string>("gametoken_Token8",  _HELP("GameToken to be stored"),                                                                                                      _HELP("Token8")),
			InputPortConfig<string>("gametoken_Token9",  _HELP("GameToken to be stored"),                                                                                                      _HELP("Token9")),
			InputPortConfig<string>("gametoken_Token10", _HELP("GameToken to be stored"),                                                                                                      _HELP("Token10")),
			{ 0 }
		};
		config.pInputPorts = inputs;
		config.sDescription = _HELP("When triggered, the value of all specified gametokens will be internally stored. Those values can be restored by using the 'restore' flow node in next level");
		config.SetCategory(EFLN_ADVANCED);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, INP_TRIGGER))
				{
					std::vector<const char*> gameTokensList;
					for (uint32 i = INP_TOKEN1; i <= INP_TOKEN10; ++i)
					{
						const string& gameTokenName = GetPortString(pActInfo, i);
						if (!gameTokenName.empty())
							gameTokensList.push_back(gameTokenName.c_str());
					}
					const char** ppGameTokensList = &(gameTokensList[0]);
					GetIGameTokenSystem()->SerializeSaveLevelToLevel(ppGameTokensList, (uint32)gameTokensList.size());
				}
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////

class CFlowNodeGameTokensLevelToLevelRestore : public CFlowBaseNode<eNCT_Singleton>
{
	enum
	{
		INP_TRIGGER = 0,
	};

public:
	CFlowNodeGameTokensLevelToLevelRestore(SActivationInfo* pActInfo)
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Trigger", _HELP("When activated, the gametokens previously saved with a 'store' flownode (suposedly at the end of the previous level) will recover their stored value")),
			{ 0 }
		};
		config.pInputPorts = inputs;
		config.sDescription = _HELP("When 'Trigger' is activated, the gametokens previously saved with a 'store' flownode (supposedly at the end of the previous level) will recover their stored value");
		config.SetCategory(EFLN_ADVANCED);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, INP_TRIGGER))
				{
					GetIGameTokenSystem()->SerializeReadLevelToLevel();
				}
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

REGISTER_FLOW_NODE("Mission:GameTokenModify", CModifyGameTokenFlowNode);
REGISTER_FLOW_NODE("Mission:GameTokenSet", CSetGameTokenFlowNode);
REGISTER_FLOW_NODE("Mission:GameTokenGet", CGetGameTokenFlowNode);
REGISTER_FLOW_NODE("Mission:GameTokenCheck", CCheckGameTokenFlowNode);
REGISTER_FLOW_NODE("Mission:GameTokenCheckMulti", CGameTokenCheckMultiFlowNode);
REGISTER_FLOW_NODE("Mission:GameToken", CGameTokenFlowNode);
REGISTER_FLOW_NODE("Mission:GameTokensLevelToLevelStore", CFlowNodeGameTokensLevelToLevelStore);
REGISTER_FLOW_NODE("Mission:GameTokensLevelToLevelRestore", CFlowNodeGameTokensLevelToLevelRestore);
