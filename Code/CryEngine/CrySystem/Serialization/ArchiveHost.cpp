// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/yasli/JSONIArchive.h>
#include <CrySerialization/yasli/JSONOArchive.h>
#include <CrySerialization/yasli/BinArchive.h>
#include "XmlIArchive.h"
#include "XmlOArchive.h"
#include <CrySerialization/BlackBox.h>
#include <CrySerialization/ClassFactory.h>

namespace Serialization
{
constexpr const char* kXmlVersionAttribute = "CryXmlVersion";
constexpr int kXmlVersionCurrent = 2;

bool LoadFile(std::vector<char>& content, const char* filename)
{
	FILE* f = gEnv->pCryPak->FOpen(filename, "rb");
	if (!f)
		return false;

	gEnv->pCryPak->FSeek(f, 0, SEEK_END);
	size_t size = gEnv->pCryPak->FTell(f);
	gEnv->pCryPak->FSeek(f, 0, SEEK_SET);

	content.resize(size);
	bool result = true;
	if (size != 0)
		result = gEnv->pCryPak->FRead(&content[0], size, f) == size;
	gEnv->pCryPak->FClose(f);
	return result;
}

class CArchiveHost : public IArchiveHost
{
public:
	bool LoadJsonFile(const SStruct& obj, const char* filename) override
	{
		std::vector<char> content;
		if (!LoadFile(content, filename))
			return false;
		yasli::JSONIArchive ia;
		if (!ia.open(content.data(), content.size()))
			return false;
		return ia(obj);
	}

	bool SaveJsonFile(const char* gameFilename, const SStruct& obj) override
	{
		char buffer[ICryPak::g_nMaxPath];
		const char* filename = gEnv->pCryPak->AdjustFileName(gameFilename, buffer, ICryPak::FLAGS_FOR_WRITING);
		yasli::JSONOArchive oa;
		if (!oa(obj))
			return false;
		return oa.save(filename);
	}

	bool LoadJsonBuffer(const SStruct& obj, const char* buffer, size_t bufferLength) override
	{
		if (bufferLength == 0)
			return false;
		yasli::JSONIArchive ia;
		if (!ia.open(buffer, bufferLength))
			return false;
		return ia(obj);
	}

	bool SaveJsonBuffer(DynArray<char>& buffer, const SStruct& obj) override
	{
		yasli::JSONOArchive oa;
		if (!oa(obj))
			return false;
		buffer.assign(oa.buffer(), oa.buffer() + oa.length());
		return true;
	}

	bool LoadBinaryFile(const SStruct& obj, const char* filename) override
	{
		std::vector<char> content;
		if (!LoadFile(content, filename))
			return false;
		yasli::BinIArchive ia;
		if (!ia.open(content.data(), content.size()))
			return false;
		return ia(obj);
	}

	bool SaveBinaryFile(const char* gameFilename, const SStruct& obj) override
	{
		char buffer[ICryPak::g_nMaxPath];
		const char* filename = gEnv->pCryPak->AdjustFileName(gameFilename, buffer, ICryPak::FLAGS_FOR_WRITING);
		yasli::BinOArchive oa;
		obj(oa);
		return oa.save(filename);
	}

	bool LoadBinaryBuffer(const SStruct& obj, const char* buffer, size_t bufferLength) override
	{
		if (bufferLength == 0)
			return false;
		yasli::BinIArchive ia;
		if (!ia.open(buffer, bufferLength))
			return false;
		return ia(obj);
	}

	bool SaveBinaryBuffer(DynArray<char>& buffer, const SStruct& obj) override
	{
		yasli::BinOArchive oa;
		obj(oa);
		buffer.assign(oa.buffer(), oa.buffer() + oa.length());
		return true;
	}

	bool CloneBinary(const SStruct& dest, const SStruct& src) override
	{
		yasli::BinOArchive oa;
		src(oa);
		yasli::BinIArchive ia;
		if (!ia.open(oa.buffer(), oa.length()))
			return false;
		dest(ia);
		return true;
	}

	bool CompareBinary(const SStruct& lhs, const SStruct& rhs) override
	{
		yasli::BinOArchive oa1;
		lhs(oa1);
		yasli::BinOArchive oa2;
		rhs(oa2);
		if (oa1.length() != oa2.length())
			return false;
		return memcmp(oa1.buffer(), oa2.buffer(), oa1.length()) == 0;
	}

	bool SaveXmlFile(const char* filename, const SStruct& obj, const char* rootNodeName) override
	{
		XmlNodeRef node = SaveXmlNode(obj, rootNodeName);
		if (!node)
			return false;
		return node->saveToFile(filename);
	}

	bool LoadXmlFile(const SStruct& obj, const char* filename,int forceVersion=-1) override
	{
		XmlNodeRef node = gEnv->pSystem->LoadXmlFromFile(filename);
		if (!node)
			return false;
		return LoadXmlNode(obj, node, forceVersion);
	}

	XmlNodeRef SaveXmlNode(const SStruct& obj, const char* nodeName) override
	{
		//CXmlOArchive oa;
		CXmlOutputArchive oa;
		XmlNodeRef node = gEnv->pSystem->CreateXmlNode(nodeName);
		if (!node)
			return XmlNodeRef();
		//node->setAttr("CryXmlVersion",1); // for CXmlOArchive oa;
		node->setAttr(kXmlVersionAttribute,kXmlVersionCurrent);
		oa.SetXmlNode(node);
		if (!obj(oa))
			return XmlNodeRef();
		return oa.GetXmlNode();
	}

	bool SaveXmlNode(XmlNodeRef& node, const SStruct& obj) override
	{
		if (!node)
			return false;
		//CXmlOArchive oa;
		CXmlOutputArchive oa;
		//node->setAttr("CryXmlVersion",1); // for CXmlOArchive oa;
		node->setAttr(kXmlVersionAttribute,kXmlVersionCurrent);
		oa.SetXmlNode(node);
		return obj(oa);
	}

	bool LoadXmlNode(const SStruct& obj, const XmlNodeRef& node,int forceVersion=-1) override
	{
		int version = 1; // Default to old version
		if (forceVersion < 0)
		{
			node->getAttr(kXmlVersionAttribute,version);
		}
		else
		{
			version = forceVersion;
		}
		if (version == kXmlVersionCurrent)
		{
			CXmlInputArchive ia;
			ia.SetXmlNode(node);
			if (!obj(ia))
				return false;
		}
		else
		{
			// Old version 1
			CXmlIArchiveVer1 ia;
			ia.SetXmlNode(node);
			if (!obj(ia))
				return false;
		}
		return true;
	}

	bool LoadBlackBox(const SStruct& outObj, SBlackBox& box) override
	{
		if (box.format && box.data)
		{
			if (strcmp(box.format, "json") == 0)
			{
				return LoadJsonBuffer(outObj, static_cast<const char*>(box.data), box.size);
			}
			else if (strcmp(box.format, "xml") == 0)
			{
				return LoadXmlNode(outObj, *static_cast<const XmlNodeRef*>(box.data),box.xmlVersion);
			}
		}
		return false;
	}
};

IArchiveHost* CreateArchiveHost()
{
	return new CArchiveHost;
}

}
