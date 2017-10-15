// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioSystemEditor.h>
#include <IAudioConnection.h>
#include <IAudioSystemItem.h>
#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>

namespace ACE
{
class IAudioConnectionInspectorPanel;

class CParameterConnection : public IAudioConnection
{
public:

	explicit CParameterConnection(CID const id)
		: IAudioConnection(id)
		, mult(1.0f)
		, shift(0.0f)
	{}

	virtual bool HasProperties() const override { return true; }

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		ar(mult, "mult", "Multiply");
		ar(shift, "shift", "Shift");

		if (ar.isInput())
		{
			signalConnectionChanged();
		}
	}

	float mult;
	float shift;
};

typedef std::shared_ptr<CParameterConnection> ParameterConnectionPtr;

class CStateToParameterConnection : public IAudioConnection
{
public:

	explicit CStateToParameterConnection(CID const id)
		: IAudioConnection(id)
		, value(0.0f)
	{}

	virtual bool HasProperties() const override { return true; }

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		ar(value, "value", "Value");

		if (ar.isInput())
		{
			signalConnectionChanged();
		}
	}

	float value;
};

typedef std::shared_ptr<CStateToParameterConnection> StateConnectionPtr;

class CImplementationSettings_wwise final : public IImplementationSettings
{
public:

	CImplementationSettings_wwise()
		: m_projectPath(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "wwise_project")
		, m_soundBanksPath(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "wwise")
	{}

	virtual char const* GetSoundBanksPath() const { return m_soundBanksPath.c_str(); }
	virtual char const* GetProjectPath() const    { return m_projectPath.c_str(); }
	virtual void        SetProjectPath(char const* szPath);

	void                Serialize(Serialization::IArchive& ar)
	{
		ar(m_projectPath, "projectPath", "Project Path");
	}

private:

	string       m_projectPath;
	string const m_soundBanksPath;
};

class CAudioSystemEditor_wwise final : public IAudioSystemEditor
{
	friend class CAudioWwiseLoader;

public:

	CAudioSystemEditor_wwise();
	~CAudioSystemEditor_wwise();

	// IAudioSystemEditor
	virtual void                     Reload(bool const preserveConnectionStatus = true) override;
	virtual IAudioSystemItem*        GetRoot() override { return &m_rootControl; }
	virtual IAudioSystemItem*        GetControl(CID const id) const override;
	virtual TImplControlTypeMask     GetCompatibleTypes(EItemType const controlType) const override;
	virtual char const*              GetTypeIcon(ItemType const type) const override;
	virtual string                   GetName() const override;
	virtual IImplementationSettings* GetSettings() override { return &m_settings; }
	virtual EItemType                ImplTypeToSystemType(ItemType const itemType) const override;
	virtual ConnectionPtr            CreateConnectionToControl(EItemType const controlType, IAudioSystemItem* const pMiddlewareControl) override;
	virtual ConnectionPtr            CreateConnectionFromXMLNode(XmlNodeRef pNode, EItemType const controlType) override;
	virtual XmlNodeRef               CreateXMLNodeFromConnection(ConnectionPtr const pConnection, EItemType const controlType) override;
	virtual void                     EnableConnection(ConnectionPtr const pConnection) override;
	virtual void                     DisableConnection(ConnectionPtr const pConnection) override;
	// ~IAudioSystemEditor

private:

	void Clear();
	void CreateControlCache(IAudioSystemItem const* const pParent);
	void UpdateConnectedStatus();

	// Generates the ID of the control given its full path name.
	CID GenerateID(string const& controlName, bool isLocalized, IAudioSystemItem* pParent) const;
	// Convenience function to form the full path name.
	// Controls can have the same name if they're under different parents so knowledge of the parent name is needed.
	// Localized controls live in different areas of disk so we also need to know if its localized.
	CID GenerateID(string const& fullPathName) const;

	typedef std::map<CID, IAudioSystemItem*> ControlMap;
	typedef std::map<CID, int> TConnectionsMap;

	IAudioSystemItem              m_rootControl;
	ControlMap                    m_controlsCache;       // cache of the controls stored by id for faster access
	TConnectionsMap               m_connectionsByID;
	CImplementationSettings_wwise m_settings;
};
} // namespace ACE
