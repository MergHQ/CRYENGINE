// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/IEngineModule.h>
#include <memory>

class ICrySizer;
struct IFlashPlayer;
struct IFlashPlayerBootStrapper;
struct IFlashLoadMovieHandler;
/*
TODO: IScaleformHelperEngineModule should be removed and both SF3 and SF4 modules should be derived from a common Cry::ScaleformModule::IModule.
Unfortunately this is not possible, until SF3 is completely removed from CrySystem library or until we change "InitializeEngineModule" behaviour.
It is because the "InitializeEngineModule" function ignores provided "dllName", if the searched interface is already present in one of our loaded libraries.
Since SF3 is part of CrySystem, SF3 module interface is always present, and therefore specifying if SF3 or SF4 "dllName" should be used will be ignored.
*/
struct IScaleformHelperEngineModule : public Cry::IDefaultModule
{
	CRYINTERFACE_DECLARE_GUID(IScaleformHelperEngineModule, "3b0e8940-4ac6-4cbb-aa3f-7e13ecb9f871"_cry_guid);
};

namespace Cry {
namespace ScaleformModule {

struct IModule : public Cry::IDefaultModule
{
	CRYINTERFACE_DECLARE_GUID(IModule, "{33D4A1F3-CF74-4D47-9236-D859C0C740C1}"_cry_guid);
};
} // ~ScaleformModule namespace
} // ~Cry namespace

//! Helper for Scaleform-specific function access
struct IScaleformHelper
{
public:
	//! Initialize helper object
	virtual bool Init() = 0;

	//! Destroy helper object
	virtual void Destroy() = 0;

	//! Enables or disables AMP support.
	//! Note: This function does nothing if AMP feature is not compiled in.
	virtual void SetAmpEnabled(bool bEnabled) = 0;

	//! Advance AMP by one frame.
	//! Note: This function does nothing if AMP feature is not compiled in.
	virtual void AmpAdvanceFrame() = 0;

	//! Sets the word-wrapping mode for the specified language.
	virtual void SetTranslatorWordWrappingMode(const char* szLanguage) = 0;

	//! Mark translator as dirty, causing a refresh of localized strings.
	virtual void OnLanguageChanged() = 0;

	//! Reset all cached meshes
	virtual void ResetMeshCache() = 0;

	//! Create new instance of flash player
	virtual std::shared_ptr<IFlashPlayer> CreateFlashPlayerInstance() = 0;

	//! Create new instance of flash player bootstrapper
	virtual IFlashPlayerBootStrapper* CreateFlashPlayerBootStrapper() = 0;

	//! Displays flash information usign AUX renderer
	virtual void RenderFlashInfo() = 0;

	//! Query memory usage of all flash player instances
	virtual void GetFlashMemoryUsage(ICrySizer* pSizer) = 0;

	//! Set handler for call-backs of flash loading
	virtual void SetFlashLoadMovieHandler(IFlashLoadMovieHandler* pHandler) = 0;

	//! Query the total time spent in flash updating
	virtual float GetFlashProfileResults() = 0;

	//! Query the number of draws and triangles used to render flash content this frame
	virtual void GetFlashRenderStats(unsigned& numDPs, unsigned int& numTris) = 0;

	//! Set thread ID values for Scaleform
	virtual void SetRenderThreadIDs(threadID main, threadID render) = 0;
};
