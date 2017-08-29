using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;

namespace CryBackend
{
	public static class Utility
	{
		public static Type FindType(string fullName)
		{
			var ret = FindTypes(type => type.FullName == fullName, GetUserAssemblies());
			return ret.FirstOrDefault();
		}

		public static IEnumerable<Type> FindTypes(Type attribType, IEnumerable<Assembly> asms)
		{
			return FindTypes(type => type.GetCustomAttribute(attribType, false) != null, asms);
		}

		public static IEnumerable<Type> FindTypes(Type attribType, params Assembly[] asms)
		{
			return FindTypes(attribType, asms.AsEnumerable());
		}

		public static IEnumerable<Type> FindTypes(Type attribType)
		{
			return FindTypes(attribType, GetUserAssemblies());
		}

		public static IEnumerable<KeyValuePair<Type, TAttrib>> FindTypes<TAttrib>(IEnumerable<Assembly> asms)
			where TAttrib : Attribute
		{
			return from type in FindTypes(typeof(TAttrib), asms)
				   select new KeyValuePair<Type, TAttrib>(type, type.GetCustomAttribute<TAttrib>(false));
		}

		public static IEnumerable<KeyValuePair<Type, TAttrib>> FindTypes<TAttrib>(Assembly asm, params Assembly[] more)
			where TAttrib : Attribute
		{
			return FindTypes<TAttrib>(Enumerable.Repeat(asm, 1).Concat(more));
		}

		public static IEnumerable<KeyValuePair<Type, TAttrib>> FindTypes<TAttrib>()
			where TAttrib : Attribute
		{
			return FindTypes<TAttrib>(GetUserAssemblies());
		}

		public static IEnumerable<Type> FindTypes(Func<Type, bool> condition, IEnumerable<Assembly> asms)
		{
			var ret = from asm in asms
					  from type in asm.GetTypes()
					  where condition(type)
					  select type;
			return ret.Distinct();
		}

		public static IEnumerable<Assembly> LoadAssembliesWithDependencies(IEnumerable<Assembly> asms)
		{
			var loadedAsms = GetUserAssemblies().ToDictionary(asm => asm.GetName(), asm => asm);

			return asms.SelectMany(asm => asm.GetReferencedAssemblies())
				.Union(asms.Select(a => a.GetName())).Distinct()
				.FilterUserAsmNames().Select(name =>
				{
					Assembly asm;
					if (loadedAsms.TryGetValue(name, out asm))
						return asm;

					// TODO: would be nicer if we use ReflectionOnlyLoad, but in that mode one can't use GetCustomAttribute
					// from CustomAttribute, instead, System.Reflection.CustomAttributeData should be used
					try
					{
						return Assembly.Load(name.FullName);
					}
					catch (FileNotFoundException)
					{
						return null;
					}
				})
				.Where(e => e != null);
		}

		public static IEnumerable<Assembly> GetUserAssemblies()
		{
			return AppDomain.CurrentDomain.GetAssemblies().Where(e => !IsSystemAssemblyName(e.GetName()));
		}

		public static IEnumerable<AssemblyName> FilterUserAsmNames(this IEnumerable<AssemblyName> asms)
		{
			foreach (var asm in asms)
			{
				if (IsSystemAssemblyName(asm))
					continue;

				yield return asm;
			}
		}

		public static bool IsSystemAssemblyName(AssemblyName asmName)
		{
			string name = asmName.Name;
			return (0 == string.Compare(name, "mscorlib", true)
			        || name.StartsWith("Microsoft.", StringComparison.InvariantCultureIgnoreCase)
			        || name.StartsWith("System.", StringComparison.InvariantCultureIgnoreCase)
			        || name.StartsWith("Mono", StringComparison.InvariantCultureIgnoreCase)
			        || name == "System"
			);
		}

		public static AssemblyName GetLocalAssemblyName(string assemblyFileName)
		{
			var fullPath = MakeLocalAssemblyPath(assemblyFileName);
			return AssemblyName.GetAssemblyName(fullPath);
		}

		private static string MakeLocalAssemblyPath(string assemblyFileName)
		{
			var cwd = Environment.CurrentDirectory + Path.DirectorySeparatorChar;
			return cwd + (assemblyFileName.EndsWith(".dll") ? assemblyFileName : assemblyFileName + ".dll");
		}

		public static Assembly LoadLocalAssembly(AssemblyName assemblyName)
		{
			return Assembly.Load(assemblyName);
		}

		public static Assembly LoadLocalAssembly(string assemblyFileName)
		{
			string _;
			return LoadLocalAssembly(assemblyFileName, out _);
		}

		public static Assembly LoadLocalAssembly(string assemblyFileName, out string fullPath)
		{
			fullPath = MakeLocalAssemblyPath(assemblyFileName);
			var asmName = AssemblyName.GetAssemblyName(fullPath);
			return Assembly.Load(asmName);
		}

		// TODO: verbose log
		public static void SupressException(this Task task)
		{
			var exc = task.Exception;
			if (exc != null)
				exc.GetBaseException();
		}

		public static bool SupressException<TExc>(this Task task)
			where TExc : Exception
		{
			var exc = task.Exception;
			if (exc != null)
				return exc.GetBaseException() is TExc;

			return true;
		}

		public static bool SupressException<TExc1, TExc2>(this Task task)
			where TExc1 : Exception
			where TExc2 : Exception
		{
			var exc = task.Exception;
			if (exc != null)
			{
				var baseExc = exc.GetBaseException();
				return baseExc is TExc1 || baseExc is TExc2;
			}
			return true;
		}
	}

}
