// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __XMLCONVERTER_H__
#define __XMLCONVERTER_H__

#include "IConvertor.h"
#include "ICryXML.h"
#include "NameConvertor.h"
#include <CrySystem/XML/XMLBinaryHeaders.h>

struct XMLFilterElement
{
	XMLBinary::IFilter::EType type;
	bool bAccept;
	string wildcards;
};

class XMLCompiler : public ICompiler
{
public:
	XMLCompiler(
		ICryXML* pCryXML,
		const std::vector<XMLFilterElement>& filter,
		const std::vector<string>& tableFilemasks,
		const NameConvertor& nameConverter);

public:
	virtual void Release();

	virtual void BeginProcessing(const IConfig* config) { }
	virtual void EndProcessing() { }
	virtual IConvertContext* GetConvertContext() { return &m_CC; }
	virtual bool Process();

private:
	string GetOutputFileNameOnly() const;
	string GetOutputPath() const;

private:
	int m_refCount;
	ICryXML* m_pCryXML;
	const std::vector<XMLFilterElement>* m_pFilter;
	const std::vector<string>* m_pTableFilemasks;
	const NameConvertor* m_pNameConverter;
	ConvertContext m_CC;
};

class XMLConverter : public IConvertor
{
public:
	explicit XMLConverter(ICryXML* pCryXML);
	~XMLConverter();

	virtual void Release();

	virtual void Init(const ConvertorInitContext& context);
	virtual ICompiler* CreateCompiler();
	virtual bool SupportsMultithreading() const;
	virtual const char* GetExt(int index) const;

private:
	int m_refCount;
	ICryXML* m_pCryXML;
	std::vector<XMLFilterElement> m_filter;
	std::vector<string> m_tableFilemasks;
	NameConvertor m_nameConvertor;
};

#endif //__XMLCONVERTER_H__
