// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

using System;

namespace CryEngine.Compilation
{
	/// <summary>
	/// Exception indicating that compiling C# source files has failed.
	/// </summary>
	public class CompilationFailedException : Exception
	{
		/// <summary>
		/// Gets the error data.
		/// </summary>
		/// <value>
		/// The error data.
		/// </value>
		public string[] ErrorData { get; }

		/// <summary>
		/// Gets the error count.
		/// </summary>
		/// <value>
		/// The error count.
		/// </value>
		public Int64 ErrorCount { get; }

		/// <summary>
		/// Exception indicating that compiling C# source files has failed.
		/// </summary>
		public CompilationFailedException()
		{
			
		}

		/// <summary>
		/// Exception indicating that compiling C# source files has failed.
		/// </summary>
		/// <param name="errorMessage">The error message.</param>
		/// <param name="errorData">The error data in csv format.</param>
		/// <param name="errorCount">The number of errors.</param>
		public CompilationFailedException(string errorMessage, string[] errorData, Int64 errorCount)
			: base(errorMessage)
		{
			ErrorData = errorData;
			ErrorCount = errorCount;
		}

		/// <summary>
		/// Exception indicating that compiling C# source files has failed.
		/// </summary>
		/// <param name="errorMessage"></param>
		/// <param name="innerEx"></param>
		/// <param name="errorData"></param>
		/// <param name="errorCount"></param>
		public CompilationFailedException(string errorMessage, Exception innerEx, string[] errorData, Int64 errorCount)
			: base(errorMessage, innerEx)
		{
			ErrorData = errorData;
			ErrorCount = errorCount;
		}
	}
}
