// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;

namespace CryEngine.Compilation
{
	internal static class Compiler
	{
		private static List<Assembly> defaultNetFrameworkAssemblies = new List<Assembly>();

		static Compiler()
		{
			var defaultAssemblyPaths = Directory.GetFiles(Engine.GlobalAssemblyCacheDirectory, "*.dll", SearchOption.AllDirectories);
			defaultNetFrameworkAssemblies.Capacity = defaultAssemblyPaths.Length;

			foreach(var assemblyPath in defaultAssemblyPaths)
			{
				defaultNetFrameworkAssemblies.Add(Assembly.ReflectionOnlyLoadFrom(assemblyPath));
			}
		}

		public static Assembly CompileCSharpSourceFiles(string assemblyPath, string[] sourceFiles, string[] managedPlugins, out string compileException)
		{
			Assembly assembly = null;
			compileException = null;
			try
			{
				assembly = CompileSourceFiles(CodeDomProvider.CreateProvider("CSharp"), assemblyPath, sourceFiles, managedPlugins);
			}
			catch(CompilationFailedException ex)
			{
				compileException = ex.ToString();
			}
			catch(Exception ex)
			{
				throw (ex);
			}
			return assembly;
		}

		public static Assembly CompileSourceFiles(CodeDomProvider provider, string assemblyPath, string[] sourceFiles, string[] managedPlugins)
		{
			using(provider)
			{
				var compilerParameters = new CompilerParameters();

				compilerParameters.GenerateExecutable = false;

				// Necessary for stack trace line numbers etc
				compilerParameters.IncludeDebugInformation = true;
				compilerParameters.GenerateInMemory = false;

#if RELEASE
				compilerParameters.GenerateInMemory = true;
				compilerParameters.IncludeDebugInformation = false;
#endif

				if(!compilerParameters.GenerateInMemory)
				{
					Directory.CreateDirectory(Path.GetDirectoryName(assemblyPath));
					compilerParameters.OutputAssembly = Path.Combine(assemblyPath);
				}

				AddReferencedAssembliesForSourceFiles(managedPlugins, ref compilerParameters);

				var results = provider.CompileAssemblyFromFile(compilerParameters, sourceFiles);
				if(!results.Errors.HasErrors && results.CompiledAssembly != null)
				{
					return results.CompiledAssembly;
				}

				string compilationError = string.Format("Compilation failed; {0} errors: ", results.Errors.Count);

				foreach(CompilerError error in results.Errors)
				{
					compilationError += Environment.NewLine;

					if(!error.ErrorText.Contains("(Location of the symbol related to previous error)"))
						compilationError += string.Format("{0}({1},{2}): {3} {4}: {5}", error.FileName, error.Line, error.Column, error.IsWarning ? "warning" : "error", error.ErrorNumber, error.ErrorText);
					else
						compilationError += "    " + error.ErrorText;
				}

				throw new CompilationFailedException(compilationError);
			}
		}

		private static void AddReferencedAssembliesForSourceFiles(string[] managedPlugins, ref CompilerParameters compilerParameters)
		{
			// Always reference any currently loaded assembly, such CryEngine.Core.dll.
			var currentDomainAssemblies = AppDomain.CurrentDomain.GetAssemblies().Select(x => x.Location).ToArray();
			compilerParameters.ReferencedAssemblies.AddRange(currentDomainAssemblies);

			compilerParameters.ReferencedAssemblies.AddRange(managedPlugins);
		}

		private static void GetNamespacesFromStream(Stream stream, ref HashSet<string> namespaces)
		{
			using(var streamReader = new StreamReader(stream))
			{
				string line;

				while((line = streamReader.ReadLine()) != null)
				{
					//Filter for using statements
					var matches = Regex.Matches(line, @"using ([^;]+);");
					foreach(Match match in matches)
					{
						namespaces.Add(match.Groups[1].Value);
					}
				}
			}
		}
	}
}
