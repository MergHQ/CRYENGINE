using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CryEngine.Plugins.Compilers
{
	public class CompilationFailedException : Exception
	{
		public CompilationFailedException() { }

		public CompilationFailedException(string errorMessage)
			: base(errorMessage) { }

		public CompilationFailedException(string errorMessage, Exception innerEx)
			: base(errorMessage, innerEx) { }
	}
}
