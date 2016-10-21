// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QPropertyTree/ContextList.h>
#include <Schematyc/Env/Schematyc_IEnvRegistry.h>
#include <Schematyc/Services/Schematyc_ISettingsManager.h>
#include <Schematyc/Utils/Schematyc_Signal.h>

#include "Schematyc_ScriptGraphView.h"

namespace CrySchematycEditor {

enum class EDetailItemType
{
	Empty,
	Settings,
	ScriptElement,
	GraphViewNode
};

struct IDetailItem
{
	virtual ~IDetailItem() {}

	virtual EDetailItemType              GetType() const = 0;
	virtual Schematyc::SGUID             GetGUID() const = 0;
	virtual Serialization::CContextList* GetContextList() = 0;
	virtual void                         Serialize(Serialization::IArchive& archive) = 0;
};

DECLARE_SHARED_POINTERS(IDetailItem)

class CSettingsDetailItem : public IDetailItem
{
public:
	CSettingsDetailItem(const Schematyc::ISettingsPtr& pSettings);

	// IDetailItem
	virtual EDetailItemType              GetType() const override;
	virtual Schematyc::SGUID             GetGUID() const override;
	virtual Serialization::CContextList* GetContextList() override;
	virtual void                         Serialize(Serialization::IArchive& archive) override;
	// ~IDetailItem

private:
	Schematyc::ISettingsPtr m_pSettings;
};
DECLARE_SHARED_POINTERS(CSettingsDetailItem)

class CScriptElementDetailItem : public IDetailItem
{
public:
	CScriptElementDetailItem(Schematyc::IScriptElement* pScriptElement);

	// IDetailItem
	virtual EDetailItemType              GetType() const override;
	virtual Schematyc::SGUID             GetGUID() const override;
	virtual Serialization::CContextList* GetContextList() override;
	virtual void                         Serialize(Serialization::IArchive& archive) override;
	// ~IDetailItem

private:
	Schematyc::IScriptElement*                   m_pScriptElement;
	std::unique_ptr<Serialization::CContextList> m_pContextList;
};
DECLARE_SHARED_POINTERS(CScriptElementDetailItem)

class CScriptGraphViewNodeDetailItem : public IDetailItem
{
public:
	CScriptGraphViewNodeDetailItem(const Schematyc::CScriptGraphViewNodePtr& pScriptGraphViewNode);

	// IDetailItem
	virtual EDetailItemType              GetType() const override;
	virtual Schematyc::SGUID             GetGUID() const override;
	virtual Serialization::CContextList* GetContextList() override;
	virtual void                         Serialize(Serialization::IArchive& archive) override;
	// ~IDetailItem

private:
	Schematyc::CScriptGraphViewNodePtr           m_pScriptGraphViewNode;
	std::unique_ptr<Serialization::CContextList> m_pContextList;
};
DECLARE_SHARED_POINTERS(CScriptGraphViewNodeDetailItem)

}
