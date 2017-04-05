// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
// Sandbox plugin wrapper.
#pragma once
#include "IEditor.h"
#include "IPlugin.h"

// Base class for plugin
class CFbxToolPlugin : public IPlugin
{
public:
	static CFbxToolPlugin* GetInstance();

	explicit CFbxToolPlugin(IEditor* pEditor);

	void SetPersonalizationProperty(const QString& propName, const QVariant& value);
	const QVariant& GetPersonalizationProperty(const QString& propName);

	virtual void        Release() override;

	virtual const char* GetPluginGUID() override;

	virtual const char* GetPluginName() override;

	virtual DWORD       GetPluginVersion() override;

	virtual bool        CanExitNow() override
	{
		return true;
	}

	virtual void ShowAbout() override
	{
		// not implemented
	}

	virtual void OnEditorNotify(EEditorNotifyEvent aEventId) override
	{
		// empty on purpose
	}

	IEditor* GetIEditor() const
	{
		return m_pEditor;
	}

private:
	IEditor* m_pEditor;
};
