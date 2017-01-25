using System;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace CryEngine.NativeInternals
{
	public static class IConsole
	{
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern public static void AddConsoleCommandFunction(IntPtr consoleCommandFunctionPtr, string functionName, uint nFlags, string sHelp, bool isManaged);
	}
}