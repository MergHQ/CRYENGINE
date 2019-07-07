// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   AsyncPakManager.h (formerly LayerPakManager.h)
//  Version:     v1.00
//  Created:     23/09/2010 by Kenzo.
//  Description: Manage async pak files
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _CRY_LAYER_PAK_MANAGER_H_
#define _CRY_LAYER_PAK_MANAGER_H_

#pragma once

#include <CrySystem/File/IResourceManager.h>
#include <CrySystem/IStreamEngine.h>

struct ICustomMemoryBlock;

class CAsyncPakManager : public IStreamCallback
{
protected:

	struct SAsyncPak
	{
		enum EState
		{
			STATE_UNLOADED,
			STATE_REQUESTED,
			STATE_REQUESTUNLOAD,
			STATE_LOADED,
		};

		enum ELifeTime
		{
			LIFETIME_LOAD_ONLY,
			LIFETIME_LEVEL_COMPLETE,
			LIFETIME_PERMANENT
		};

		string& GetStatus(string&) const;

		string              layername;
		string              filename;
		size_t              nSize              = 0;
		ICustomMemoryBlock* pData              = nullptr;
		EState              eState             = STATE_UNLOADED;
		ELifeTime           eLifeTime          = LIFETIME_LOAD_ONLY;
		bool                bStreaming         = false;
		bool                bPakAlreadyOpen    = false;
		bool                bClosePakOnRelease = false;
		int                 nRequestCount      = 0;
		IReadStreamPtr      pReadStream        = nullptr;
	};
	typedef std::map<string, SAsyncPak> TPakMap;

public:

	CAsyncPakManager();
	~CAsyncPakManager();

	void ParseLayerPaks(const string& levelCachePath);

	bool LoadPakToMemAsync(const char* pPath, bool bLevelLoadOnly);
	void UnloadLevelLoadPaks();
	bool LoadLayerPak(const char* sLayerName);
	void UnloadLayerPak(const char* sLayerName);
	void CancelPendingJobs();

	void GetLayerPakStats(SLayerPakStats& stats, bool bCollectAllStats) const;

	void Clear();
	void Update();

protected:

	bool LoadPak(SAsyncPak& layerPak);

	void StartStreaming(SAsyncPak* pLayerPak);
	void ReleaseData(SAsyncPak* pLayerPak);

	//////////////////////////////////////////////////////////////////////////
	// IStreamCallback interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void  StreamAsyncOnComplete(IReadStream* pStream, unsigned nError);
	virtual void  StreamOnComplete(IReadStream* pStream, unsigned nError);
	virtual void* StreamOnNeedStorage(IReadStream* pStream, unsigned nSize, bool& bAbortOnFailToAlloc);
	//////////////////////////////////////////////////////////////////////////

	TPakMap m_paks;
	size_t  m_nTotalOpenLayerPakSize;
	bool    m_bRequestLayerUpdate;
};

#endif //_CRY_LAYER_PAK_MANAGER_H_
