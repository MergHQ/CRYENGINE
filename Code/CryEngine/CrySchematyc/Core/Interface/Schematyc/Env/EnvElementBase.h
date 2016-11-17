// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Env/IEnvElement.h"

namespace Schematyc
{
template<typename INTERFACE> class CEnvElementBase : public INTERFACE
{
public:

	inline CEnvElementBase(const SGUID& guid, const char* szName, const SSourceFileInfo& sourceFileInfo)
		: m_guid(guid)
		, m_name(szName)
		, m_sourceFileInfo(sourceFileInfo)
		, m_pParent(nullptr)
		, m_pFirstChild(nullptr)
		, m_pLastChild(nullptr)
		, m_pPrevSibling(nullptr)
		, m_pNextSibling(nullptr)
	{}

	inline ~CEnvElementBase()
	{
		if (m_pParent)
		{
			m_pParent->DetachChild(*this);
		}
		for (IEnvElement* pChild = m_pFirstChild; pChild; )
		{
			IEnvElement* pNextChild = pChild->GetNextSibling();
			pChild->SetParent(nullptr);
			pChild->SetPrevSibling(nullptr);
			pChild->SetNextSibling(nullptr);
			pChild = pNextChild;
		}
	}

	// IEnvElement

	virtual EEnvElementType GetElementType() const override
	{
		return INTERFACE::ElementType;
	}

	virtual EnvElementFlags GetElementFlags() const override
	{
		return m_flags;
	}

	virtual SGUID GetGUID() const override
	{
		return m_guid;
	}

	virtual const char* GetName() const override
	{
		return m_name.c_str();
	}

	virtual SSourceFileInfo GetSourceFileInfo() const override
	{
		return m_sourceFileInfo;
	}

	virtual const char* GetDescription() const override
	{
		return m_description.c_str();
	}

	virtual const char* GetAuthor() const override
	{
		return m_author.c_str();
	}

	virtual const char* GetWikiLink() const override
	{
		return m_wikiLink.c_str();
	}

	virtual bool AttachChild(IEnvElement& child) override
	{
		if (child.IsValidScope(*this))
		{
			IEnvElement* pPrevParent = child.GetParent();
			if (pPrevParent)
			{
				pPrevParent->DetachChild(child);
			}

			child.SetParent(this);
			child.SetPrevSibling(m_pLastChild);

			if (m_pFirstChild)
			{
				m_pLastChild->SetNextSibling(&child);
			}
			else
			{
				m_pFirstChild = &child;
			}
			m_pLastChild = &child;
			return true;
		}
		return false;
	}

	virtual void DetachChild(IEnvElement& child) override
	{
		const IEnvElement* pParent = child.GetParent();
		SCHEMATYC_CORE_ASSERT(pParent == this);
		if (pParent == this)
		{
			IEnvElement* pPrevSibling = child.GetPrevSibling();
			IEnvElement* pNextSibling = child.GetNextSibling();

			if (&child == m_pFirstChild)
			{
				SCHEMATYC_CORE_ASSERT(!pPrevSibling);
				m_pFirstChild = pNextSibling;
			}
			else
			{
				SCHEMATYC_CORE_ASSERT(pPrevSibling);
				pPrevSibling->SetNextSibling(pNextSibling);
			}

			if (&child == m_pLastChild)
			{
				SCHEMATYC_CORE_ASSERT(!pNextSibling);
				m_pLastChild = pPrevSibling;
			}
			else
			{
				SCHEMATYC_CORE_ASSERT(pNextSibling);
				pNextSibling->SetPrevSibling(pPrevSibling);
			}

			child.SetParent(nullptr);
			child.SetPrevSibling(nullptr);
			child.SetNextSibling(nullptr);
		}
	}

	virtual void SetParent(IEnvElement* pParent) override
	{
		m_pParent = pParent;
	}

	virtual void SetPrevSibling(IEnvElement* pPrevSibling) override
	{
		m_pPrevSibling = pPrevSibling;
	}

	virtual void SetNextSibling(IEnvElement* pNextSibling) override
	{
		m_pNextSibling = pNextSibling;
	}

	virtual IEnvElement* GetParent() override
	{
		return m_pParent;
	}

	virtual const IEnvElement* GetParent() const override
	{
		return m_pParent;
	}

	virtual IEnvElement* GetFirstChild() override
	{
		return m_pFirstChild;
	}

	virtual const IEnvElement* GetFirstChild() const override
	{
		return m_pFirstChild;
	}

	virtual IEnvElement* GetLastChild() override
	{
		return m_pLastChild;
	}

	virtual const IEnvElement* GetLastChild() const override
	{
		return m_pLastChild;
	}

	virtual IEnvElement* GetPrevSibling() override
	{
		return m_pPrevSibling;
	}

	virtual const IEnvElement* GetPrevSibling() const override
	{
		return m_pPrevSibling;
	}

	virtual IEnvElement* GetNextSibling() override
	{
		return m_pNextSibling;
	}

	virtual const IEnvElement* GetNextSibling() const override
	{
		return m_pNextSibling;
	}

	virtual EVisitStatus VisitChildren(const EnvElementConstVisitor& visitor) const override
	{
		SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
		if (!visitor.IsEmpty())
		{
			for (const IEnvElement* pChild = m_pFirstChild; pChild; pChild = pChild->GetNextSibling())
			{
				EVisitStatus visitStatus = visitor(*pChild);
				if (visitStatus == EVisitStatus::Recurse)
				{
					visitStatus = pChild->VisitChildren(visitor);
				}
				if (visitStatus == EVisitStatus::Stop)
				{
					return visitStatus;
				}
			}
			return EVisitStatus::Continue;
		}
		return EVisitStatus::Error;
	}

	// ~IEnvElement

	inline void SetAuthor(const char* szAuthor)
	{
		m_author = szAuthor;
	}

	inline void SetDescription(const char* szDescription)
	{
		m_description = szDescription;
	}

	inline void SetWikiLink(const char* szWikiLink)
	{
		m_wikiLink = szWikiLink;
	}

	//protected:

	inline void SetName(const char* szName)
	{
		m_name = szName;
	}

private:

	EnvElementFlags m_flags;
	SGUID           m_guid;
	string          m_name;
	SSourceFileInfo m_sourceFileInfo;
	string          m_author;
	string          m_description;
	string          m_wikiLink;
	IEnvElement*    m_pParent;
	IEnvElement*    m_pFirstChild;
	IEnvElement*    m_pLastChild;
	IEnvElement*    m_pPrevSibling;
	IEnvElement*    m_pNextSibling;
};
} // Schematyc
