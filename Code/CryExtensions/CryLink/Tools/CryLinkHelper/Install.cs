using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Serialization;

namespace CryLinkHelper
{
	static class Install
	{
		/// <summary>
		/// Installs URI handling to the system.
		/// </summary>
		public static void Uri()
		{
			RegistryKey classRoot = Registry.ClassesRoot;
			classRoot.DeleteSubKeyTree("crylink", false);

			RegistryKey cryLink = classRoot.CreateSubKey("crylink");
			cryLink.SetValue("", "URL:CryLink Protocol");
			cryLink.SetValue("URL Protocol", "", RegistryValueKind.String);

			Assembly executable = Assembly.GetExecutingAssembly();
			using(RegistryKey defaultIcon = cryLink.CreateSubKey("DefaultIcon"))
			{
				defaultIcon.SetValue("", String.Format("\"{0}\",0", executable.Location));
			}

			using (RegistryKey shell = cryLink.CreateSubKey("shell"))
			{
				shell.SetValue("", "open");

				RegistryKey open = shell.CreateSubKey("open");
				RegistryKey command = open.CreateSubKey("command");
				command.SetValue("", String.Format("\"{0}\" -link \"%1\"", executable.Location));
			}
		}

		/// <summary>
		/// Creates the settings file with default settings if it doesn't exist.
		/// </summary>
		/// <param name="forceOverwrite">Forces an overwrite of the settings file.</param>
		public static bool DefaultSettings(bool forceOverwrite = false)
		{
			if (forceOverwrite || !File.Exists(Settings.FilePath))
			{
				Settings.Service editorService = new Settings.Service();
				editorService.ProcessName = "Editor";
				editorService.ExecutablePath = "..\\..\\Bin\\win_x64\\Editor.exe";
				editorService.Port = 1880;
				editorService.Password = "crylink_pw";

				Settings.Root root = new Settings.Root();
				root.Services.Add(editorService);

				XmlSerializer serializer = new XmlSerializer(typeof(Settings.Root));
				serializer.Serialize(new StreamWriter(Settings.FilePath, false, Encoding.UTF8), root);

				return true;
			}

			Console.WriteLine("[Warning] Couldn't create settings file. There is already a 'settings.xml' please delete it and run command again.");
			return false;
		}
	}
}
