// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#ifdef INCLUDE_SCALEFORM_SDK

	#include "ScaleformTypes.h"

	#include <CrySystem/Scaleform/IScaleformHelper.h>
	#include <CrySystem/ISystem.h>

	#include "SharedResources.h"
	#include "SharedStates.h"
	#include "FlashPlayerInstance.h"

	#if defined(SF_AMP_SERVER)
		#include <Kernel/SF_AmpInterface.h>
		#include <GFx/AMP/Amp_Server.h>
	#endif

namespace Cry {
namespace Scaleform4 {

class CScaleformHelper final : public IScaleformHelper
{
public:
	virtual bool Init() override
	{
		MEMSTAT_CONTEXT(EMemStatContextType::Other, "Init Scaleform Helper");
		CFlashPlayer::InitCVars();
		CSharedFlashPlayerResources::Init();
	#if defined(SF_AMP_SERVER)
		Scaleform::GFx::AMP::Server::Init();
		IScaleformRecording* pRenderer = CSharedFlashPlayerResources::GetAccess().GetRenderer(true);
		GFxAmpServer::GetInstance().SetRenderer(pRenderer);
	#endif
		return true;
	}

	virtual void Destroy() override
	{
	#if defined(SF_AMP_SERVER)
		GFxAmpServer::GetInstance().CloseConnection();
		Scaleform::GFx::AMP::Server::Uninit();
	#endif
		CSharedFlashPlayerResources::Shutdown();
		delete this;
	}

	virtual void SetAmpEnabled(bool bEnabled) override
	{
	#if defined(SF_AMP_SERVER)

		Ptr<Scaleform::GFx::AMP::ServerState> pServerState = *SF_NEW Scaleform::GFx::AMP::ServerState();
		pServerState->StateFlags = GFxAmpServer::GetInstance().GetCurrentState();
		pServerState->ProfileLevel = GFxAmpServer::GetInstance().GetProfileLevel();

		if (bEnabled)
			pServerState->StateFlags &= ~Scaleform::GFx::AMP::Amp_Disabled;
		else
			pServerState->StateFlags |= Scaleform::GFx::AMP::Amp_Disabled;

		GFxAmpServer::GetInstance().UpdateState(pServerState);
	#endif
	}

	virtual void AmpAdvanceFrame() override
	{
	#if defined(SF_AMP_SERVER)
		GFxAmpServer::GetInstance().AdvanceFrame();
	#endif
	}

	virtual void SetTranslatorWordWrappingMode(const char* szLanguage) override
	{
		CryGFxTranslator::GetAccess().SetWordWrappingMode(szLanguage);
	}

	virtual void OnLanguageChanged() override
	{
		CryGFxTranslator::OnLanguageChanged();
	}

	virtual void ResetMeshCache() override
	{
		CSharedFlashPlayerResources::GetAccess().ResetMeshCache();
	}

	virtual std::shared_ptr<IFlashPlayer> CreateFlashPlayerInstance() override
	{
		return std::make_shared<CFlashPlayer>();
	}

	virtual IFlashPlayerBootStrapper* CreateFlashPlayerBootStrapper() override
	{
		return CFlashPlayer::CreateBootstrapper();
	}

	virtual void RenderFlashInfo() override
	{
		CFlashPlayer::RenderFlashInfo();
	}

	virtual void GetFlashMemoryUsage(ICrySizer* pSizer) override
	{
		CSharedFlashPlayerResources::GetAccess().GetMemoryUsage(pSizer);
	}

	virtual void SetFlashLoadMovieHandler(IFlashLoadMovieHandler* pHandler) override
	{
		CFlashPlayer::SetFlashLoadMovieHandler(pHandler);
	}

	virtual float GetFlashProfileResults() override
	{
		float accumTime;
		CFlashPlayer::GetFlashProfileResults(accumTime);
		return accumTime;
	}

	virtual void GetFlashRenderStats(unsigned& numDPs, unsigned int& numTris) override
	{
		numDPs = 0;
		numTris = 0;
	#ifndef _RELEASE
		IScaleformRecording* pFlashRenderer(CSharedFlashPlayerResources::GetAccess().GetRenderer(true));

		if (pFlashRenderer)
		{
			IScaleformRecording::Stats stats;
			pFlashRenderer->GetStats(&stats, false);
			numDPs = stats.Primitives;
			numTris = stats.Triangles;
		}
	#endif
	}

	virtual void SetRenderThreadIDs(threadID main, threadID render) override
	{
	}
};

} // ~Scaleform4 namespace
} // ~Cry namespace

#endif
