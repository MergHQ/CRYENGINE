// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ICompressionPolicy.h"
#include "ArithModel.h"
#include "Protocol/Serialize.h"

class CTimePolicy
{
public:
	bool Load(XmlNodeRef node, const string& filename)
	{
		m_stream = eTS_Network;

		if (XmlNodeRef params = node->findChild("Params"))
		{
			string stream = params->getAttr("stream");
			if (stream == "Network")
				m_stream = eTS_Network;
			else if (stream == "Pong")
				m_stream = eTS_NetworkPong;
			else if (stream == "Ping")
				m_stream = eTS_NetworkPing;
			else if (stream == "Physics")
				m_stream = eTS_Physics;
			else if (stream == "Remote")
				m_stream = eTS_RemoteTime;
			else if (stream == "PongElapsed")
				m_stream = eTS_PongElapsed;
			else
				NetWarning("Unknown time-stream '%s' found at %s:%d", stream.c_str(), filename.c_str(), params->getLine());
		}

		return true;
	}
#if USE_MEMENTO_PREDICTORS
	bool ReadMemento(CByteInputStream& in) const
	{
		return true;
	}

	bool WriteMemento(CByteOutputStream& out) const
	{
		return true;
	}

	void NoMemento() const
	{
	}
#endif

#if USE_ARITHSTREAM
	bool ReadValue(CCommInputStream& in, CTimeValue& value, CArithModel* pModel, uint32 age) const
	{
		value = pModel->ReadTime(in, m_stream);
		NetLogPacketDebug("CTimePolicy::ReadValue %" PRId64 " (%f)", value.GetMilliSecondsAsInt64(), in.GetBitSize());
		return true;
	}
	bool WriteValue(CCommOutputStream& out, CTimeValue value, CArithModel* pModel, uint32 age) const
	{
		pModel->WriteTime(out, m_stream, value);
		return true;
	}

	template<class T>
	bool ReadValue(CCommInputStream& in, T& value, CArithModel* pModel, uint32 age) const
	{
		NetWarning("EntityIdPolicy: not implemented for generic types");
		return false;
	}
	template<class T>
	bool WriteValue(CCommOutputStream& out, T value, CArithModel* pModel, uint32 age) const
	{
		NetWarning("EntityIdPolicy: not implemented for generic types");
		return false;
	}
#else
	bool ReadValue(CNetInputSerializeImpl* in, CTimeValue& value, uint32 age) const
	{
		value = in->ReadTime(m_stream);
		NetLogPacketDebug("CTimePolicy::ReadValue %" PRId64 " (%f)", value.GetMilliSecondsAsInt64(), in->GetBitSize());
		return true;
	}

	bool WriteValue(CNetOutputSerializeImpl* out, CTimeValue value, uint32 age) const
	{
		out->WriteTime(m_stream, value);
		return true;
	}

	template<class T>
	bool ReadValue(CNetInputSerializeImpl* in, T& value, uint32 age) const
	{
		NetWarning("EntityIdPolicy: not implemented for generic types");
		return false;
	}

	template<class T>
	bool WriteValue(CNetOutputSerializeImpl* out, T value, uint32 age) const
	{
		NetWarning("EntityIdPolicy: not implemented for generic types");
		return false;
	}
#endif

	void GetMemoryStatistics(ICrySizer* pSizer) const
	{
		SIZER_COMPONENT_NAME(pSizer, "CTimePolicy");
		pSizer->Add(*this);
	}
#if NET_PROFILE_ENABLE
	int GetBitCount(CTimeValue value)
	{
		return BITCOUNT_TIME;
	}

	template<class T>
	int GetBitCount(T value)
	{
		return 0;
	}
#endif
private:
	ETimeStream m_stream;
};

REGISTER_COMPRESSION_POLICY(CTimePolicy, "Time");
