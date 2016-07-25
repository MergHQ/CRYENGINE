using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using System.Xml.Serialization;

namespace CryLinkHelper
{
	static public class Settings
	{
		/// <summary>
		/// Gets the service dictionary.
		/// </summary>
		public static Dictionary<String, Service> Services 
		{ 
			get { return m_services; }
		}

		/// <summary>
		/// Gets whether the settings were loaded.
		/// </summary>
		public static bool Loaded 
		{ 
			get; private set; 
		}

		/// <summary>
		/// Gets the sleep time in seconds after a process was started.
		/// </summary>
		public static Int32 StartUpSleep
		{
			get { return m_root.StartUpSleep; } 
		}

		public static bool AutoClose
		{
			get { return m_root.AutoClose; }
		}

		/// <summary>
		/// Gets the absolute filepath of the settings file.
		/// </summary>
		public static String FilePath
		{
			get; private set;
		}

		/// <summary>
		/// Loads the settings from disk if not already loaded.
		/// </summary>
		/// <param name="forceReload">If set to true it will reload the settings from disk.</param>
		/// <returns>True if load was successful.</returns>
		public static bool Load(bool forceReload = false)
		{
			if (forceReload || Loaded == false)
			{
				try
				{
					StreamReader settings = new StreamReader(FilePath);

					XmlSerializer serializer = new XmlSerializer(typeof(Root));
					m_root = (Root)serializer.Deserialize(settings);
					m_services = new Dictionary<String, Service>();

					Initialize();
				}
				catch
				{
					return false;
				}

				Loaded = true;
			}

			return Loaded;
		}

		/// <summary>
		/// Initializes the settings.
		/// </summary>
		private static void Initialize()
		{
			foreach(Service service in m_root.Services)
			{
				if (String.IsNullOrEmpty(service.ProcessName))
					continue;
				if (String.IsNullOrEmpty(service.ExecutablePath))
					continue;
				if (String.IsNullOrEmpty(service.Password))
					continue;
				if (service.Port < 1024 || service.Port > 32767)
					continue;
				if (String.IsNullOrEmpty(service.Host))
					continue;

				m_services.Add(service.ProcessName.ToLower(), service);
			}
		}

		public class Service
		{
			[XmlAttribute(AttributeName = "processName")]
			public String ProcessName;

			[XmlAttribute(AttributeName = "executablePath")]
			public String ExecutablePath;

			[XmlAttribute(AttributeName = "host")]
			public String Host;

			[XmlAttribute(AttributeName = "port")]
			public Int32 Port;

			[XmlAttribute(AttributeName = "password")]
			public String Password;

			public Service()
			{
				Port = 0;
				Host = "127.0.0.1";
			}
		}

		[XmlRoot(ElementName = "CryLinkHelper_Settings")]
		public class Root
		{
			[XmlArray(ElementName = "Services")]
			[XmlArrayItem(ElementName = "Service")]
			public List<Service> Services;

			[XmlElement(ElementName = "WaitForExecutablesToStart")]
			public Int32 StartUpSleep;

			[XmlElement(ElementName = "AutoClose")]
			public bool AutoClose;

			public Root()
			{
				Services = new List<Service>();
				StartUpSleep = 120;
				AutoClose = true;
			}
		}

		static Settings()
		{
			FilePath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) + "\\settings.xml";
		}

		private static Root m_root;
		private static Dictionary<String, Service> m_services;
	}
}
