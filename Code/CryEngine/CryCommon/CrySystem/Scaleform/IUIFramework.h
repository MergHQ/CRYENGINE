// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <CryGame/IGameFramework.h>
#include <CrySystem/Scaleform/IFlashPlayer.h>

struct IUILayout;
struct SUILayoutEvent;
struct IUILayoutListener;
struct IUIObject;
struct IInworldUI;

#define DEFAULT_LAYOUT_ID 0
#define INVALID_LAYOUT_ID 0xFFFFFFFF

typedef DynArray<string>         UINameDynArray;
typedef DynArray<SUILayoutEvent> UIEventDynArray;

enum EUINavigate
{
	eUINavigate_None = 0,
	eUINavigate_Up,
	eUINavigate_Down,
	eUINavigate_Left,
	eUINavigate_Right,
	eUINavigate_Confirm,
	eUINavigate_Back
};

struct IUILayout
{
	virtual ~IUILayout(){}
	virtual uint32        Load() = 0;
	virtual void          Unload() = 0;
	virtual bool          IsLoaded() const = 0;
	virtual void          SetVisible(const bool visible) = 0;
	virtual bool          GetVisible() const = 0;
	virtual const char*   GetName() const = 0;
	virtual IUIObject*    GetObject(const char* objectName) = 0;
	virtual IFlashPlayer* GetPlayer() = 0;
	virtual void          GetUIEvents(UIEventDynArray& uiEvents) const = 0;
	virtual uint32        GetId() const = 0;
	virtual void          RegisterListener(IUILayoutListener* pListener, const char* eventName) = 0;
	virtual void          RegisterListenerWithAllEvents(IUILayoutListener* pListener) = 0;
	virtual void          UnregisterListener(IUILayoutListener* pListener, const char* eventId) = 0;
	virtual void          UnregisterListenerAll(IUILayoutListener* pListener) = 0;
	virtual void          GetObjectNames(UINameDynArray& objectNames) const = 0;
	virtual bool          IsDynamicTexture() const = 0;
};

namespace UIFramework
{
struct IUIFramework : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IUIFramework, 0x89F04B15741A40DE, 0x94AD79A8AC3B7419)

	virtual IUILayout * GetLayout(const char* layoutName, const uint32 layoutId = DEFAULT_LAYOUT_ID) = 0;
	virtual void        GetAllLayoutNames(UINameDynArray& layoutNames) const = 0;
	virtual uint32      LoadLayout(const char* layoutName) = 0;
	virtual void        UnloadLayout(const char* layoutName, const uint32 layoutId = DEFAULT_LAYOUT_ID) = 0;
	virtual void        SetLoadingThread(const bool bLoadTime) = 0;
	virtual IInworldUI* GetInworldUI() = 0;
	virtual void        Init() = 0;
	virtual void        Clear() = 0;
	virtual void        ScheduleReload() = 0;

	virtual bool        IsEditing() = 0;

	virtual void        Navigate(const EUINavigate navigate) = 0;

	virtual void        RegisterAutoLayoutListener(IUILayoutListener* pListener) = 0;
	virtual void        UnregisterAutoLayoutListener(IUILayoutListener* pListener) = 0;
};

IUIFramework* CreateFramework(IGameFramework* pGameFramework);
}
