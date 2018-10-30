// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   stdafx.h
//  Version:     v1.00
//  Created:     30/9/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: Precompiled Header.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __stdafx_h__
#define __stdafx_h__

#if _MSC_VER > 1000
	#pragma once
#endif

//#define DEFINE_MODULE_NAME "CrySystem"

#include <CryCore/Project/CryModuleDefs.h>
#define eCryModule eCryM_System

#define CRYSYSTEM_EXPORTS

#include <CryCore/Platform/platform.h>
#include <CryCore/Assert/CryAssert.h>

#ifdef USE_PCH

// Ensure included first to prevent windows.h from being included by certain standard library headers, e.g. <future> on Durango
#include <CryCore/Platform/CryWindows.h>

#include <cstdlib>
#include <csignal>
#include <csetjmp>
#include <cstdarg>
#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <bitset>
#include <functional>
#include <utility>
#include <ctime>
#include <chrono>
#include <cstddef>
#include <initializer_list>
#include <tuple>
#include <new>
#include <memory>
#include <scoped_allocator>
#include <climits>
#include <cfloat>
#include <cstdint>
#include <cinttypes>
#include <limits>
#include <exception>
#include <stdexcept>
#include <cassert>
#include <system_error>
#include <cerrno>
#include <cctype>
#include <cwctype>
#include <cstring>
#include <cwchar>
#include <string>
#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <complex>
#include <valarray>
#include <random>
#include <numeric>
#include <ratio>
#include <cfenv>
#include <iosfwd>
#include <ios>
#include <istream>
#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <streambuf>
#include <cstdio>
#include <locale>
#include <clocale>
#include <codecvt>
#include <regex>
#include <atomic>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>
#include <ciso646>

//////////////////////////////////////////////////////////////////////////
// CRT
//////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#if !CRY_PLATFORM_LINUX && !CRY_PLATFORM_ANDROID && !CRY_PLATFORM_APPLE && !CRY_PLATFORM_ORBIS
	#include <memory.h>
	#include <malloc.h>
#endif
#include <fcntl.h>

#if !CRY_PLATFORM_ORBIS && !CRY_PLATFORM_APPLE && !CRY_PLATFORM_ANDROID
	#if CRY_PLATFORM_LINUX
		#include <sys/io.h>
	#else
		#include <io.h>
	#endif
#endif
#include <cmath>

#if CRY_PLATFORM_WINDOWS
	#include <winsock2.h>
	#include <shlobj.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <CryMath/Cry_Math.h>
#include <CryMath/Cry_Camera.h>
#include <CryMath/Random.h>
#include <CryMath/Range.h>
#include <CryMath/Angle.h>
#include <CryMath/ISplines.h>
#include <CryMath/Rotation.h>
#include <CryMath/Transform.h>

#include <CryMemory/CrySizer.h>
#include <CryMemory/AddressHelpers.h>
#include <CryMemory/HeapAllocator.h>
#include <CryMemory/BucketAllocator.h>
#include <CryMemory/STLGlobalAllocator.h>

#include <CryCore/BoostHelpers.h>

#include <CryCore/smartptr.h>
#include <CryCore/CryEnumMacro.h>
#include <CryCore/StlUtils.h>
#include <CryCore/stridedptr.h>
#include <CryCore/functor.h>
#include <CryCore/RingBuffer.h>
#include <CryCore/SmallFunction.h>
#include <CryCore/BitMask.h>
#include <CryCore/CryVariant.h>
#include <CryCore/optional.h>
#include <CryCore/CountedValue.h>
#include <CryCore/CryCustomTypes.h>
#include <CryCore/CryTypeInfo.h>
#include <CryCore/Containers/CryArray.h>
#include <CryCore/Containers/CryFixedArray.h>
#include <CryCore/Containers/CryListenerSet.h>
#include <CryCore/Containers/MiniQueue.h>
#include <CryCore/Containers/VectorSet.h>
#include <CryCore/Containers/VectorMap.h>
#include <CryCore/ToolsHelpers/ResourceCompilerHelper.h>
#include <CryCore/ToolsHelpers/SettingsManagerHelpers.h>
#include <CryCore/TypeInfo_impl.h>

#include <CryString/CryString.h>
#include <CryString/CryFixedString.h>
#include <CryString/CryName.h>
#include <CryString/CryPath.h>

#include <CrySerialization/STL.h>
#include <CrySerialization/DynArray.h>
#include <CrySerialization/Color.h>
#include <CrySerialization/ColorImpl.h>
#include <CrySerialization/Math.h>
#include <CrySerialization/MathImpl.h>

#include <CryExtension/ClassWeaver.h>
#include <CryExtension/Conversion.h>
#include <CryExtension/RegFactoryNode.h>

#include <CryThreading/CryThreadSafeRendererContainer.h>
#include <CryThreading/IJobManager.h> //TODO ideally don't include it, but it defines flags such as JOBMANAGER_SUPPORT_FRAMEPROFILER

#include <CryRenderer/IScaleform.h> // expensive header
#include <CryRenderer/IRenderer.h> // expensive header

#ifdef INCLUDE_SCALEFORM_SDK
	#include <GRefCount.h>
	#include <GMemory.h>
	#include <GMemoryHeap.h>
	#include <GAtomic.h>
	#include <GStats.h>
	#include <GTimer.h>
	#include <GList.h>
	#include <GSysAllocMalloc.h>
	#include <GArray.h>
	#include <GAllocator.h>
	#include <GMath.h>
	#include <GFxPlayerStats.h>
	#include <GColor.h>
	#include <GColorMacros.h>
	#include <GMatrix2D.h>
	#include <GTypes2DF.h>
	#include <GMatrix3D.h>
	#include <GPoint3.h>
	#include <GImage.h>
	#include <GRendererEventHandler.h>
#endif

#endif // USE_PCH

#endif // __stdafx_h__
