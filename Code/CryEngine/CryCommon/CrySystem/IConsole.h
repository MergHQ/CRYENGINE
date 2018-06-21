// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _ICONSOLE_H_
#define _ICONSOLE_H_

#include <CryCore/SFunctor.h>
#include <CryString/CryString.h>
#include <initializer_list>
#include <functional>
#include <type_traits>

struct ConsoleBind;

struct ICVar;
class ITexture;
class ICrySizer;
enum ELoadConfigurationType;

enum class ECVarType
{
	Invalid = 0,
	Int     = 1,
	Float   = 2,
	String  = 3,
	Int64   = 4
};

namespace ECVarTypeHelper
{
	template <typename T>
	constexpr ECVarType GetTypeForT();
	template<>
	constexpr ECVarType GetTypeForT<int>() { return ECVarType::Int; }
	template<>
	constexpr ECVarType GetTypeForT<float>() { return ECVarType::Float; }
	template<>
	constexpr ECVarType GetTypeForT<const char*>() { return ECVarType::String; }
	template<>
	constexpr ECVarType GetTypeForT<int64>() { return ECVarType::Int64; }

	template <typename T>
	constexpr const char* GetNameForT();
	template<>
	constexpr const char* GetNameForT<int>() { return "int"; }
	template<>
	constexpr const char* GetNameForT<float>() { return "float"; }
	template<>
	constexpr const char* GetNameForT<const char*>() { return "string"; }
	
	inline const char* GetNameForECVar(ECVarType type)
	{
		switch (type)
		{
		case ECVarType::Invalid:    return "?";
		case ECVarType::Int:        return "int";
		case ECVarType::Float:      return "float";
		case ECVarType::String:     return "string";
		default: CRY_ASSERT(false); return "?";
		}
	}
}

// Defines for backwards compatibility
#define CVAR_INT     ECVarType::Int
#define CVAR_FLOAT   ECVarType::Float
#define CVAR_STRING  ECVarType::String

#if defined(_RELEASE)
	#define ALLOW_AUDIT_CVARS    0
	#define LOG_CVAR_INFRACTIONS 1
	#define LOG_CVAR_USAGE       0
#else
	#define ALLOW_AUDIT_CVARS    1
	#define LOG_CVAR_INFRACTIONS 1
	#define LOG_CVAR_USAGE       1
#endif // defined(_RELEASE)

#define CVAR_GROUPS_ARE_PRIVILEGED     1                           // allows cvar groups to manipulate const/cheat cvars (turn off to log group cvar manipulation)
#define LOG_CVAR_INFRACTIONS_CALLSTACK (LOG_CVAR_INFRACTIONS && 0) // logs the callstack when certain cvar issues arise; helpful in tracking down where the problem originated

//! If this flag is set during registering a console variable, and the variable exists,
//! then the variable will store its value in memory given by src.
#define CVF_CHANGE_SOURCE (1u << 16)

//! Flags used by ICVar.
enum EVarFlags : uint32
{
	VF_NULL                    = 0x00000000,      //!< Just to have one recognizable spot where the flags are located in the Register call.
	VF_CHEAT                   = 0x00000002,      //!< Stays in the default state when cheats are disabled.
	VF_DEV_ONLY                = 0x00000004,      //!< Cvar is only registered with the console in non release builds.
	VF_DEDI_ONLY               = 0x00000008,      //!< Cvar is only registered with the console in non release or dedicated server builds.
	VF_NET_SYNCED              = 0x00000080,      //!< Synchronised between server and client(s); server is authoritative.
	VF_DUMPTODISK              = 0x00000100,
	VF_READONLY                = 0x00000800,      //!< Can not be changed by the user.
	VF_REQUIRE_LEVEL_RELOAD    = 0x00001000,
	VF_REQUIRE_APP_RESTART     = 0x00002000,
	VF_WARNING_NOTUSED         = 0x00004000,      //!< Shows warning that this var was not used in config file.
	VF_COPYNAME                = 0x00008000,      //!< Otherwise the const char * to the name will be stored without copying the memory.
	VF_MODIFIED                = 0x00010000,      //!< Set when variable value modified.
	VF_WASINCONFIG             = 0x00020000,      //!< Set when variable was present in config file.
	VF_BITFIELD                = 0x00040000,      //!< Allow bitfield setting syntax.
	VF_RESTRICTEDMODE          = 0x00080000,      //!< Is visible and usable in restricted (normal user) console mode.
	VF_INVISIBLE               = 0x00100000,      //!< Invisible to the user in console.
	VF_ALWAYSONCHANGE          = 0x00200000,      //!< Always accept variable value and call on change callback even if variable value didnt change.
	VF_BLOCKFRAME              = 0x00400000,      //!< Blocks the execution of console commands for one frame.
	VF_CONST_CVAR              = 0x00800000,      //!< Set if it is a const cvar not to be set inside cfg-files.
	VF_CHEAT_ALWAYS_CHECK      = 0x01000000,      //!< This variable is critical to check in every hash, since it's extremely vulnerable.
	VF_CHEAT_NOCHECK           = 0x02000000,      //!< This variable is set as VF_CHEAT but doesn't have to be checked/hashed since it's harmless to hack.
	VF_SYSSPEC_OVERWRITE       = 0x04000000,      //!< This variable is specified by system.cfg with the intention to overwrite all subsequent settings.
	VF_CVARGRP_IGNOREINREALVAL = 0x08000000,      //!< This variable will be ignored when cvar group's real val is checked (Needed for cvars which are in a group but change in various situations).
	VF_LIVE_CREATE_SYNCED      = 0x10000000,      //!< This variable will be synced with LiveCreate clients.
	VF_RENDERER_CVAR           = 0x20000000,      //!< The update of this variable will be done in render thread.
	VF_DEPRECATED              = 0x40000000,      //!< Deprecated cvars use default values which cannot be modified outside the code.
	VF_EXPERIMENTAL            = 0x80000000,      //!< This variable is used by WIP or experimental feature.
};

struct ICVarDumpSink
{
	// <interfuscator:shuffle>
	virtual ~ICVarDumpSink(){}
	virtual void OnElementFound(ICVar* pCVar) = 0;
	// </interfuscator:shuffle>
};

struct IKeyBindDumpSink
{
	// <interfuscator:shuffle>
	virtual ~IKeyBindDumpSink(){}
	virtual void OnKeyBindFound(const char* sBind, const char* sCommand) = 0;
	// </interfuscator:shuffle>
};

struct IOutputPrintSink
{
	// <interfuscator:shuffle>
	virtual ~IOutputPrintSink(){}
	virtual void Print(const char* inszText) = 0;
	// </interfuscator:shuffle>
};

//! Callback class to derive from when you want to receive callbacks when console var changes.
struct IConsoleVarSink
{
	// <interfuscator:shuffle>
	virtual ~IConsoleVarSink(){}

	//! Called by Console before changing console var value, to validate if var can be changed.
	//! \return true if ok to change value, false if should not change value.
	virtual bool OnBeforeVarChange(ICVar* pVar, const char* sNewValue) = 0;

	//! Called by Console after variable has changed value.
	virtual void OnAfterVarChange(ICVar* pVar) = 0;

	//! Called by Console after variable has been unregistered.
	virtual void OnVarUnregister(ICVar* pVar) = 0;
	// </interfuscator:shuffle>
};

#if defined(GetCommandLine)
	#undef GetCommandLine
#endif

//! Interface to the arguments of the console command.
struct IConsoleCmdArgs
{
	// <interfuscator:shuffle>
	virtual ~IConsoleCmdArgs(){}

	//! Gets number of arguments supplied to the command (including the command itself).
	virtual int GetArgCount() const = 0;

	//! Gets argument by index, nIndex must be in 0 <= nIndex < GetArgCount().
	virtual const char* GetArg(int nIndex) const = 0;

	//! Gets complete command line.
	virtual const char* GetCommandLine() const = 0;
	// </interfuscator:shuffle>
};

//! Interface to the arguments of the console command.
struct IConsoleArgumentAutoComplete
{
	// <interfuscator:shuffle>
	virtual ~IConsoleArgumentAutoComplete(){}

	//! Gets number of matches for the argument to auto complete.
	virtual int GetCount() const = 0;

	//! Gets argument value by index, nIndex must be in 0 <= nIndex < GetCount().
	virtual const char* GetValue(int nIndex) const = 0;
	// </interfuscator:shuffle>
};

//! This a definition of the console command function that can be added to console with AddCommand.
typedef void (* ConsoleCommandFunc)(IConsoleCmdArgs*);

//! This a definition of the callback function that is called when variable change.
typedef void (* ConsoleVarFunc)(ICVar*);

struct IManagedConsoleCommandListener
{
	virtual ~IManagedConsoleCommandListener() {};
	virtual void OnManagedConsoleCommandEvent(const char* commandName, IConsoleCmdArgs* consoleCommandArguments) = 0;
};

namespace Detail { struct ConsoleRegistrationHelper; }

//! Interface to the engine console.
//! The engine console allow to manipulate the internal engine parameters and to invoke commands.
//! This interface allow external modules to integrate their functionalities into the console as commands or variables.
//! \note The console takes advantage of the script engine to store the console variables,
//!		 this mean that all variables visible through script and console.
struct IConsole
{
	// <interfuscator:shuffle>
	virtual ~IConsole(){}

	//! Deletes the console.
	virtual void Release() = 0;

	//! Remove a variable from the console.
	//! \param sVarName Console variable name.
	//! \param bDelete If true, the variable is deleted.
	//! \see ICVar.
	//! \par Example
	//! \include CrySystem/Examples/ConsoleVariable.cpp
	virtual void UnregisterVariable(const char* sVarName, bool bDelete = true) = 0;

	//! Set the y coordinate where the console will stop to scroll when is dropped.
	//! \param value Y in screen coordinates.
	virtual void SetScrollMax(int value) = 0;

	//! Add output sink (clases which are interested in the output) - order is not guaranteed.
	//! \param inpSink Must not be 0 and is not allowed to be added twice.
	virtual void AddOutputPrintSink(IOutputPrintSink* inpSink) = 0;

	//! Remove output sink (clases which are interested in the output) - order is not guaranteed.
	//! \param inpSink Must not be 0 and has to be added before.
	virtual void RemoveOutputPrintSink(IOutputPrintSink* inpSink) = 0;

	//! Show/hide the console.
	//! \param specifies if the window must be (true=show,false=hide).
	//! \param specifies iRequestScrollMax <=0 if not used, otherwise it sets SetScrollMax temporary to the given value.
	virtual void ShowConsole(bool show, const int iRequestScrollMax = -1) = 0;

	//! Dump all console-variables to a callback-interface.
	//! \param Callback Callback-interface which needs to be called for each element.
	virtual void DumpCVars(ICVarDumpSink* pCallback, unsigned int nFlagsFilter = 0) = 0;

	//! Bind a console command to a key.
	//! \param sCmd console command that must be executed.
	//! \param sRes name of the key to invoke the command.
	virtual void CreateKeyBind(const char* sCmd, const char* sRes) = 0;

	//! Sets the background-image.
	//! \param pImage background-image.
	virtual void SetImage(ITexture* pImage, bool bDeleteCurrent) = 0;

	//! Retrieves the background-image.
	//! \return background-image.
	virtual ITexture* GetImage() = 0;

	//! Sets static/scroll background-mode.
	//! \param bStatic true if static.
	virtual void StaticBackground(bool bStatic) = 0;

	//! Sets the loading-background-image.
	//! \param pImage Background image.
	virtual void SetLoadingImage(const char* szFilename) = 0;

	//! Iterate through the lines - used for dedicated server (truncated if needed).
	//! \param indwLineNo 0.. counted from the last printed line on.
	//! \param outszBuffer Pointer to the destination string buffer (zero terminated afterwards), must not be 0.
	//! \param indwBufferSize 1.. size of the buffer.
	//! \return true if line was returned, false=there are no more lines.

	virtual bool GetLineNo(const int indwLineNo, char* outszBuffer, const int indwBufferSize) const = 0;

	//! \return Current number of lines in the console.
	virtual int GetLineCount() const = 0;

	//! Retrieve a console variable by name - not case sensitive.
	//! \param name variable name.
	//! \return a pointer to the ICVar interface, NULL if is not found.
	//! \see ICVar.
	//! \par Example
	//! \include CrySystem/Examples/ConsoleVariable.cpp
	virtual ICVar* GetCVar(const char* name) = 0;

	//! Print a string in the console and go to the new line.
	//! \param s The string to print.
	virtual void PrintLine(const char* s) = 0;

	//! Append a string in the last console line.
	//! \param s The string to print.
	virtual void PrintLinePlus(const char* s) = 0;

	//! Retrieve the status of the console (active/not active).
	//! \return the variable value(true = active/false = not active).
	virtual bool GetStatus() = 0;

	//! Clear the console text.
	virtual void Clear() = 0;

	//! Update the console.
	virtual void Update() = 0;

	//! Draw the console.
	virtual void Draw() = 0;

	virtual void RegisterListener(IManagedConsoleCommandListener* pListener, const char* name) = 0;
	virtual void UnregisterListener(IManagedConsoleCommandListener* pListener) = 0;

	//! Removes a console command which was previously registered with AddCommand.
	//! \param sCommand Command name.
	//! \par Example
	//! \include CrySystem/Examples/ConsoleCommand.cpp
	virtual void RemoveCommand(const char* sName) = 0;

	//! Execute a string in the console.
	//! \param command Console command e.g. "map testy" - no leading slash
	//! \param bSilentMode true=suppresses log in error case and logging the command to the console
	//! \param bDeferExecution true=the command is stored in special fifo that allows delayed execution by using wait_seconds and wait_frames commands
	//! \par Example
	//! \include CrySystem/Examples/ConsoleCommand.cpp
	virtual void ExecuteString(const char* command, const bool bSilentMode = false, const bool bDeferExecution = false) = 0;

	//! Print a message into the log and abort the execution of the application.
	//! \param message error string to print in the log.
	virtual void Exit(const char* command, ...) PRINTF_PARAMS(2, 3) = 0;

	//! Return true if the console is opened.
	//! \return the variable value(true = opened/false = closed).
	virtual bool IsOpened() = 0;

	//! Auto completion.
	virtual size_t GetNumVars(bool bIncludeCommands = false) const = 0;

	//! \param szPrefix - 0 or prefix e.g. "sys_spec_".
	//! \return Used size.
	//! nListType = 0 get all values, nListTypes=1 return only cvars, nListTypes=2 return only console commands
	virtual size_t      GetSortedVars(const char** pszArray, size_t numItems, const char* szPrefix = 0, int nListTypes = 0) const = 0;
	virtual const char* AutoComplete(const char* substr) = 0;
	virtual const char* AutoCompletePrev(const char* substr) = 0;
	virtual const char* ProcessCompletion(const char* szInputBuffer) = 0;
	virtual void        RegisterAutoComplete(const char* sVarOrCommand, IConsoleArgumentAutoComplete* pArgAutoComplete) = 0;
	virtual void        UnRegisterAutoComplete(const char* sVarOrCommand) = 0;
	virtual void        ResetAutoCompletion() = 0;

	//! Calculation of the memory used by the whole console system.
	virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

	//! Function related to progress bar.
	virtual void ResetProgressBar(int nProgressRange) = 0;

	//! Function related to progress bar.
	virtual void TickProgressBar() = 0;

	//! \param szLine Must not be 0.
	virtual void SetInputLine(const char* szLine) = 0;

	//! Set Console Editable
	virtual void SetReadOnly(bool readonly) = 0;
	virtual bool IsReadOnly() = 0;

	//! Dump all key bindings to a callback-interface.
	//! \param Callback callback-interface which needs to be called for each element.
	virtual void        DumpKeyBinds(IKeyBindDumpSink* pCallback) = 0;
	virtual const char* FindKeyBind(const char* sCmd) const = 0;

	// Hashing of cvars (for anti-cheat). Separates setting of range, calculation,
	// and retrieval of result so calculation can be done at a known safe point
	// (e.g end of frame) when we know cvars won't be modified or in temporary state.

	//! Get Number of cvars that can be hashed.
	virtual int GetNumCheatVars() = 0;

	//! Set the range of cvars.
	virtual void SetCheatVarHashRange(size_t firstVar, size_t lastVar) = 0;

	//! Calculate the hash from current cvars.
	virtual void CalcCheatVarHash() = 0;

	//! Since hash is calculated async, check if it's completed.
	virtual bool IsHashCalculated() = 0;

	//! Get the hash calculated.
	virtual uint64 GetCheatVarHash() = 0;
	virtual void   PrintCheatVars(bool bUseLastHashRange) = 0;
	virtual char*  GetCheatVarAt(uint32 nOffset) = 0;

	//! Console variable sink.
	//! Adds a new console variables sink callback.
	virtual void AddConsoleVarSink(IConsoleVarSink* pSink) = 0;

	//! Removes a console variables sink callback.
	virtual void RemoveConsoleVarSink(IConsoleVarSink* pSink) = 0;

	//! Called by Console before changing console var value, to validate if var can be changed.
	//! \return true if ok to change value, false if should not change value.
	virtual bool OnBeforeVarChange(ICVar* pVar, const char* sNewValue) = 0;

	//! Called by Console after variable has changed value.
	virtual void OnAfterVarChange(ICVar* pVar) = 0;

	//////////////////////////////////////////////////////////////////////////
	// History
	//////////////////////////////////////////////////////////////////////////

	//! \param bUpOrDown true=after pressed "up", false=after pressed "down".
	//! \return 0 if there is no history line or pointer to the null terminated history line.
	virtual const char* GetHistoryElement(const bool bUpOrDown) = 0;

	//! \param szCommand Must not be 0.
	virtual void                   AddCommandToHistory(const char* szCommand) = 0;

	//! Sets the value of a CVar as loaded from a config
	//! Will defer setting of the value until the CVar is registered if it hasn't been already
	virtual void                   LoadConfigVar(const char* sVariable, const char* sValue) = 0;
	//! Executes a command with optional arguments
	//! Will defer setting of the value until the command is registered if it hasn't been already
	virtual void                   LoadConfigCommand(const char* szCommand, const char* szArguments = nullptr) = 0;

	// Sets how to treat calls to LoadConfigVar, return previous configuration type.
	virtual ELoadConfigurationType SetCurrentConfigType(ELoadConfigurationType configType) = 0;

	//! Enable or disable the activation key (tilde by default).
	//! This is useful when user is in a text field and want to be able to enter the default key.
	//! \param BEnable If true, console will show.
	virtual void EnableActivationKey(bool bEnable) = 0;

	//! Save/Load internal state (CVars) - used by LiveCreate.
	virtual void SaveInternalState(struct IDataWriteStream& writer) const = 0;
	virtual void LoadInternalState(struct IDataReadStream& reader) = 0;

	// </interfuscator:shuffle>

#if defined(DEDICATED_SERVER)
	virtual void SetClientDataProbeString(const char* pName, const char* pValue) = 0;
#endif

protected:
	friend struct ConsoleRegistrationHelper;
	friend class CTCVarBase;

	//! Register a new console command.
	//! \param sCommand Command name.
	//! \param func     Pointer to the console command function to be called when command is invoked.
	//! \param nFlags   Bitfield consisting of VF_ flags (e.g. VF_CHEAT).
	//! \param sHelp    Help string, will be displayed when typing in console "command ?".
	//! \par Example
	//! \include CrySystem/Examples/ConsoleCommand.cpp
	virtual void AddCommand(const char* sCommand, ConsoleCommandFunc func, int nFlags = 0, const char* sHelp = NULL, bool bIsManaged = false) = 0;

	//! Register a new console command that execute script function.
	//! EG "Game.Connect(%1)" the symbol "%1" will be replaced with the command parameter 1
	//! writing in the console "connect 127.0.0.1" will invoke Game.Connect("127.0.0.1").
	//! \param sCommand    Command name.
	//! \param sScriptFunc Script function to be executed when command is invoked.
	//! \param nFlags      Bitfield consist of VF_ flags (e.g. VF_CHEAT).
	//! \param sHelp       Help string, will be displayed when typing in console "command ?".
	//! \par Example
	//! \include CrySystem/Examples/ConsoleCommand.cpp
	virtual void AddCommand(const char* sName, const char* sScriptFunc, int nFlags = 0, const char* sHelp = NULL) = 0;

	//! Create a new console variable that store the value in a string.
	//! \param sName Console variable name.
	//! \param sValue Default value.
	//! \param nFlags User defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data).
	//! \param help Help text that is shown when you use "<sName> ?" in the console.
	//! \return Pointer to the interface ICVar.
	virtual ICVar* RegisterString(const char* sName, const char* sValue, int nFlags, const char* help = "", ConsoleVarFunc pChangeFunc = 0) = 0;

	//! Create a new console variable that store the value in a int.
	//! \param sName Console variable name.
	//! \param iValue Default value.
	//! \param nFlags User defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data).
	//! \param Help help text that is shown when you use "<sName> ?" in the console.
	//! \return Pointer to the interface ICVar.
	//! \par Example
	//! \include CrySystem/Examples/ConsoleVariable.cpp
	virtual ICVar* RegisterInt(const char* sName, int iValue, int nFlags, const char* help = "", ConsoleVarFunc pChangeFunc = 0) = 0;

	//! Create a new console variable that store the value in a int64.
	//! \param sName Console variable name.
	//! \param iValue Default value.
	//! \param nFlags Her defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data).
	//! \param help - help text that is shown when you use "<sName> ?" in the console.
	//! \return Pointer to the interface ICVar.
	//! \par Example
	//! \include CrySystem/Examples/ConsoleVariable.cpp
	virtual ICVar* RegisterInt64(const char* sName, int64 iValue, int nFlags, const char* help = "", ConsoleVarFunc pChangeFunc = 0) = 0;

	//! Create a new console variable that store the value in a float.
	//! \param sName Console variable name.
	//! \param fValue Default value.
	//! \param nFlags User defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data).
	//! \param help help text that is shown when you use "<sName> ?" in the console.
	//! \return Pointer to the interface ICVar.
	//! \par Example
	//! \include CrySystem/Examples/ConsoleVariable.cpp
	virtual ICVar* RegisterFloat(const char* sName, float fValue, int nFlags, const char* help = "", ConsoleVarFunc pChangeFunc = 0) = 0;

	//! Create a new console variable that will update the user defined float.
	//! \param sName Console variable name.
	//! \param src Pointer to the memory that will be updated.
	//! \param nFlags User defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data).
	//! \param help Help text that is shown when you use "<sName> ?" in the console.
	//! \param allowModify Allow modification through config vars, prevents missing modifications in release mode.
	//! \return pointer to the interface ICVar.
	//! \par Example
	//! \include CrySystem/Examples/ConsoleVariable.cpp
	virtual ICVar* Register(const char* name, float* src, float defaultvalue, int nFlags = 0, const char* help = "", ConsoleVarFunc pChangeFunc = 0, bool allowModify = true) = 0;

	//! Create a new console variable that will update the user defined float.
	//! \param sName Console variable name.
	//! \param src Pointer to the memory that will be updated.
	//! \param nFlags User defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data).
	//! \param help Help text that is shown when you use "<sName> ?" in the console.
	//! \param allowModify Allow modification through config vars, prevents missing modifications in release mode.
	//! \return pointer to the interface ICVar.
	//! \par Example
	//! \include CrySystem/Examples/ConsoleVariable.cpp
	virtual ICVar* Register(const char* name, int* src, int defaultvalue, int nFlags = 0, const char* help = "", ConsoleVarFunc pChangeFunc = 0, bool allowModify = true) = 0;

	//! Create a new console variable that will update the user defined float.
	//! \param sName Console variable name.
	//! \param src Pointer to the memory that will be updated.
	//! \param nFlags User defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data).
	//! \param help Help text that is shown when you use "<sName> ?" in the console.
	//! \param allowModify Allow modification through config vars, prevents missing modifications in release mode.
	//! \return pointer to the interface ICVar.
	//! \par Example
	//! \include CrySystem/Examples/ConsoleVariable.cpp
	virtual ICVar* Register(const char* name, const char** src, const char* defaultvalue, int nFlags = 0, const char* help = "", ConsoleVarFunc pChangeFunc = 0, bool allowModify = true) = 0;

	//! Registers an existing console variable.
	//! Should only be used with static duration objects, object is never freed.
	//! \param pVar The existing console variable.
	//! \return Pointer to the interface ICVar (that was passed in)
	virtual ICVar* Register(ICVar* pVar) = 0;
};

//! \cond INTERNAL
//! This interface for the remote console.
struct IRemoteConsoleListener
{
	virtual ~IRemoteConsoleListener() {}

	virtual void OnConsoleCommand(const char* cmd)  {};
	virtual void OnGameplayCommand(const char* cmd) {};
};

struct IRemoteConsole
{
	virtual ~IRemoteConsole() {};

	virtual void RegisterConsoleVariables() = 0;
	virtual void UnregisterConsoleVariables() = 0;

	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual bool IsStarted() const = 0;

	virtual void AddLogMessage(const char* log) = 0;
	virtual void AddLogWarning(const char* log) = 0;
	virtual void AddLogError(const char* log) = 0;

	virtual void Update() = 0;

	virtual void RegisterListener(IRemoteConsoleListener* pListener, const char* name) = 0;
	virtual void UnregisterListener(IRemoteConsoleListener* pListener) = 0;
};
//! \endcond

//! This interface is the 1:1 "C++ representation" of a console variable.
//! \note A console variable is accessible in C++ trough this interface and in all
//! scripts as global variable (with the same name of the variable in the console).
struct ICVar
{
	enum EConsoleLogMode
	{
		eCLM_Off,               //!< Off.
		eCLM_ConsoleAndFile,    //!< Normal info to console and file.
		eCLM_FileOnly,          //!< Normal info to file only.
		eCLM_FullInfo           //!< Full info to file only.
	};
	typedef std::function<void(void)> CallbackFunction;

	// <interfuscator:shuffle>
	// TODO make protected;
	virtual ~ICVar() {}

	//! Delete the variable.
	//! \note The variable will automatically unregister itself from the console.
	virtual void Release() = 0;

	//! \return Value of the variable as an integer.
	virtual int GetIVal() const = 0;

	//! \return The 64-bit integer value of the variable.
	virtual int64 GetI64Val() const = 0;

	//! \return The float value of the variable.
	virtual float GetFVal() const = 0;

	//! \note Don't store pointer as multiple calls to this function might return same memory ptr.
	//! \return The string value of the variable.
	virtual const char* GetString() const = 0;

	//! \note Don't store pointer as multiple calls to this function might return same memory ptr.
	//! \return The data probe string value of the variable.
	virtual const char* GetDataProbeString() const = 0;

	//! Set the string value of the variable.
	//! \param s String representation the value.
	virtual void Set(const char* s) = 0;

	//! Force to set the string value of the variable - can only be called from inside code.
	//! \param s String representation the value.
	virtual void ForceSet(const char* s) = 0;

	//! Set the float value of the variable.
	//! \param s Float representation the value.
	virtual void Set(float f) = 0;

	//! Set the int value of the variable.
	//! \param s integer representation the value.
	virtual void Set(int i) = 0;

	//! Set the int64 value of the variable.
	//! \param s integer representation the value.
	virtual void Set(int64 i) = 0;

	//! Sets the value of the variable regardless of type
	//! \param s String representation of the value.
	virtual void SetFromString(const char* szValue) = 0;

	//! Clear the specified bits in the flag field.
	virtual void ClearFlags(int flags) = 0;

	//! Return the variable's flags.
	//! \return the variable's flags.
	virtual int GetFlags() const = 0;

	//! Set the variable's flags.
	virtual int SetFlags(int flags) = 0;

	//! \return the primary variable's type, e.g. ECVarType::Int, ECVarType::Float, ECVarType::String.
	virtual ECVarType GetType() const = 0;

	//! \return The variable's name.
	virtual const char* GetName() const = 0;

	//! \return The variable's help text, can be NULL if no help is available.
	virtual const char* GetHelp() const = 0;

	//! \return true if the variable may be modified in config files.
	virtual bool IsConstCVar() const = 0;

	//! Adds a new on change functor to the list.
	//! It will add from index 1 on (0 is reserved).
	virtual uint64 AddOnChangeFunctor(const SFunctor& pChangeFunctor) = 0;

	//! Adds a new on change callback function to the cvar.
	//! It will add from index 1 on (0 is reserved).
	uint64 AddOnChange(const CallbackFunction &callback) { return AddOnChangeFunctor(SFunctor(callback)); };

	//!  \return The number of registered on change functos.
	virtual uint64 GetNumberOfOnChangeFunctors() const = 0;

	//! Returns the number of registered on change functors.
	virtual const SFunctor& GetOnChangeFunctor(uint64 nFunctorIndex) const = 0;

	//! Removes an on change functor.
	//! \return true if removal was successful.
	virtual bool RemoveOnChangeFunctor(const uint64 nElement) = 0;

	virtual void GetMemoryUsage(class ICrySizer* pSizer) const = 0;

	//! Only useful for CVarGroups, other types return GetIVal().
	//! CVarGroups set multiple other CVars and this function returns
	//! the integer value the CVarGroup should have, when looking at the controlled cvars.
	//! \return Value that would represent the state, -1 if the state cannot be found
	virtual int GetRealIVal() const = 0;

	//! Log difference between expected state and real state. Only useful for CVarGroups.
	virtual void DebugLog(const int iExpectedValue, const EConsoleLogMode mode) const {}
	// </interfuscator:shuffle>

#if defined(DEDICATED_SERVER)
	//! Set the data probe string value of the variable.
	virtual void SetDataProbeString(const char* pDataProbeString) = 0;
#endif

	//! Set the minimum value that is allowed for this CVar. 
	virtual void SetMinValue(int min) = 0;
	virtual void SetMinValue(int64 min) = 0;
	virtual void SetMinValue(float min) = 0;

	//! Set the maximum value that is allowed for this CVar.
	virtual void SetMaxValue(int max) = 0;
	virtual void SetMaxValue(int64 max) = 0;
	virtual void SetMaxValue(float max) = 0;

	//! Set the allowed values for this CVar
	virtual void SetAllowedValues(std::initializer_list<int> values) = 0;
	virtual void SetAllowedValues(std::initializer_list<int64> values) = 0;
	virtual void SetAllowedValues(std::initializer_list<float> values) = 0;
	virtual void SetAllowedValues(std::initializer_list<string> values) = 0;

	//! Indicates whether the console ownes the CVar and should delete it
	virtual bool IsOwnedByConsole() const = 0;
};

struct ScopedConsoleLoadConfigType
{
	ScopedConsoleLoadConfigType(IConsole* pConsole, ELoadConfigurationType configType)
		: m_pConsole(pConsole)
	{
		m_prevType = pConsole->SetCurrentConfigType(configType);
	};
	~ScopedConsoleLoadConfigType() { m_pConsole->SetCurrentConfigType(m_prevType); }
private:
	IConsole*              m_pConsole;
	ELoadConfigurationType m_prevType;
};

//#ifndef SWIG // SWIG does not like template specialisation

class CTCVarBase : public ICVar
{
public:

	virtual ~CTCVarBase()
	{
		if (m_szName && (m_nFlags & VF_COPYNAME))
		{
			delete[] m_szName;
		}
#if defined(DEDICATED_SERVER)
		if (m_pDataProbeString)
		{
			delete[] m_pDataProbeString;
		}
#endif
	}

	virtual void SetConsole(IConsole* pConsole)
	{
		m_pConsole = pConsole;
	}
	
	virtual bool IsOwnedByConsole() const override { return false; }

protected:

	CTCVarBase() {}

	void RegisterBase(IConsole* pConsole, const char* sName, int nFlags, const char* help)
	{
		m_psHelp = help;
		m_pConsole = pConsole;
		m_nFlags = nFlags;
		if (nFlags & VF_COPYNAME)
		{
			char* sCopyName = new char[strlen(sName) + 1];
			strcpy(sCopyName, sName);
			m_szName = sCopyName;
		}
		else
		{
			m_szName = sName;
		}
		pConsole->Register(this);
	}

	virtual void Release() override                { m_pConsole->UnregisterVariable(m_szName, false); }
	virtual void Set(const char* s) override       { CRY_ASSERT(false); return; }
	virtual void Set(float f) override             { CRY_ASSERT(false); return; }
	virtual void Set(int i) override               { CRY_ASSERT(false); return; }
	virtual void Set(int64 i) override             { CRY_ASSERT(false); return; }
	virtual void ForceSet(const char* s) override  { CRY_ASSERT(false); return; }
	virtual int SetFlags(int flags) override       { m_nFlags = flags; return m_nFlags; }
	virtual void SetMinValue(int min) override     { CRY_ASSERT(false); return; }
	virtual void SetMinValue(int64 min) override   { CRY_ASSERT(false); return; }
	virtual void SetMinValue(float min) override   { CRY_ASSERT(false); return; }
	virtual void SetMaxValue(int max) override     { CRY_ASSERT(false); return; }
	virtual void SetMaxValue(int64 max) override   { CRY_ASSERT(false); return; }
	virtual void SetMaxValue(float max) override   { CRY_ASSERT(false); return; }

	virtual int GetIVal() const override           { CRY_ASSERT(false); return 0; }
	virtual int64 GetI64Val() const override       { CRY_ASSERT(false); return 0; }
	virtual float GetFVal() const override         { CRY_ASSERT(false); return 0; }
	virtual const char *GetString() const override { CRY_ASSERT(false); return "NULL"; }
	virtual int GetRealIVal() const override       { return GetIVal(); }
	virtual ECVarType GetType() const override     { CRY_ASSERT(false); return ECVarType::Int; }
	virtual const char* GetName() const override final { return m_szName; }
	virtual const char* GetHelp() const override   { return m_psHelp; }
	virtual int GetFlags() const override          { return m_nFlags; }
	virtual bool IsConstCVar() const override      { return (m_nFlags & VF_CONST_CVAR) != 0; }
	virtual void ClearFlags(int flags) override    { m_nFlags &= ~flags; }

	virtual void SetAllowedValues(std::initializer_list<int> values) override    { CRY_ASSERT(false); return; }
	virtual void SetAllowedValues(std::initializer_list<int64> values) override  { CRY_ASSERT(false); return; }
	virtual void SetAllowedValues(std::initializer_list<float> values) override  { CRY_ASSERT(false); return; }
	virtual void SetAllowedValues(std::initializer_list<string> values) override { CRY_ASSERT(false); return; }
	virtual void GetMemoryUsage(class ICrySizer* pSizer) const override          { CRY_ASSERT(false); return; }
	virtual void DebugLog(const int iExpectedValue, const EConsoleLogMode mode) const override { CRY_ASSERT(false); }

	virtual uint64 AddOnChangeFunctor(const SFunctor& pChangeFunctor) override 
	{ 
		m_cpChangeFunctors.push_back(pChangeFunctor);
		return m_cpChangeFunctors.size() - 1;
	}

	virtual bool RemoveOnChangeFunctor(const uint64 nElement) override 
	{ 
		if (nElement < m_cpChangeFunctors.size())
		{
			m_cpChangeFunctors.erase(m_cpChangeFunctors.begin() + (size_t)nElement);
			return true;
		}
		return false;
	}

	virtual uint64 GetNumberOfOnChangeFunctors() const override 
	{ 
		return m_cpChangeFunctors.size();
	}

	virtual const SFunctor& GetOnChangeFunctor(uint64 nFunctorIndex) const override 
	{ 
		if (nFunctorIndex < m_cpChangeFunctors.size())
		{
			return m_cpChangeFunctors[(uint)nFunctorIndex];
		}
		static SFunctor sDummyFunctor;
		CRY_ASSERT_MESSAGE(false, "[CTCVarBase::GetOnChangeFunctor] Functor index out of range");
		return sDummyFunctor;
	}

	virtual void CallOnChangeFunctions()
	{
		size_t nTotal(m_cpChangeFunctors.size());
		for (size_t nCount(0); nCount < nTotal; ++nCount)
		{
			m_cpChangeFunctors[nCount].Call();
		}
	}

#if defined(DEDICATED_SERVER)
	virtual void SetDataProbeString(const char* pDataProbeString) override
	{
		CRY_ASSERT(m_pDataProbeString == NULL);
		m_pDataProbeString = new char[strlen(pDataProbeString) + 1];
		strcpy(m_pDataProbeString, pDataProbeString);
	}
#endif

	virtual const char* GetDataProbeString() const override
	{
#if defined(DEDICATED_SERVER)
		if (m_pDataProbeString)
		{
			return m_pDataProbeString;
		}
#endif
		return GetString();
	}

protected:

	const char* m_szName = nullptr;
	const char* m_psHelp = nullptr;
#if defined(DEDICATED_SERVER)
	char* m_pDataProbeString = nullptr;
#endif
	int m_nFlags = 0;
	std::vector<SFunctor> m_cpChangeFunctors;
	IConsole* m_pConsole = nullptr;

	int& m_sys_cvar_logging()
	{
		static int m_sys_cvar_logging = 1;
		return m_sys_cvar_logging;
	}
};

#include <CrySystem/ISystem.h> // for CryWarning

template <typename T>
class TCVar;

template<>
class TCVar<int> : public CTCVarBase
{
public:

	TCVar()
		: m_type(ECVarTypeHelper::GetTypeForT<int>())
		, m_min(std::numeric_limits<int>::lowest())
		, m_max(std::numeric_limits<int>::max())
	{}

	virtual ~TCVar()
	{
		m_pConsole->UnregisterVariable(GetName(), false);
	}

	void Register(const char* sName, IConsole* pConsole, int val, int nFlags, const char* help)
	{
		RegisterBase(pConsole, sName, nFlags, help);
		Set(val);
	}

	virtual void SetFromString(const char* s) override { Set(atoi(s)); }
	virtual void Set(int val) override
	{
		if (val == m_val && !(m_nFlags & VF_ALWAYSONCHANGE))
		{
			return;
		}

		if (m_pConsole->OnBeforeVarChange(this, string().Format("%i", val)))
		{
			m_nFlags |= VF_MODIFIED;
			if (!m_allowedValues.empty())
			{
				if (std::find(m_allowedValues.begin(), m_allowedValues.end(), val) != m_allowedValues.end())
				{
					m_val = val;
				}
				else if (m_sys_cvar_logging() > 0)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "'%d' is not a valid value of '%s'", val, GetName());
				}
			}
			else
			{
				if (m_sys_cvar_logging() > 0 && (val > m_max || val < m_min))
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "'%d' is not in the allowed range of '%s' (%d-%d)", val, GetName(), m_min, m_max);
				}
				m_val = std::min(std::max(val, m_min), m_max);
			}
			CallOnChangeFunctions();
			m_pConsole->OnAfterVarChange(this);
		}
	}
	virtual void SetAllowedValues(std::initializer_list<int> allowed) override 
	{ 
		m_allowedValues = allowed;
		if (!m_allowedValues.empty() && std::find(m_allowedValues.begin(), m_allowedValues.end(), m_val) == m_allowedValues.end())
		{
			Set(m_allowedValues[0]);
		}
	}
	virtual void SetMinValue(int min) override { m_min = min; if (m_val < m_min) Set(m_min); }
	virtual void SetMaxValue(int max) override { m_max = max; if (m_val > m_max) Set(m_max); }

	virtual int GetIVal() const override { return m_val; }
	virtual const char* GetString() const override
	{
		static thread_local string str;
		str.Format("%i", m_val);
		return str.c_str();
	}

	virtual ECVarType GetType() const override { return m_type; }

	TCVar<int>& operator=(int val)  { Set(val); return *this; }
	TCVar<int>& operator*=(int val) { Set(m_val * val); return *this; }
	TCVar<int>& operator/=(int val) { Set(m_val / val); return *this; }
	TCVar<int>& operator+=(int val) { Set(m_val + val); return *this; }
	TCVar<int>& operator-=(int val) { Set(m_val - val); return *this; }
	TCVar<int>& operator&=(int val) { Set(m_val & val); return *this; }
	TCVar<int>& operator|=(int val) { Set(m_val | val); return *this; }
	TCVar<int>& operator++()        { Set(m_val + 1); return *this; }
	TCVar<int>& operator--()        { Set(m_val - 1); return *this; }
	operator int() const            { return m_val; }

private:

	const ECVarType m_type;

	int m_val;
	int m_min;
	int m_max;
	std::vector<int> m_allowedValues;
};

template<>
class TCVar<int64> : public CTCVarBase
{
public:

	TCVar()
		: m_type(ECVarTypeHelper::GetTypeForT<int64>())
		, m_min(std::numeric_limits<int64>::lowest())
		, m_max(std::numeric_limits<int64>::max())
	{}

	virtual ~TCVar()
	{
		m_pConsole->UnregisterVariable(GetName(), false);
	}

	void Register(const char* sName, IConsole* pConsole, int64 val, int nFlags, const char* help)
	{
		RegisterBase(pConsole, sName, nFlags, help);
		Set(val);
	}

	virtual void SetFromString(const char* s) override { Set(atoll(s)); }

	virtual void Set(int val) override { Set((int64)val); }
	virtual void Set(int64 val) override
	{
		if (val == m_val && !(m_nFlags & VF_ALWAYSONCHANGE))
		{
			return;
		}

		if (m_pConsole->OnBeforeVarChange(this, string().Format("%i", val)))
		{
			m_nFlags |= VF_MODIFIED;
			if (!m_allowedValues.empty())
			{
				if (std::find(m_allowedValues.begin(), m_allowedValues.end(), val) != m_allowedValues.end())
				{
					m_val = val;
				}
				else if (m_sys_cvar_logging() > 0)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "'%d' is not a valid value of '%s'", val, GetName());
				}
			}
			else
			{
				if (m_sys_cvar_logging() > 0 && (val > m_max || val < m_min))
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "'%d' is not in the allowed range of '%s' (%d-%d)", val, GetName(), m_min, m_max);
				}
				m_val = std::min(std::max(val, m_min), m_max);
			}
			CallOnChangeFunctions();
			m_pConsole->OnAfterVarChange(this);
		}
	}
	virtual void SetAllowedValues(std::initializer_list<int64> allowed) override
	{
		m_allowedValues = allowed;
		if (!m_allowedValues.empty() && std::find(m_allowedValues.begin(), m_allowedValues.end(), m_val) == m_allowedValues.end())
		{
			Set(m_allowedValues[0]);
		}
	}
	virtual void SetMinValue(int64 min) override { m_min = min; if (m_val < m_min) Set(m_min); }
	virtual void SetMaxValue(int64 max) override { m_max = max; if (m_val > m_max) Set(m_max); }

	virtual int GetIVal() const override { return (int)m_val; } // Might eventually want remove this?
	virtual int64 GetI64Val() const override { return m_val; }
	virtual const char* GetString() const override
	{
		static thread_local string str;
		str.Format("%lli", m_val);
		return str.c_str();
	}

	virtual ECVarType GetType() const override { return m_type; }

	TCVar<int64>& operator=(int64 val) { Set(val); return *this; }
	TCVar<int64>& operator*=(int64 val) { Set(m_val * val); return *this; }
	TCVar<int64>& operator/=(int64 val) { Set(m_val / val); return *this; }
	TCVar<int64>& operator+=(int64 val) { Set(m_val + val); return *this; }
	TCVar<int64>& operator-=(int64 val) { Set(m_val - val); return *this; }
	TCVar<int64>& operator&=(int64 val) { Set(m_val & val); return *this; }
	TCVar<int64>& operator|=(int64 val) { Set(m_val | val); return *this; }
	TCVar<int64>& operator++() { Set(m_val + 1); return *this; }
	TCVar<int64>& operator--() { Set(m_val - 1); return *this; }
	operator int64() const { return m_val; }

private:

	const ECVarType m_type;

	int64 m_val;
	int64 m_min;
	int64 m_max;
	std::vector<int64> m_allowedValues;
};


template<>
class TCVar<float> : public CTCVarBase // Final because of virtual call in destructor by UnregisterVariable
{
public:

	TCVar()
		: m_type(ECVarTypeHelper::GetTypeForT<float>())
		, m_min(std::numeric_limits<float>::lowest())
		, m_max(std::numeric_limits<float>::max())
	{}

	virtual ~TCVar() 
	{
		m_pConsole->UnregisterVariable(GetName(), false);
	}

	void Register(const char* sName, IConsole* pConsole, float val, int nFlags, const char* help)
	{
		RegisterBase(pConsole, sName, nFlags, help);
		Set(val);
	}

	virtual void SetFromString(const char* s) override { Set((float)atof(s)); }
	virtual void Set(float val) override 
	{
		if (val == m_val && !(m_nFlags & VF_ALWAYSONCHANGE))
		{
			return;
		}
		if (m_pConsole->OnBeforeVarChange(this, string().Format("%i", val)))
		{
			m_nFlags |= VF_MODIFIED;
			if (m_sys_cvar_logging() > 0 && (val > m_max || val < m_min))
			{
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "'%f' is not in the allowed range of '%s' (%f-%f)", val, GetName(), m_min, m_max);
			}
			m_val = std::min(std::max(val, m_min), m_max);
			CallOnChangeFunctions();
			m_pConsole->OnAfterVarChange(this);
		}
	}
	virtual void SetMinValue(float min) override { m_min = min; if (m_val < m_min) Set(m_min); }
	virtual void SetMaxValue(float max) override { m_max = max; if (m_val > m_max) Set(m_max); }

	virtual float GetFVal() const override { return m_val; }
	virtual const char* GetString() const override
	{
		static thread_local string str;
		str.Format("%.8f", m_val);
		return str.c_str();
	}
	virtual ECVarType GetType() const override { return m_type; }

	TCVar<float>& operator=(float val)  { Set(val);  return *this; }
	TCVar<float>& operator*=(float val) { Set(m_val * val); return *this; }
	TCVar<float>& operator/=(float val) { Set(m_val / val); return *this; }
	TCVar<float>& operator+=(float val) { Set(m_val + val); return *this; }
	TCVar<float>& operator-=(float val) { Set(m_val - val); return *this; }
	operator float() const { return m_val; }

private:

	const ECVarType m_type;

	float m_val;
	float m_min;
	float m_max;
};

template<>
class TCVar<const char*> : public CTCVarBase
{
public:

	TCVar()
		: m_type(ECVarTypeHelper::GetTypeForT<const char*>())
	{}
	virtual ~TCVar() 
	{
		m_pConsole->UnregisterVariable(GetName(), false);
	}

	void Register(const char* sName, IConsole* pConsole, const char* val, int nFlags, const char* help)
	{
		RegisterBase(pConsole, sName, nFlags, help);
		Set(val);
	}

	virtual void SetFromString(const char* s) override { Set(s); }
	virtual void Set(const char* val) override 
	{ 
		if (string(val) == m_val && !(m_nFlags & VF_ALWAYSONCHANGE))
		{
			return;
		}
		if (m_pConsole->OnBeforeVarChange(this, val))
		{
			m_nFlags |= VF_MODIFIED;
			if (m_allowedValues.empty() || (!m_allowedValues.empty() && std::find(m_allowedValues.begin(), m_allowedValues.end(), string(val)) != m_allowedValues.end()))
			{
				m_val = val;
			}
			CallOnChangeFunctions();
			m_pConsole->OnAfterVarChange(this);
		}
	}
	virtual const char* GetString() const override { return m_val.c_str(); }
	virtual void SetAllowedValues(std::initializer_list<string> allowed) override 
	{ 
		m_allowedValues = allowed;
		if (!m_allowedValues.empty() && std::find(m_allowedValues.begin(), m_allowedValues.end(), m_val) == m_allowedValues.end())
		{
			Set(m_allowedValues[0]);
		}
	}
	virtual ECVarType GetType() const override { return m_type; }

	TCVar<const char*>& operator=(const char* val) { Set(val); return *this; }
	operator const string&() { return m_val; }

private:

	const ECVarType m_type;

	string m_val;
	std::vector<string> m_allowedValues;
};

// std overloads to allow for min(int, TCVar<int>) etc for compatibility
namespace std
{
////////// min //////////
template<class WrappedLeft, class Right>
constexpr typename std::common_type<WrappedLeft, Right>::type min(const TCVar<WrappedLeft>& lhs, const Right& rhs)
{
	return ((WrappedLeft)lhs < rhs) ? (WrappedLeft)lhs : rhs;
}

template<class Left, class WrappedRight>
constexpr typename std::common_type<Left, WrappedRight>::type min(const Left& lhs, const TCVar<WrappedRight>& rhs)
{
	return (lhs < (WrappedRight)rhs) ? lhs : (WrappedRight)rhs;
}

template<class T>
constexpr const TCVar<T>& min(const TCVar<T>& lhs, const TCVar<T>& rhs)
{
	return (lhs < rhs) ? lhs : rhs;
}

////////// max //////////
template<class WrappedLeft, class Right>
constexpr typename std::common_type<WrappedLeft, Right>::type max(const TCVar<WrappedLeft>& lhs, const Right& rhs)
{
	return ((WrappedLeft)lhs > rhs) ? (WrappedLeft)lhs : rhs;
}

template<class Left, class WrappedRight>
constexpr typename std::common_type<Left, WrappedRight>::type max(const Left& lhs, const TCVar<WrappedRight>& rhs)
{
	return (lhs > (WrappedRight)rhs) ? lhs : (WrappedRight)rhs;
}

template<class T>
constexpr const TCVar<T>& max(const TCVar<T>& lhs, const TCVar<T>& rhs)
{
	return (lhs > rhs) ? lhs : rhs;
}
}

//#endif // SWIG

class CXConsoleVariableBase : public ICVar
{
public:
	//! constructor
	//! \param pConsole must not be 0
	CXConsoleVariableBase(IConsole* pConsole, const char* szName, int flags, const char* szHelp, bool ownedByConsole)
		: m_szName(const_cast<char*>(szName))
		, m_szHelp(const_cast<char*>(szHelp))
#if defined(DEDICATED_SERVER)
		, m_szDataProbeString(nullptr)
#endif // defined(DEDICATED_SERVER)
		, m_flags(flags)
		, m_pConsole(pConsole)
		, m_ownedByConsole(ownedByConsole)
	{
		CRY_ASSERT_MESSAGE(m_pConsole != nullptr, "Console must always exist");

		if ((flags & VF_COPYNAME) != 0)
		{
			m_szName = new char[strlen(szName) + 1];
			strcpy(m_szName, szName);
		}
	}
	//! destructor
	virtual ~CXConsoleVariableBase()
	{
		Release();

		if ((m_flags & VF_COPYNAME) != 0)
			SAFE_DELETE_ARRAY(m_szName);

#if defined(DEDICATED_SERVER)
		SAFE_DELETE_ARRAY(m_szDataProbeString);
#endif
	}

	// interface ICVar --------------------------------------------------------------------------------------

	virtual void            ClearFlags(int flags) override
	{
		m_flags &= ~flags;
	}
	virtual int             GetFlags() const override
	{
		return m_flags;
	}
	virtual int             SetFlags(int flags) override
	{
		m_flags = flags;
		return m_flags;
	}
	virtual const char*     GetName() const override
	{
		return m_szName;
	}
	virtual const char*     GetHelp() const override
	{
		return m_szHelp;
	}
	virtual void            Release() override
	{
		m_pConsole->UnregisterVariable(m_szName);
	}
	virtual void            ForceSet(const char* szValue) override
	{
		const int excludeFlags = (VF_CHEAT | VF_READONLY | VF_NET_SYNCED);
		const int oldFlags = (m_flags & excludeFlags);
		m_flags &= ~(excludeFlags);
		SetFromString(szValue);

		m_flags |= oldFlags;
	}
	virtual uint64          AddOnChangeFunctor(const SFunctor& pChangeFunctor) override
	{
		m_changeFunctors.push_back(pChangeFunctor);
		return m_changeFunctors.size() - 1;
	}
	virtual bool            RemoveOnChangeFunctor(const uint64 functorIndex) override
	{
		CRY_ASSERT_MESSAGE(functorIndex < m_changeFunctors.size(), "Trying to remove a functor that does not exist");

		if (functorIndex < m_changeFunctors.size())
		{
			m_changeFunctors.erase(m_changeFunctors.begin() + static_cast<size_t>(functorIndex));
			return true;
		}
		return false;
	}

	virtual uint64          GetNumberOfOnChangeFunctors() const override
	{
		return m_changeFunctors.size();
	}
	virtual const SFunctor& GetOnChangeFunctor(uint64 functorIndex) const override
	{
		CRY_ASSERT_MESSAGE(functorIndex < m_changeFunctors.size(), "Trying to get a functor that does not exist");

		static SFunctor dummyFunctor;
		return functorIndex < m_changeFunctors.size() ? m_changeFunctors[static_cast<size_t>(functorIndex)] : dummyFunctor;
	}

	virtual int             GetRealIVal() const override { return GetIVal(); }
	virtual bool            IsConstCVar() const override { return (m_flags & VF_CONST_CVAR) != 0; }
#if defined(DEDICATED_SERVER)
	virtual void            SetDataProbeString(const char* pDataProbeString) override
	{
		CRY_ASSERT(m_szDataProbeString == nullptr);
		m_szDataProbeString = new char[strlen(pDataProbeString) + 1];
		strcpy(m_szDataProbeString, pDataProbeString);
	}
#endif
	virtual const char* GetDataProbeString() const override
	{
#if defined(DEDICATED_SERVER)
		if (m_szDataProbeString)
		{
			return m_szDataProbeString;
		}
#endif
		return GetOwnDataProbeString();
	}

	bool IsOwnedByConsole() const override { return m_ownedByConsole; }

protected: // ------------------------------------------------------------------------------------------

	virtual const char* GetOwnDataProbeString() const
	{
		return GetString();
	}

	void CallOnChangeFunctions()
	{
		for (SFunctor& it : m_changeFunctors)
			it.Call();
	}

	char*                 m_szName;                 // if VF_COPYNAME then this data need to be deleteed, otherwise it's pointer to .dll/.exe

	char*                 m_szHelp;                 // pointer to the help string, might be 0
#if defined(DEDICATED_SERVER)
	char*                 m_szDataProbeString;       // value client is required to have for data probes
#endif
	int                   m_flags;                 // e.g. VF_CHEAT, ...

	std::vector<SFunctor> m_changeFunctors;

	IConsole*             m_pConsole;               // used for the callback OnBeforeVarChange()

	const bool            m_ownedByConsole;
};

template<class T>
class CXNumericConsoleVariable : public CXConsoleVariableBase
{
	using value_type = T;
	using base_type = typename std::remove_reference<value_type>::type;
	static_assert(std::is_arithmetic<base_type>::value, "base_type has to be arithmetic");

public:
	CXNumericConsoleVariable(IConsole* const pConsole, const char* szName, value_type var, int flags, const char* szHelp, bool ownedByConsole)
		: CXConsoleVariableBase(pConsole, szName, flags, szHelp, ownedByConsole)
		, m_value(var) // Assign here first, in case value_type is a reference
		, m_maxValue(std::numeric_limits<base_type>::max())
		, m_minValue(std::numeric_limits<base_type>::lowest())
	{
		m_value = GetCVarOverride(szName, m_value);
	}

	virtual int GetIVal() const override
	{
		return static_cast<int>(m_value);
	}
	virtual int64 GetI64Val() const override
	{
		return static_cast<int64>(m_value);
	}
	virtual float GetFVal() const override
	{
		return static_cast<float>(m_value);
	}
	virtual const char* GetString() const override
	{
		static char szReturnString[256];

		constexpr ECVarType type = GetTypeInternal();
		if (type == ECVarType::Int)
			cry_sprintf(szReturnString, "%d", m_value);
		else if (type == ECVarType::Int64)
			cry_sprintf(szReturnString, "%lli", m_value);
		else if (type == ECVarType::Float)
			cry_sprintf(szReturnString, "%g", m_value);

		return szReturnString;
	}

	virtual void Set(const char* szValue) override
	{
		CRY_ASSERT_MESSAGE(GetType() == ECVarType::String, "Wrong Set() function called. %s", GetName());
		SetInternal<base_type>(szValue);
	}
	virtual void Set(float value) override
	{
		CRY_ASSERT_MESSAGE(GetType() == ECVarType::Float, "Wrong Set() function called. %s", GetName());
		SetInternal(static_cast<base_type>(value));
	}
	virtual void Set(int value) override
	{
		CRY_ASSERT_MESSAGE(GetType() == ECVarType::Int, "Wrong Set() function called. %s", GetName());
		SetInternal(static_cast<base_type>(value));
	}
	virtual void Set(int64 value) override
	{
		CRY_ASSERT_MESSAGE(GetType() == ECVarType::Int64, "Wrong Set() function called. %s", GetName());
		SetInternal(static_cast<base_type>(value));
	}

	virtual void SetFromString(const char* szValue) override
	{
		constexpr ECVarType type = GetTypeInternal();
		if (type == ECVarType::Int)
			Set(TextToInt(szValue, GetIVal(), (GetFlags() | VF_BITFIELD) != 0));
		else if (type == ECVarType::Int64)
			Set(TextToInt64(szValue, GetIVal(), (GetFlags() | VF_BITFIELD) != 0));
		else if (type == ECVarType::Float)
			Set(static_cast<float>(std::atof(szValue)));
	}

	virtual void SetMaxValue(int max) override
	{
		CRY_ASSERT_MESSAGE(GetTypeInternal() == ECVarType::Int, "Wrong type used (%s)", GetName());
		CRY_ASSERT_MESSAGE((m_flags & VF_BITFIELD) == 0, "Trying to set a minimum value on a bitfield (%s)", GetName());
		CRY_ASSERT_MESSAGE(m_allowedValues.empty(), "SetMaxValue() called after SetAllowedValues(), the maximum will be ignored (%s)", GetName());

		m_maxValue = static_cast<base_type>(max);
		SetInternal(m_value);
	}
	virtual void SetMaxValue(int64 max) override
	{
		CRY_ASSERT_MESSAGE(GetTypeInternal() == ECVarType::Int64, "Wrong type used (%s)", GetName());
		CRY_ASSERT_MESSAGE((m_flags & VF_BITFIELD) == 0, "Trying to set a minimum value on a bitfield (%s)", GetName());
		CRY_ASSERT_MESSAGE(m_allowedValues.empty(), "SetMaxValue() called after SetAllowedValues(), the maximum will be ignored (%s)", GetName());

		m_maxValue = static_cast<base_type>(max);
		SetInternal(m_value);
	}
	virtual void SetMaxValue(float max) override
	{
		CRY_ASSERT_MESSAGE(GetTypeInternal() == ECVarType::Float, "Wrong type used (%s)", GetName());
		CRY_ASSERT_MESSAGE((m_flags & VF_BITFIELD) == 0, "Trying to set a minimum value on a bitfield (%s)", GetName());
		CRY_ASSERT_MESSAGE(m_allowedValues.empty(), "SetMaxValue() called after SetAllowedValues(), the maximum will be ignored (%s)", GetName());

		m_maxValue = static_cast<base_type>(max);
		SetInternal(m_value);
	}
	virtual void SetMinValue(int min) override
	{
		CRY_ASSERT_MESSAGE(GetTypeInternal() == ECVarType::Int, "Wrong type used (%s)", GetName());
		CRY_ASSERT_MESSAGE((m_flags & VF_BITFIELD) == 0, "Trying to set a minimum value on a bitfield (%s)", GetName());
		CRY_ASSERT_MESSAGE(m_allowedValues.empty(), "SetMinValue() called after SetAllowedValues(), the minimum will be ignored (%s)", GetName());

		m_minValue = static_cast<base_type>(min);
		SetInternal(m_value);
	}
	virtual void SetMinValue(int64 min) override
	{
		CRY_ASSERT_MESSAGE(GetTypeInternal() == ECVarType::Int64, "Wrong type used (%s)", GetName());
		CRY_ASSERT_MESSAGE((m_flags & VF_BITFIELD) == 0, "Trying to set a minimum value on a bitfield (%s)", GetName());
		CRY_ASSERT_MESSAGE(m_allowedValues.empty(), "SetMinValue() called after SetAllowedValues(), the minimum will be ignored (%s)", GetName());

		m_minValue = static_cast<base_type>(min);
		SetInternal(m_value);
	}
	virtual void SetMinValue(float min) override
	{
		CRY_ASSERT_MESSAGE(GetTypeInternal() == ECVarType::Float, "Wrong type used (%s)", GetName());
		CRY_ASSERT_MESSAGE((m_flags & VF_BITFIELD) == 0, "Trying to set a minimum value on a bitfield (%s)", GetName());
		CRY_ASSERT_MESSAGE(m_allowedValues.empty(), "SetMinValue() called after SetAllowedValues(), the minimum will be ignored (%s)", GetName());

		m_minValue = static_cast<base_type>(min);
		SetInternal(m_value);
	}

	virtual void SetAllowedValues(std::initializer_list<int> values) override
	{
		CRY_ASSERT_MESSAGE(GetTypeInternal() == ECVarType::Int, "Wrong type used");
		SetAllowedValuesInternal(values);
	}
	virtual void SetAllowedValues(std::initializer_list<int64> values) override
	{
		CRY_ASSERT_MESSAGE(GetTypeInternal() == ECVarType::Int64, "Wrong type used");
		SetAllowedValuesInternal(values);
	}
	virtual void SetAllowedValues(std::initializer_list<float> values) override
	{
		CRY_ASSERT_MESSAGE(GetTypeInternal() == ECVarType::Float, "Wrong type used");
		SetAllowedValuesInternal(values);
	}
	virtual void SetAllowedValues(std::initializer_list<string> values) override
	{
		CRY_ASSERT_MESSAGE(GetTypeInternal() == ECVarType::String, "Wrong type used");
	}

	virtual ECVarType GetType() const override
	{
		return GetTypeInternal();
	}

	virtual void GetMemoryUsage(class ICrySizer* pSizer) const override;

protected:
	static constexpr ECVarType GetTypeInternal()
	{
		return std::is_integral<base_type>::value
			? (sizeof(base_type) == sizeof(int) ? ECVarType::Int : ECVarType::Int64)
			: ECVarType::Float;
	}

	static int64 TextToInt64(const char* szText, int64 currentValue, bool isBitfield)
	{
		int64 value = 0;
		if (szText != nullptr)
		{
			char* pEnd;
			if (isBitfield)
			{
				// Bit manipulation.
				if (*szText == '^')
					value = 1LL << std::strtoll(++szText, &pEnd, 10); // Bit number
				else
					value = std::strtoll(szText, &pEnd, 10); // Full number

															 // Check letter codes.
				while ((*pEnd >= 'a' && *pEnd <= 'z') || (*pEnd >= 'A' && *pEnd <= 'Z'))
				{
					value |= AlphaBit64(*pEnd);
					++pEnd;
				}

				if (*pEnd == '+')
					value = currentValue | value;
				else if (*pEnd == '-')
					value = currentValue & ~value;
				else if (*pEnd == '^')
					value = currentValue ^ value;
			}
			else
			{
				value = std::strtoll(szText, &pEnd, 10);
			}
		}

		return value;
	}
	static int TextToInt(const char* szText, int currentValue, bool isBitfield)
	{
		return static_cast<int>(TextToInt64(szText, currentValue, isBitfield));
	}

	template<class U>
	typename std::enable_if<std::is_same<base_type, U>::value>::type SetAllowedValuesInternal(std::initializer_list<U> values)
	{
		CRY_ASSERT_MESSAGE(values.size() == 0 || (m_maxValue == std::numeric_limits<base_type>::max() && m_minValue == std::numeric_limits<base_type>::lowest()),
			"SetAllowedValues() called after SetMinValue() and/or SetMinValue(), minimum and maximum will be ignored (%s)", GetName());
		m_allowedValues = values;
	}
	template<class U>
	typename std::enable_if<!std::is_same<base_type, U>::value>::type SetAllowedValuesInternal(std::initializer_list<U> values)
	{
		CRY_ASSERT_MESSAGE(values.size() == 0 || (m_maxValue == std::numeric_limits<base_type>::max() && m_minValue == std::numeric_limits<base_type>::lowest()),
			"SetAllowedValues() called after SetMinValue() and/or SetMinValue(), minimum and maximum will be ignored (%s)", GetName());

		if (values.size() > 0)
		{
			m_allowedValues.resize(values.size());
			std::transform(values.begin(), values.end(), m_allowedValues.begin(), [](U val) { return static_cast<base_type>(val); });
		}
		else
		{
			m_allowedValues.clear();
		}
	}

	template<class U = base_type>
	typename std::enable_if<std::is_same<U, int>::value>::type SetInternal(const char* szValue)
	{
		static_assert(std::is_same<U, base_type>::value, "You're not supposed to pass in a different template parameter");
		const int value = TextToInt(szValue, m_value, (m_flags & VF_BITFIELD) != 0);
		SetInternal(value);
	}
	template<class U = base_type>
	typename std::enable_if<std::is_same<U, int64>::value>::type SetInternal(const char* szValue)
	{
		static_assert(std::is_same<U, base_type>::value, "You're not supposed to pass in a different template parameter");
		const int64 value = TextToInt64(szValue, m_value, (m_flags & VF_BITFIELD) != 0);
		SetInternal(value);
	}
	template<class U = base_type>
	typename std::enable_if<std::is_same<U, float>::value>::type SetInternal(const char* szValue)
	{
		static_assert(std::is_same<U, base_type>::value, "You're not supposed to pass in a different template parameter");
		const float value = static_cast<float>(std::atof(szValue));
		SetInternal(value);
	}

	void SetInternal(base_type value)
	{
		constexpr ECVarType type = GetTypeInternal();
		if (!m_allowedValues.empty())
		{
			if (std::find(m_allowedValues.cbegin(), m_allowedValues.cend(), value) == m_allowedValues.cend())
			{
				if (ICVar* pCVarLogging = gEnv->pConsole->GetCVar("sys_cvar_logging"))
				{
					if (pCVarLogging->GetIVal() > 0)
					{
						if (type == ECVarType::Int)
							CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[CVARS] '%i' is not a valid value of '%s'", value, GetName());
						else if (type == ECVarType::Int64)
							CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[CVARS] '%lli' is not a valid value of '%s'", value, GetName());
						else if (type == ECVarType::Float)
							CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[CVARS] '%g' is not a valid value of '%s'", value, GetName());
					}
				}
				return;
			}
		}
		else
		{
			if (value > m_maxValue || value < m_minValue)
			{
				if (ICVar* pCVarLogging = gEnv->pConsole->GetCVar("sys_cvar_logging"))
				{
					if (pCVarLogging->GetIVal() > 0)
					{
						if (type == ECVarType::Int)
							CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[CVARS] '%i' is not in the allowed range of '%s' (%i-%i)", value, GetName(), m_minValue, m_maxValue);
						else if (type == ECVarType::Int64)
							CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[CVARS] '%lli' is not in the allowed range of '%s' (%lli-%lli)", value, GetName(), m_minValue, m_maxValue);
						else if (type == ECVarType::Float)
							CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[CVARS] '%g' is not in the allowed range of '%s' (%g-%g)", value, GetName(), m_minValue, m_maxValue);
					}
				}
				value = clamp_tpl(value, m_minValue, m_maxValue);
			}
		}

		if (value == m_value && (m_flags & VF_ALWAYSONCHANGE) == 0)
			return;

		if (m_pConsole->OnBeforeVarChange(this, GetString()))
		{
			if (ICVar* pCVarLogging = gEnv->pConsole->GetCVar("sys_cvar_logging"))
			{
				if (pCVarLogging->GetIVal() >= 2)
				{
					if (type == ECVarType::Int)
						CryLog("[CVARS] '%s' set to %d (was %d)", GetName(), value, m_value);
					else if (type == ECVarType::Int64)
						CryLog("[CVARS] '%s' set to %lli (was %lli)", GetName(), value, m_value);
					else if (type == ECVarType::Float)
						CryLog("[CVARS] '%s' set to %g (was %g)", GetName(), value, m_value);
				}
			}

			m_flags |= VF_MODIFIED;
			m_value = value;

			CallOnChangeFunctions();

			m_pConsole->OnAfterVarChange(this);
		}
	}

	value_type m_value;

	base_type m_maxValue;
	base_type m_minValue;
	std::vector<base_type> m_allowedValues;
};

template<class T>
class CXStringConsoleVariable : public CXConsoleVariableBase
{
	using value_type = T;
	using base_type = typename std::remove_reference<value_type>::type;
	static_assert(std::is_same<base_type, string>::value, "base_type has to be string");

public:
	CXStringConsoleVariable(IConsole* const pConsole, const char* szName, typename base_type::const_pointer defaultValue, int flags, const char* szHelp, bool ownedByConsole)
		: CXConsoleVariableBase(pConsole, szName, flags, szHelp, ownedByConsole)
		, m_value(GetCVarOverride(szName, defaultValue))
		, m_pUserBuffer(nullptr)
	{
		static_assert(!std::is_reference<value_type>::value, "This constructor can only be used for non-reference types.");
	}
	CXStringConsoleVariable(IConsole* const pConsole, const char* szName, typename base_type::const_pointer& userBuffer, typename base_type::const_pointer defaultValue, int nFlags, const char* szHelp, bool ownedByConsole)
		: CXConsoleVariableBase(pConsole, szName, nFlags, szHelp, ownedByConsole)
		, m_value(GetCVarOverride(szName, defaultValue))
		, m_pUserBuffer(&userBuffer)
	{
		static_assert(std::is_reference<value_type>::value, "This constructor can only be used for reference types.");
		*m_pUserBuffer = m_value.c_str();
	}

	virtual int GetIVal() const override
	{
		return atoi(m_value.c_str());
	}
	virtual int64 GetI64Val() const override
	{
		return _atoi64(m_value.c_str());
	}
	virtual float GetFVal() const override
	{
		return static_cast<float>(atof(m_value.c_str()));
	}
	virtual const char* GetString() const override
	{
		return m_value.c_str();
	}

	virtual void Set(const char* szValue) override
	{
		if (szValue == nullptr)
			return;

		if (m_value == szValue && (m_flags & VF_ALWAYSONCHANGE) == 0)
			return;

		if (!m_allowedValues.empty())
		{
			if (std::find_if(m_allowedValues.cbegin(), m_allowedValues.cend(),
				[szValue](const string& allowedValue) { return stricmp(allowedValue.c_str(), szValue) == 0; }) == m_allowedValues.cend())
			{
				if (ICVar* pCVarLogging = gEnv->pConsole->GetCVar("sys_cvar_logging"))
				{
					if (pCVarLogging->GetIVal() > 0)
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[CVARS] '%s' is not a valid value of '%s'", szValue, GetName());
				}
				return;
			}
		}

		if (m_pConsole->OnBeforeVarChange(this, szValue))
		{
			if (ICVar* pCVarLogging = gEnv->pConsole->GetCVar("sys_cvar_logging"))
			{
				if (pCVarLogging->GetIVal() >= 2)
					CryLog("[CVARS] '%s' set to %s (was %s)", GetName(), szValue, m_value.c_str());
			}

			m_flags |= VF_MODIFIED;
			m_value = szValue;
			if (m_pUserBuffer != nullptr)
				*m_pUserBuffer = m_value.c_str();

			CallOnChangeFunctions();

			m_pConsole->OnAfterVarChange(this);
		}
	}
	virtual void Set(float value) override
	{
		CRY_ASSERT_MESSAGE(GetType() == ECVarType::String, "Wrong Set() function called. %s", GetName());

		stack_string temp;
		temp.Format("%g", value);
		Set(temp.c_str());
	}
	virtual void Set(int value) override
	{
		CRY_ASSERT_MESSAGE(GetType() == ECVarType::String, "Wrong Set() function called. %s", GetName());

		stack_string temp;
		temp.Format("%d", value);
		Set(temp.c_str());
	}
	virtual void Set(int64 value) override
	{
		CRY_ASSERT_MESSAGE(GetType() == ECVarType::String, "Wrong Set() function called. %s", GetName());

		stack_string temp;
		temp.Format("%lld", value);
		Set(temp.c_str());
	}

	virtual void SetFromString(const char* szValue) override
	{
		Set(szValue);
	}

	virtual void SetMinValue(int min) override { CRY_ASSERT_MESSAGE(false, "Trying to set a minimum value on a string CVar."); }
	virtual void SetMinValue(int64 min) override { CRY_ASSERT_MESSAGE(false, "Trying to set a minimum value on a string CVar."); }
	virtual void SetMinValue(float min) override { CRY_ASSERT_MESSAGE(false, "Trying to set a minimum value on a string CVar."); }
	virtual void SetMaxValue(int max) override { CRY_ASSERT_MESSAGE(false, "Trying to set a maximum value on a string CVar."); }
	virtual void SetMaxValue(int64 max) override { CRY_ASSERT_MESSAGE(false, "Trying to set a maximum value on a string CVar."); }
	virtual void SetMaxValue(float max) override { CRY_ASSERT_MESSAGE(false, "Trying to set a maximum value on a string CVar."); }

	virtual void SetAllowedValues(std::initializer_list<int> values) override { CRY_ASSERT_MESSAGE(false, "Trying to set allowed int values on a string CVar."); }
	virtual void SetAllowedValues(std::initializer_list<int64> values) override { CRY_ASSERT_MESSAGE(false, "Trying to set allowed int values on a string CVar."); }
	virtual void SetAllowedValues(std::initializer_list<float> values) override { CRY_ASSERT_MESSAGE(false, "Trying to set allowed float values on a string CVar."); }
	virtual void SetAllowedValues(std::initializer_list<string> values) override { m_allowedValues = values; }

	virtual ECVarType GetType() const override { return ECVarType::String; }

	virtual void GetMemoryUsage(class ICrySizer* pSizer) const override;

private:
	base_type m_value;
	typename base_type::const_pointer* const m_pUserBuffer;

	std::vector<base_type> m_allowedValues;
};

using CXConsoleVariableInt = CXNumericConsoleVariable<int>;
using CXConsoleVariableIntRef = CXNumericConsoleVariable<int&>;
using CXConsoleVariableFloat = CXNumericConsoleVariable<float>;
using CXConsoleVariableFloatRef = CXNumericConsoleVariable<float&>;
using CXConsoleVariableInt64 = CXNumericConsoleVariable<int64>;
using CXConsoleVariableString = CXStringConsoleVariable<string>;
using CXConsoleVariableStringRef = CXStringConsoleVariable<string&>;

#include <CryMemory/CrySizer.h>

template<class T>
void CXNumericConsoleVariable<T>::GetMemoryUsage(class ICrySizer* pSizer) const
{
	pSizer->AddObject(this, sizeof(*this));
}

template<class T>
void CXStringConsoleVariable<T>::GetMemoryUsage(class ICrySizer* pSizer) const
{
	pSizer->AddObject(this, sizeof(*this));
}

#endif //_ICONSOLE_H_
