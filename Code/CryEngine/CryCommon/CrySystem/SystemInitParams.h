#pragma once

struct ISystem;
struct ILog;
struct ILogCallback;
struct ISystemUserCallback;
struct IValidator;
struct IOutputPrintSink;

struct SCvarsDefault
{
	SCvarsDefault()
	{
		sz_r_DriverDef = NULL;
	}

	const char* sz_r_DriverDef;
};

//! Structure passed to Init method of ISystem interface.
struct SSystemInitParams
{
	void*                hWnd;
	ILog*                pLog;           //!< You can specify your own ILog to be used by System.
	ILogCallback*        pLogCallback;   //!< You can specify your own ILogCallback to be added on log creation (used by Editor).
	ISystemUserCallback* pUserCallback;
	const char*          sLogFileName;          //!< File name to use for log.
	IValidator*          pValidator;            //!< You can specify different validator object to use by System.
	IOutputPrintSink*    pPrintSync;            //!< Print Sync which can be used to catch all output from engine.
	char                 szSystemCmdLine[2048]; //!< Command line.
	char                 szUserPath[256];       //!< User alias path relative to My Documents folder.
	char                 szBinariesDir[256];

#if !defined(RELEASE)
	bool bEditor = false;                       //!< When running in Editor mode.
#endif

	bool bDedicatedServer;                    //!< When running a dedicated server.
	bool bExecuteCommandLine;                 //!< can be switched of to suppress the feature or do it later during the initialization.
	bool bUIFramework;
	bool bSkipFont;                            //!< Don't load CryFont.dll.
	bool bSkipRenderer;                        //!< Don't load Renderer.
	bool bSkipNetwork;                         //!< Don't create Network.
	bool bSkipLiveCreate;                      //!< Don't create LiveCreate.
	bool bSkipWebsocketServer;                 //!< Don't create the WebSocket server.
	bool bMinimal;                             //!< Don't load banks.
	bool bSkipInput;                           //!< do not load CryInput.
	bool bTesting;                             //!< When running CryUnitTest.
	bool bNoRandom;                            //!< use fixed generator init/seed.
	bool bShaderCacheGen;                      //!< When running in shadercache gen mode.
	bool bUnattendedMode;                      //!< When running as part of a build on build-machines: Prevent popping up of any dialog.

#if CRY_PLATFORM_DURANGO
	Windows::UI::Core::ICoreWindow^ window;
	const EPLM_Event* pLastPLMEvent;
	Windows::ApplicationModel::Activation::IActivatedEventArgs^ pFirstActivatedEventArgs;
#endif

	ISystem* pSystem;                     //!< Pointer to existing ISystem interface, it will be reused if not NULL.
	//! Char szLocalIP[256];              //! local IP address (needed if we have several servers on one machine).
#if CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID || CRY_PLATFORM_APPLE
	void(*pCheckFunc)(void*);             //!< Authentication function (must be set).
#else
	void* pCheckFunc;                     //!< Authentication function (must be set).
#endif

	SCvarsDefault*    pCvarsDefault;      //!< To override the default value of some cvar.

	//! Initialization defaults.
	SSystemInitParams()
	{
		hWnd = nullptr;
		pLog = nullptr;
		pLogCallback = nullptr;
		pUserCallback = nullptr;
		sLogFileName = nullptr;
		pValidator = nullptr;
		pPrintSync = nullptr;
		memset(szSystemCmdLine, 0, sizeof(szSystemCmdLine));
		memset(szUserPath, 0, sizeof(szUserPath));
		memset(szBinariesDir, 0, sizeof(szBinariesDir));
		bDedicatedServer = false;
		bExecuteCommandLine = true;
		bUIFramework = false;
		bSkipFont = false;
		bSkipRenderer = false;
		bSkipNetwork = false;
#if CRY_PLATFORM_WINDOWS
		// create websocket server by default. bear in mind that USE_HTTP_WEBSOCKETS is not defined in release.
		bSkipWebsocketServer = false;
#else
		// CTCPStreamSocket does not support XBOX ONE and PS4 at all, and some of its functionality only seems to support Win32 and 64
		bSkipWebsocketServer = true;
#endif
		bMinimal = false;
		bSkipInput = false;
		bSkipLiveCreate = false;
		bTesting = false;
		bNoRandom = false;
		bShaderCacheGen = false;
		bUnattendedMode = false;

		pSystem = nullptr;
		pCheckFunc = nullptr;

#if CRY_PLATFORM_DURANGO
		pLastPLMEvent = nullptr;
#endif
		pCvarsDefault = nullptr;
	}
};