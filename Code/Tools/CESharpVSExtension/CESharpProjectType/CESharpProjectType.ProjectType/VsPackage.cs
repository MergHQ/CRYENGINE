/***************************************************************************

Copyright (c) Microsoft Corporation. All rights reserved.
This code is licensed under the Visual Studio SDK license terms.
THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.

***************************************************************************/

namespace CESharpProjectType
{
	using System;
	using System.ComponentModel;
	using System.IO;
	using System.Reflection;
	using System.Runtime.InteropServices;
	using EnvDTE;
	using EnvDTE80;
	using Microsoft.VisualStudio.Shell;
	using Microsoft.VisualStudio.Shell.Interop;
	/// <summary>
	/// This class implements the package exposed by this assembly.
	/// </summary>
	/// <remarks>
	/// This package is required if you want to define adds custom commands (ctmenu)
	/// or localized resources for the strings that appear in the New Project and Open Project dialogs.
	/// Creating project extensions or project types does not actually require a VSPackage.
	/// </remarks>
	[PackageRegistration(UseManagedResourcesOnly = true)]
	[Description("CE# Extention Package")]
	[Guid(VsPackage.PackageGuid)]
	[ProvideAutoLoad(UIContextGuids80.SolutionExists)]
	public sealed class VsPackage : Package
	{
		/// <summary>
		/// The GUID for this package.
		/// </summary>
		public const string PackageGuid = "0dd7381a-dd35-4be1-a04b-2e0a8d631a25";

		/// <summary>
		/// The GUID for this project type.  It is unique with the project file extension and
		/// appears under the VS registry hive's Projects key.
		/// </summary>
		public const string ProjectTypeGuid = "E7562513-36BA-4D11-A927-975E5375ED10";

		/// <summary>
		/// The file extension of this project type.  No preceding period.
		/// </summary>
		public const string ProjectExtension = "ceproj";

		/// <summary>
		/// The default namespace this project compiles with, so that manifest
		/// resource names can be calculated for embedded resources.
		/// </summary>
		internal const string DefaultNamespace = "CryEngine.Launcher";

		public static DTE2 DTE { get; private set; }

		private static SolutionEvents _solutionEvents = null;

		private VsOutputPane _outputPane = null;
		private Plugin _plugin;

		protected override void Initialize()
		{
			base.Initialize();
			DTE = (DTE2)GetService(typeof(DTE));
			InstallMetafiles();
			AttachToEvents();
			//CreateMenu();
		}

		/*private void CreateMenu()
		{
			var menuBar = ((CommandBars)DTE.CommandBars)["MenuBar"];
			var toolsControl = menuBar.Controls["Tools"];
			CommandBar ceSharpBar;
			try
			{
				ceSharpBar = ((CommandBars)DTE.CommandBars)["CE#"];
			}
			catch
			{
				var ctrl = menuBar.Controls.Add(MsoControlType.msoControlPopup, 4242, "", toolsControl.Index - 1);
				ctrl.Caption = "CE#";
				ceSharpBar = ((CommandBars)DTE.CommandBars)["CE#"];
			}

			var btn = (CommandBarButton)ceSharpBar.Controls.Add(MsoControlType.msoControlButton);
			btn.Caption = "Attach Mono Debugger";
			btn.Style = MsoButtonStyle.msoButtonCaption;
			btn.Click += new _CommandBarButtonEvents_ClickEventHandler(myStandardCommandBarButton_Click);
		}

		private void myStandardCommandBarButton_Click(CommandBarButton Ctrl, ref bool CancelDefault)
		{
			MessageBox.Show("Button with caption '" + Ctrl.Caption + "' clicked");
		}*/

		public void AttachToEvents()
		{
			_solutionEvents = DTE.Events.SolutionEvents;
			_solutionEvents.Opened += new _dispSolutionEvents_OpenedEventHandler(OnSolutionOpened);
			_solutionEvents.AfterClosing += new _dispSolutionEvents_AfterClosingEventHandler(OnSolutionAfterClosing);
		}

		private void OnSolutionOpened()
		{
			if (_plugin != null)
				return;

			_outputPane = new VsOutputPane();
			_outputPane.Init(DTE);
			VsOutputPane.ClearLog();
			VsOutputPane.FocusOnLog();

			_plugin = new Plugin();
			_plugin.Init(DTE);
		}

		private void OnSolutionAfterClosing()
		{
			_outputPane.Shutdown();
			_outputPane = null;
			_plugin.Shutdown();
			_plugin = null;
		}
		private void InstallMetafiles()
		{
			var localUserDir = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
			var assembly = Assembly.GetExecutingAssembly();
			var targetRoot = localUserDir + "/CustomProjectSystems/CESharpProjectType/";
			Directory.CreateDirectory(targetRoot);
			Directory.CreateDirectory(targetRoot + "/Rules/");

			var rootFiles = new[] 
			{
				"CustomProject.Default.props",
				"CustomProject.props",
				"CustomProject.targets",
				"CustomProjectCs.targets"
			};
			foreach (var url in rootFiles)
			{
				using (Stream stream = assembly.GetManifestResourceStream("CESharpProjectType.Resource.DeployedBuildSystem." + url))
				{
					using (StreamReader reader = new StreamReader(stream))
					{
						string result = reader.ReadToEnd();
						File.WriteAllText(targetRoot + url, result);
					}
				}
			}
			var rulesFiles = new[]
			{
				"assemblyreference",
				"comreference",
				"csharp.browseobject",
				"csharp",
				"CustomPropertyPage",
				"debugger_general",
				"EmbeddedResource",
				"folder",
				"general.browseobject",
				"general",
				"general_file",
				"MonoDebugger",
				"none",
				"ProjectItemsSchema",
				"projectreference",
				"ResolvedAssemblyReference",
				"ResolvedCOMReference",
				"ResolvedProjectReference",
				"resolvedsdkreference",
				"scc",
				"sdkreference"
			};
			foreach (var url in rulesFiles)
			{
				using (Stream stream = assembly.GetManifestResourceStream("CESharpProjectType.Resource.Rules." + url + ".xaml"))
				{
					using (StreamReader reader = new StreamReader(stream))
					{
						string result = reader.ReadToEnd();
						File.WriteAllText(targetRoot + "/Rules/" + url + ".xaml", result);
					}
				}
			}
		}
	}
}
