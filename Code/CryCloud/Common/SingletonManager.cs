using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Runtime.Remoting;
using System.Threading;
using System.Threading.Tasks;
using log4net;

namespace CryBackend
{
	/// <summary>
	/// Mark a implementation class to be a singleton
	/// ...so that the singleton manager knows for what interfaces it can create implementation objects and where
	/// to find the impl class.
	/// Construction dependency chain/tree:
	/// A singleton must express its construction dependency (on other singletons) in its constructor's parameter list.
	/// A singleton should not access the manager for another singleton during construction, that is done automatically
	/// in dependency discovery. A singleton class can only have other registered singleton interfaces as constructor
	/// parameters, otherwise exception will be thrown.
	/// </summary>
	public class SingletonAttribute : Attribute
	{
		public bool IsLazy { get; set; } = true;
	}

	/// <summary>
	/// A very very naive DI/IoC implementation, that holds all the global singletons and itself a global object
	/// SingletonManager is mocking friendly via UnitTest_set(). The recommended remote mocking approach is keeping
	/// SingletonManager local while individual singletons reside in testing AppDomain and be accessed remotely from
	/// Silos.
	/// </summary>
	public interface ISingletonManager
	{
		TInterface Get<TInterface>(string implClassName = null);
		void Rescan();

		/// <summary>
		/// Explicitly set/replace a singleton with a mock for testing
		/// </summary>
		/// <typeparam name="TInterf">The interface of the singleton</typeparam>
		/// <param name="instance">In case of grain/remote mocking the impl type must inherit MarshalByRefObject</param>
		void UnitTest_Set<TInterf>(TInterf instance) where TInterf : class;
	}


	// TODO: extend the capability of the class or replace it with Ninject
	public class SingletonManager : MarshalByRefObject, ISingletonManager
	{
		protected ILog Logger { get; } = LogManager.GetLogger("SingletonMgr");

		// sorry only one impl supported at a time
		// TODO: we can optimize it with static hash table 
		private ReadOnlyDictionary<Type, Type> m_interfToImplMap;
		private readonly ConcurrentDictionary<Type, object> m_singletons;
		private readonly ConcurrentDictionary<Type, object> m_constructingFlags;

		private readonly HashSet<Assembly> m_scannedAsms;

		public SingletonManager()
		{
			m_interfToImplMap = new ReadOnlyDictionary<Type, Type>(new Dictionary<Type, Type>());
			m_singletons = new ConcurrentDictionary<Type, object>();
			m_constructingFlags = new ConcurrentDictionary<Type, object>();
			m_scannedAsms = new HashSet<Assembly>();
		}

		/// <summary>
		/// Initializer
		/// The initializer must be called before Singleton manager is being used, however it can be no earlier than
		/// all assemblies being loaded otherwise it can't discover all existing singletons.
		/// </summary>
		public void Rescan()
		{
			Rescan(Utility.GetUserAssemblies());
		}

		public void Rescan(Assembly asm)
		{
			if( Utility.IsSystemAssemblyName(asm.GetName())
				|| asm.ReflectionOnly
				|| m_scannedAsms.Contains(asm))
				return;

			Logger.InfoFormat("Scanning types from assembly: {0}", asm.FullName);

			m_scannedAsms.Add(asm);

			DoRescan(Utility.FindTypes<SingletonAttribute>(asm).Select(kv => kv.Key));
		}

		public void Rescan(IEnumerable<Assembly> asms)
		{
			foreach (var asm in asms)
				Rescan(asm);
		}

		// TODO: there is an issue with current code it happens when a singleton implements 2 custom interfaces
		private void DoRescan(IEnumerable<Type> types)
		{
			var nonLazyOnes = new List<Type>();
			var interfToImplMap = new List<KeyValuePair<Type, Type>>();

			foreach (var impl in types)
			{
				bool isLazy = impl.GetCustomAttribute<SingletonAttribute>().IsLazy;

				foreach (var interf in impl.GetInterfaces())
				{
					if (interf.FullName.StartsWith("System"))
						continue;

					interfToImplMap.Add(new KeyValuePair<Type, Type>(interf, impl));
					Logger.DebugFormat("Singleton class found: {0} ({1})", impl.Name, interf.Name);

					if (!isLazy)
						nonLazyOnes.Add(interf);
				}
			}

			if (interfToImplMap.Count == 0)
				return;

			do
			{
				var mapRef = m_interfToImplMap;
				var newmap = mapRef.Concat(interfToImplMap)
					.GroupBy(kv => kv.Key)
					// sorry only one impl supported at a time
					.ToDictionary(kv => kv.Key, kv => kv.First().Value);
				var newval = new ReadOnlyDictionary<Type, Type>(newmap);

				if (mapRef == Interlocked.CompareExchange(ref m_interfToImplMap, newval, mapRef))
					break;
			} while (true); 

			// create all the non lazy singletons
			foreach (var t in nonLazyOnes)
			{
				Get(t);
			}
		}

		/// <summary>
		/// Only launcher can use this method
		/// </summary>
		internal void SetSingleton(Type type, object instance)
		{
			var interf2ImplMap = new Dictionary<Type, Type>(m_interfToImplMap);
			AddImplClass(interf2ImplMap, type, instance.GetType(), Logger);
			Interlocked.Exchange(ref m_interfToImplMap, new ReadOnlyDictionary<Type, Type>(interf2ImplMap));

			m_singletons.AddOrUpdate(type, instance, (t, _) =>
			{
				Logger.InfoFormat("SetSingleton is replacing singleton for interface {0}", t.FullName);
				return instance;
			});
		}

		internal void SetSingleton<TInterface>(object instance)
		{
			var type = typeof(TInterface);
			SetSingleton(type, instance);
		}

		public TInterface Get<TInterface>(string implClassName = null)
		{
			var type = typeof(TInterface);
			return (TInterface)Get(type);
		}

		// TODO: generalize the handling of inlined singletons (like Clock)
		private object Get(Type type)
		{
			// hack for inlining Clock class
			if (type.IsClass && type == typeof(IClock))
				return ConstructOrWait(type, type);

			if (!type.IsInterface)
				throw new ArgumentException("TInterface");

			object retval;
			// protective check before creating a new object
			if (m_singletons.TryGetValue(type, out retval))
				return retval;

			return ConstructOrWait(type);
		}

		private object ConstructOrWait(Type type)
		{
			Type implType;
			if (!m_interfToImplMap.TryGetValue(type, out implType))
			{
				throw new NotSupportedException(
					$"Can't find implement class for interface ({type.Name})," +
					" did you forget to mark the class with [Singleton] tag?");
			}

			return ConstructOrWait(type, implType);
		}

		private object ConstructOrWait(Type type, Type implType)
		{
			// if we successfully set the flag, we have the right to construct
			if (m_constructingFlags.TryAdd(type, null))
			{

				var paramTypes = GetParameterTypes(implType).ToArray();
				if (Logger.IsDebugEnabled
					&& paramTypes.Length > 0)
				{
					Logger.DebugFormat("Constructing depended singletons for type {0} ({1}): {2}", implType.Name, type.Name,
						string.Join(",", paramTypes.Select(t => t.Name)));
				}

				var args = BuildArgList(paramTypes);

				return m_singletons.GetOrAdd(type, t =>
				{
					try
					{
						var retval = Activator.CreateInstance(implType, args);
						Logger.DebugFormat("Done constructing singleton of type {0} ({1})", implType.Name, type.Name);
						return retval;
					}
					catch (Exception exc)
					{
						Logger.Error($"Failed to construct singleton of type {implType.Name}.", exc);
						return null;
					}
				});
			}
			else
			{
				// otherwise we should wait for the other thread to finish constructing
				object retval;
				while (!m_singletons.TryGetValue(type, out retval))
				{
					// it is ok to use Thread.Yield even if this function is called in a Task, since this is
					// a "continuous"/non-awaited function the only chance for a race condition is that the task
					// took the constructing flag is in another thread, so we would never suspend ourselve along with
					// that task/thread.
					Thread.Yield();
				}
				return retval;
			}
		}

		private object[] BuildArgList(IEnumerable<Type> paramTypes)
		{
			return (from pt in paramTypes select Get(pt))
				.ToArray();
		}

		// TODO: generalize the handling of inlined singletons (like Clock)
		private static IEnumerable<Type> GetParameterTypes(Type subject)
		{
			// start from the ctor with minimal param list
			foreach (var ctor in subject.GetConstructors().OrderBy(ci => ci.GetParameters().Length))
			{
				var paramTypes = ctor.GetParameters().Select(pi => pi.ParameterType);
				// IClock gets special treatment
				if (paramTypes.All(pt => pt.IsInterface || pt == typeof(IClock)))
					return paramTypes;
			}

			throw new ApplicationException(
				$"Can't find a proper constructor, failed to activate singleton of type: {subject.Name}");
		}

		private static void AddImplClass(Dictionary<Type,Type> m_interfToImplMap, Type ifType, Type implType, ILog logger)
		{
			if (!m_interfToImplMap.ContainsKey(ifType))
				m_interfToImplMap.Add(ifType, implType);
			else
			{
				var oldImplType = m_interfToImplMap[ifType];
				if (oldImplType != implType)
					logger.InfoFormat("SetSingleton is replacing implement class for interface ({0}), from {1} to {2}",
						ifType.FullName, oldImplType, implType);

				m_interfToImplMap[ifType] = implType;
			}
		}

		public void UnitTest_Set<TInterf>(TInterf instance)
			where TInterf : class
		{
			throw new InvalidOperationException();
		}
	}
}
