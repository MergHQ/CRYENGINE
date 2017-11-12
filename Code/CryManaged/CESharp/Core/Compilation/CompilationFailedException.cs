//Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;

namespace CryEngine.Compilation
{
	internal class CompilationFailedException : Exception
	{
		public CompilationFailedException() { }

		public CompilationFailedException(string errorMessage)
			: base(errorMessage) { }

		public CompilationFailedException(string errorMessage, Exception innerEx)
			: base(errorMessage, innerEx) { }
	}
}
