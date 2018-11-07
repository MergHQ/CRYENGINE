// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/PoolObject.h"
#include <CryAudio/IAudioInterfacesCommonData.h>
#include <CrySystem/IStreamEngine.h>
#include <CryMemory/IMemory.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <CrySystem/TimeValue.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
struct IFile;
} // namespace Impl

enum class EFileFlags : EnumFlagsType
{
	None                      = 0,
	Cached                    = BIT(0),
	NotCached                 = BIT(1),
	NotFound                  = BIT(2),
	MemAllocFail              = BIT(3),
	Removable                 = BIT(4),
	Loading                   = BIT(5),
	UseCounted                = BIT(6),
	NeedsResetToManualLoading = BIT(7),
	Localized                 = BIT(8),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EFileFlags);

class CFileEntry final : public CPoolObject<CFileEntry, stl::PSyncNone>
{
public:

	CFileEntry() = delete;
	CFileEntry(CFileEntry const&) = delete;
	CFileEntry(CFileEntry&&) = delete;
	CFileEntry& operator=(CFileEntry const&) = delete;
	CFileEntry& operator=(CFileEntry&&) = delete;

	explicit CFileEntry(char const* const szPath = nullptr, Impl::IFile* const pImplData = nullptr)
		: m_path(szPath)
		, m_size(0)
		, m_useCount(0)
		, m_memoryBlockAlignment(CRY_MEMORY_ALLOCATION_ALIGNMENT)
		, m_flags(EFileFlags::NotFound)
		, m_dataScope(EDataScope::All)
		, m_streamTaskType(eStreamTaskTypeCount)
		, m_pMemoryBlock(nullptr)
		, m_pReadStream(nullptr)
		, m_pImplData(pImplData)
	{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		m_timeCached.SetValue(0);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

	CryFixedStringT<MaxFilePathLength> m_path;
	size_t                             m_size;
	size_t                             m_useCount;
	size_t                             m_memoryBlockAlignment;
	EFileFlags                         m_flags;
	EDataScope                         m_dataScope;
	EStreamTaskType                    m_streamTaskType;
	_smart_ptr<ICustomMemoryBlock>     m_pMemoryBlock;
	IReadStreamPtr                     m_pReadStream;
	Impl::IFile*                       m_pImplData;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CTimeValue m_timeCached;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
