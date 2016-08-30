using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.IO;
using System.Threading.Tasks;
using System.Windows;
using Microsoft.VisualStudio.ProjectSystem;
using Microsoft.VisualStudio.ProjectSystem.Debuggers;
using Microsoft.VisualStudio.ProjectSystem.Utilities;
using Microsoft.VisualStudio.ProjectSystem.Utilities.DebuggerProviders;
using Microsoft.VisualStudio.ProjectSystem.VS.Debuggers;

namespace CESharpProjectType
{
	[ExportDebugger(MonoDebugger.SchemaName)]
	[AppliesTo(MyUnconfiguredProject.UniqueCapability)]
	public class MonoDebuggerLaunchProvider : DebugLaunchProviderBase
	{
		// TODO: Specify the assembly full name here
		[ExportPropertyXamlRuleDefinition("CESharpProjectType, Version=1.0.0.0, Culture=neutral, PublicKeyToken=9be6e469bc4921f1", "XamlRuleToCode:MonoDebugger.xaml", "Project")]
		[AppliesTo(MyUnconfiguredProject.UniqueCapability)]
		private object DebuggerXaml { get { throw new NotImplementedException(); } }

		private static MonoDebuggerLaunchProvider _instance;

		[ImportingConstructor]
		public MonoDebuggerLaunchProvider(ConfiguredProject configuredProject)
			: base(configuredProject)
		{
			_instance = this;
		}

		/// <summary>
		/// Gets project properties that the debugger needs to launch.
		/// </summary>
		[Import]
		private ProjectProperties DebuggerProperties { get; set; }

		public override async Task<bool> CanLaunchAsync(DebugLaunchOptions launchOptions)
		{
			MonoDebugger debuggerProps = await DebuggerProperties.GetMonoDebuggerPropertiesAsync();
			string commandValue = await debuggerProps.MonoDebuggerCommand.GetEvaluatedValueAtEndAsync();
			await LoadCERootAsync();
			return !string.IsNullOrEmpty(commandValue);
		}

		private static async Task LoadCERootAsync()
		{
			MonoDebugger debuggerProps = await _instance.DebuggerProperties.GetMonoDebuggerPropertiesAsync();
			string command = await debuggerProps.MonoDebuggerCommand.GetEvaluatedValueAtEndAsync();
            Command = Path.GetFullPath(command);
            CommandArguments = await debuggerProps.MonoDebuggerCommandArguments.GetEvaluatedValueAtEndAsync();
		}
		public static string Command { get; private set; }
		public static string CommandArguments { get; private set; }

		public override async Task<IReadOnlyList<IDebugLaunchSettings>> QueryDebugTargetsAsync(DebugLaunchOptions launchOptions)
		{
			DebugLaunchSettings settings = new DebugLaunchSettings(launchOptions);

			// The properties that are available are determined by the property XAML files in your project.
			MonoDebugger debuggerProps = await DebuggerProperties.GetMonoDebuggerPropertiesAsync();
			await LoadCERootAsync();
			string projWorkingDirectory = await debuggerProps.MonoDebuggerWorkingDirectory.GetEvaluatedValueAtEndAsync();
			settings.CurrentDirectory = new FileInfo(Command).Directory.FullName;
			settings.Executable = Command;
			settings.Arguments = CommandArguments;

			// TODO: Specify the right debugger engine
			settings.LaunchOperation = DebugLaunchOperation.CreateProcess;
			settings.LaunchDebugEngineGuid = DebuggerEngines.ManagedOnlyEngine;

			if (!File.Exists(settings.Executable))
				MessageBox.Show("CryEngine executable not found. Open \"Project Properties > Configuration Properties > Debugging\" and set \"Command/CommandArguments\" according to your targeted CryEngine runtime.", 
					"Executable not found", MessageBoxButton.OK, MessageBoxImage.Error);

			return new IDebugLaunchSettings[] { settings };
		}
	}
}
