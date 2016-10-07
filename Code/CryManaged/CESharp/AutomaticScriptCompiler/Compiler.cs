using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Reflection;

using CryEngine;
using System.Text.RegularExpressions;

namespace CryEngine.Plugins.Compilers.NET
{
    public class Compiler : ICryEnginePlugin
	{
		string[] defaultNetFrameworkAssemblies;

		public void Initialize()
		{
			defaultNetFrameworkAssemblies = Directory.GetFiles(FileSystem.GlobalAssemblyCacheDirectory, "*.dll", SearchOption.AllDirectories);

			CompileFromSource(CodeDomProvider.CreateProvider("CSharp"), "*.cs");
		}

		public void Shutdown()
		{
		}

        public void OnLevelLoaded()
        {
        }

        public void OnGameStart()
		{
		}

		public void OnGameStop()
		{

		}

		Assembly CompileFromSource(CodeDomProvider provider, string searchPattern)
		{
			var sourceFiles = Directory.GetFiles(FileSystem.DataDirectory, searchPattern, SearchOption.AllDirectories);
			if (sourceFiles.Length == 0)
				return null;
			
			using (provider)
			{
				var compilerParameters = new CompilerParameters();

				compilerParameters.GenerateExecutable = false;

				// Necessary for stack trace line numbers etc
				compilerParameters.IncludeDebugInformation = true;
				compilerParameters.GenerateInMemory = false;

#if RELEASE
			if(!compilationParameters.ForceDebugInformation)
			{
				compilerParameters.GenerateInMemory = true;
				compilerParameters.IncludeDebugInformation = false;
			}
#endif

				if (!compilerParameters.GenerateInMemory)
				{
					compilerParameters.OutputAssembly = Path.GetTempFileName() + ".dll";
				}

				AddReferencedAssembliesForSourceFiles(sourceFiles, ref compilerParameters);
					
				var results = provider.CompileAssemblyFromFile(compilerParameters, sourceFiles);
				if (results.Errors.HasErrors && results.CompiledAssembly != null)
				{
					return results.CompiledAssembly;
				}

				string compilationError = string.Format("Compilation failed; {0} errors: ", results.Errors.Count);

				foreach (CompilerError error in results.Errors)
				{
					compilationError += Environment.NewLine;

					if (!error.ErrorText.Contains("(Location of the symbol related to previous error)"))
						compilationError += string.Format("{0}({1},{2}): {3} {4}: {5}", error.FileName, error.Line, error.Column, error.IsWarning ? "warning" : "error", error.ErrorNumber, error.ErrorText);
					else
						compilationError += "    " + error.ErrorText;
				}

				throw new CompilationFailedException(compilationError);
			}
		}

		void AddReferencedAssembliesForSourceFiles(IEnumerable<string> sourceFilePaths, ref CompilerParameters compilerParameters)
		{
			var currentDomainAssemblies = AppDomain.CurrentDomain.GetAssemblies().Select(x => x.Location).ToArray();
			compilerParameters.ReferencedAssemblies.AddRange(currentDomainAssemblies);

			/*var usedNamespaces = new HashSet<string>();

			foreach (var sourceFilePath in sourceFilePaths)
			{
				// Open the script
				using (var stream = new FileStream(sourceFilePath, FileMode.Open))
				{
					GetNamespacesFromStream(stream, ref usedNamespaces);
				}
			}

			foreach (var foundNamespace in usedNamespaces)
			{
				// TODO: Go through GAC?
			}*/
		}

		protected void GetNamespacesFromStream(Stream stream, ref HashSet<string> namespaces)
		{
			using (var streamReader = new StreamReader(stream))
			{
				string line;

				while ((line = streamReader.ReadLine()) != null)
				{
					//Filter for using statements
					var matches = Regex.Matches(line, @"using ([^;]+);");
					foreach (Match match in matches)
					{
						namespaces.Add(match.Groups[1].Value);
					}
				}
			}
		}
	}
}
