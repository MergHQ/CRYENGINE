// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryGame/IGameFramework.h>

struct IFlashPlayer;
struct IFlashVariableObject;
struct IUILayout;
struct SUILayoutEvent;

#define DEFAULT_LAYOUT_ID 0
#define INVALID_LAYOUT_ID 0xFFFFFFFF

typedef DynArray<string>         UINameDynArray;
typedef DynArray<SUILayoutEvent> UIEventDynArray;

struct SUILayoutEvent
{
	SUILayoutEvent()
		: commandName("")
		, signalName("")
		, guid("")
		, object("")
		, pLayout(nullptr)
	{};

	~SUILayoutEvent(){};

	UINameDynArray params;
	string         commandName;
	string         signalName;
	string         guid;
	string         object;
	IUILayout*     pLayout;
};

struct IUILayoutListener
{
	virtual ~IUILayoutListener(){}
	virtual void OnUILayoutEvent(const SUILayoutEvent& event) = 0;
	virtual void OnLayoutDeleted(IUILayout* pLayout) = 0;
	virtual void OnLayoutReloaded(IUILayout* pLayout) = 0;
};

typedef std::vector<IUILayoutListener*> UIEventListenerVec;

struct IUIObject
{
	virtual ~IUIObject(){}
	virtual void SetVisible(const bool visible) = 0;
	virtual bool GetVisible() const = 0;
	virtual void SetParam(const char* paramName, const char* paramValue) = 0;
	virtual void SetParam(const char* paramName, const int paramValue) = 0;
	virtual void SetParam(const char* paramName, const float paramValue) = 0;
	virtual void SetParam(const char* paramName, const bool paramValue) = 0;
	virtual void CallFunction(const char* functionName) = 0;
};

struct IUILayout
{
	virtual ~IUILayout(){}
	virtual int           Load() = 0;
	virtual void          Unload() = 0;
	virtual bool          IsLoaded() const = 0;
	virtual void          SetVisible(const bool visible) = 0;
	virtual bool          GetVisible() const = 0;
	virtual const char*   GetName() const = 0;
	virtual IUIObject*    GetObject(const char* objectName) = 0;
	virtual IFlashPlayer* GetPlayer() = 0;
	virtual void          GetUIEvents(UIEventDynArray& uiEvents) const = 0;
	virtual void          RegisterListener(IUILayoutListener* pListener, const char* eventName) = 0;
	virtual void          RegisterListenerWithAllEvents(IUILayoutListener* pListener) = 0;
	virtual void          UnregisterListener(IUILayoutListener* pListener, const char* eventId) = 0;
	virtual void          UnregisterListenerAll(IUILayoutListener* pListener) = 0;
};

struct IInworldUI
{
	virtual ~IInworldUI(){}
	virtual void CreatePanel(const EntityId entityId, const char* material) = 0;
	virtual void SetPanelMaterial(const EntityId entityId, const char* material) = 0;
	virtual void SetPanelPositionOffset(const EntityId entityId, const Vec3 position) = 0;
	virtual void SetPanelRotationOffset(const EntityId entityId, const Quat rotation) = 0;
};

namespace UIFramework
{
struct IUIFramework : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IUIFramework, 0x89F04B15741A40DE, 0x94AD79A8AC3B7419);

	virtual IUILayout*  GetLayout(const char* layoutName, const uint32 layoutId = DEFAULT_LAYOUT_ID) = 0;
	virtual void        GetAllLayoutNames(UINameDynArray& layoutNames) const = 0;
	virtual int         LoadLayout(const char* layoutName) = 0;
	virtual void        UnloadLayout(const char* layoutName, const uint32 layoutId = DEFAULT_LAYOUT_ID) = 0;
	virtual void        SetLoadingThread(const bool bLoadTime) = 0;
	virtual IInworldUI* GetInworldUI() = 0;
	virtual void        Init() = 0;
	virtual void        Clear() = 0;
	virtual void        ScheduleReload() = 0;
	virtual void        RegisterAutoLayoutListener(IUILayoutListener* pListener) = 0;
	virtual void        UnregisterAutoLayoutListener(IUILayoutListener* pListener) = 0;
};

IUIFramework* CreateFramework(IGameFramework* pGameFramework);
}
