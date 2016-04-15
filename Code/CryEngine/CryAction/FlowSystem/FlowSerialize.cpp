// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FlowSerialize.h"

class CFlowDataReadVisitor : public boost::static_visitor<void>
{
public:
	CFlowDataReadVisitor(const char* data) : m_data(data), m_ok(false) {}

	void Visit(int& i)
	{
		m_ok = 1 == sscanf(m_data, "%i", &i);
	}

	void Visit(float& i)
	{
		m_ok = 1 == sscanf(m_data, "%f", &i);
	}

	void Visit(EntityId& i)
	{
		m_ok = 1 == sscanf(m_data, "%u", &i);
	}

	void Visit(Vec3& i)
	{
		m_ok = 3 == sscanf(m_data, "%f,%f,%f", &i.x, &i.y, &i.z);
	}

	void Visit(string& i)
	{
		i = m_data;
		m_ok = true;
	}

	void Visit(bool& b)
	{
		int i;
		m_ok = 1 == sscanf(m_data, "%i", &i);
		b = i != 0;
	}

	void Visit(SFlowSystemVoid&)
	{
	}

	bool operator!() const
	{
		return !m_ok;
	}

	template<class T>
	void operator()(T& type)
	{
		Visit(type);
	}

private:
	const char* m_data;
	bool        m_ok;
};

class CFlowDataWriteVisitor : public boost::static_visitor<void>
{
public:
	CFlowDataWriteVisitor(string& out) : m_out(out) {}

	void Visit(int i)
	{
		m_out.Format("%i", i);
	}

	void Visit(float i)
	{
		m_out.Format("%f", i);
	}

	void Visit(EntityId i)
	{
		m_out.Format("%u", i);
	}

	void Visit(Vec3 i)
	{
		m_out.Format("%f,%f,%f", i.x, i.y, i.z);
	}

	void Visit(const string& i)
	{
		m_out = i;
	}

	void Visit(bool b)
	{
		Visit(int(b));
	}

	void Visit(SFlowSystemVoid)
	{
		m_out.resize(0);
	}

	template<class T>
	void operator()(T& type)
	{
		Visit(type);
	}

private:
	string& m_out;
};

bool SetFromString(TFlowInputData& value, const char* str)
{
	CFlowDataReadVisitor visitor(str);
	value.Visit(visitor);
	return !!visitor;
}

string ConvertToString(const TFlowInputData& value)
{
	string out;
	CFlowDataWriteVisitor visitor(out);
	value.Visit(visitor);
	return out;
}

bool SetAttr(XmlNodeRef node, const char* attr, const TFlowInputData& value)
{
	string out = ConvertToString(value);
	node->setAttr(attr, out.c_str());
	return true;
}
