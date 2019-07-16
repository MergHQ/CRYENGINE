// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

/*
   While upgrading from SF3 to SF4, it was more beneficial to do typemapping from SF4 types to SF3 types
   in order to keep the SF3/SF4 integration files as similar as possible for easier comparison during bug tracing
 */

#pragma warning(push)
#pragma warning(disable : 4267)   // conversion from 'size_t' to 'unsigned int'
#include <CrySystem/Scaleform/ConfigScaleform.h>
#include <GFxConfig.h>
#include "GFx_Kernel.h"
#include "GFx.h"
#pragma warning(pop)

typedef Scaleform::GFx::Value             GFxValue;
typedef unsigned                          UInt;
typedef int                               SInt;
typedef float                             Float;
typedef Scaleform::GFx::MovieDef          GFxMovieDef;
typedef Scaleform::MemoryHeap             GMemoryHeap;
typedef Scaleform::Thread                 GThread;
typedef Scaleform::GFx::ActionControl     GFxActionControl;
typedef Scaleform::GFx::Movie             GFxMovieView;
typedef Scaleform::GFx::Movie             GFxMovie;
typedef Scaleform::GFx::FileOpener        GFxFileOpener;
typedef Scaleform::File                   GFile;
typedef GFile::FileConstants              GFileConstants;
typedef Scaleform::GFx::URLBuilder        GFxURLBuilder;
typedef Scaleform::String                 GString;
typedef Scaleform::GFx::Clipboard         GFxTextClipboard;
typedef Scaleform::GFx::Translator        GFxTranslator;
typedef Scaleform::Log                    GFxLog;
typedef Scaleform::GFx::FSCommandHandler  GFxFSCommandHandler;
typedef Scaleform::GFx::ExternalInterface GFxExternalInterface;
typedef Scaleform::GFx::UserEventHandler  GFxUserEventHandler;
typedef Scaleform::GFx::ImageCreator      GFxImageCreator;
typedef Scaleform::GFx::ImageCreateInfo   GFxImageCreateInfo;
typedef Scaleform::GFx::ImageInfoBase     GImageInfoBase;
typedef Scaleform::Render::Matrix2F       GMatrix2D;
typedef Scaleform::Render::Matrix3F       GMatrix3D;
typedef Scaleform::GFx::Loader            GFxLoader;
typedef Scaleform::GFx::ParseControl      GFxParseControl;
typedef Scaleform::System                 GSystem;
typedef Scaleform::Render::Texture        GTexture;
typedef Scaleform::GFx::MovieInfo         GFxMovieInfo;
typedef Scaleform::GFx::Color             GColor;
typedef Scaleform::GFx::Viewport          GViewport;
typedef Scaleform::GFx::FunctionHandler   GFxFunctionHandler;
typedef Scaleform::SysAllocPaged          GSysAllocPaged;
typedef Scaleform::SysAllocBase           GSysAllocBase;
typedef Scaleform::SysAllocStatic         GSysAllocStatic;
typedef Scaleform::GFx::MouseEvent        GFxMouseEvent;
typedef Scaleform::GFx::Event             GFxEvent;
typedef Scaleform::GFx::KeyEvent          GFxKeyEvent;
typedef Scaleform::Key                    GFxKey;
typedef Scaleform::GFx::CharEvent         GFxCharEvent;
typedef Scaleform::GFx::Resource          GFxResource;
typedef Scaleform::GFx::ImageResource     GFxImageResource;
typedef Scaleform::Memory                 GMemory;
typedef Scaleform::MemoryFile             GMemoryFile;
typedef Scaleform::GFx::IMEEvent          GFxIMEEvent;
typedef Scaleform::GFx::IMEWin32Event     GFxIMEWin32Event;
typedef Scaleform::Lock                   GLock;
typedef Scaleform::Event                  GEvent;
typedef Scaleform::Alg::MemUtil           GMemUtil;
typedef Scaleform::GFx::TaskManager       GFxTaskManager;
typedef Scaleform::GFx::FileOpenerBase    GFxFileOpenerBase;
typedef Scaleform::Render::HAL            IScaleformRecording;

using Scaleform::UPInt;
using Scaleform::UInt32;
using Scaleform::UInt64;
using Scaleform::SInt16;
using Scaleform::SInt64;
using Scaleform::UByte;
using Scaleform::Render::Image;
using Scaleform::Render::EdgeAAMode;
using Scaleform::Render::EdgeAA_Inherit;
using Scaleform::Render::EdgeAA_On;
using Scaleform::Render::EdgeAA_Off;
using Scaleform::SFwcscpy;
using Scaleform::LogMessage_Error;
using Scaleform::LogMessage_Warning;
using Scaleform::LogMessage_Text;
using Scaleform::Stat_Default_Mem;
using Scaleform::Render::ThreadCommandQueue;
using Scaleform::Render::SingleThreadCommandQueue;

template<class T> using Ptr = Scaleform::Ptr<T>;
template<class T> using GPtr = Scaleform::Ptr<T>;
template<int Stat> using GNewOverrideBase = Scaleform::NewOverrideBase<Stat>;
template<class T> using GArrayDH = Scaleform::ArrayDH<T>;

#define GFC_FX_VERSION_STRING GFX_VERSION_STRING

namespace GRenderer = Scaleform::Render;

#if defined(SF_AMP_SERVER)
typedef Scaleform::AmpServer GFxAmpServer;
#endif

#if defined(USE_GFX_VIDEO)
namespace Scaleform {
namespace GFx {
namespace Video {

class VideoPlayer;
class VideoSoundSystem;
class VideoSound;
class VideoPC;
class VideoXboxOne;
class VideoPS4;
class VideoBase;
class Video;

}
}
}

typedef Scaleform::GFx::Video::VideoPlayer      GFxVideoPlayer;
typedef Scaleform::GFx::Video::VideoSoundSystem GFxVideoSoundSystem;
typedef Scaleform::GFx::Video::VideoSound       GFxVideoSound;
typedef Scaleform::GFx::Video::VideoPC          GFxVideoPC;
typedef Scaleform::GFx::Video::VideoXboxOne     GFxVideoDurango;
typedef Scaleform::GFx::Video::VideoPS4         GFxVideoOrbis;
typedef Scaleform::GFx::Video::VideoBase        GFxVideoBase;
typedef Scaleform::GFx::Video::Video            GFxVideo;

using Scaleform::Stat_Video_Mem;

#endif
