using System;
using System.Collections.Generic;
using System.ComponentModel.Design;
using System.Globalization;
using System.Linq;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;

namespace CryEngine.Debugger.Mono
{
	/// <summary>
	/// Command for launching and debugging the GameLauncher.
	/// </summary>
	internal sealed class LauncherCommands
	{
		public static readonly int[] CommandIds = new int[]
		{
			0x0201,
			0x0202,
			0x0203
		};

		/// <summary>
		/// Command menu group (command set GUID).
		/// </summary>
		public static readonly Guid CommandSet = new Guid("5f0ed1f6-570c-4a93-a2d0-bf15e7054a90");

		/// <summary>
		/// VS Package that provides this command, not null.
		/// </summary>
		private readonly Package _package;

		/// <summary>
		/// Gets the instance of the command.
		/// </summary>
		public static LauncherCommands Instance
		{
			get;
			private set;
		}

		/// <summary>
		/// Gets the service provider from the owner package.
		/// </summary>
		private IServiceProvider ServiceProvider
		{
			get
			{
				return _package;
			}
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="GameLauncherCommand"/> class.
		/// Adds our command handlers for menu (commands must exist in the command table file)
		/// </summary>
		/// <param name="package">Owner package, not null.</param>
		private LauncherCommands(Package package)
		{
			var commandCallbacks = new Dictionary<LauncherType, EventHandler>
			{
				{ LauncherType.GameLauncher, MenuItemGameLauncherCallback },
				{ LauncherType.Sandbox, MenuItemSandboxCallback },
				{ LauncherType.Server, MenuItemServerCallback }
			};

			_package = package ?? throw new ArgumentNullException(nameof(package));

			if (ServiceProvider.GetService(typeof(IMenuCommandService)) is OleMenuCommandService commandService)
			{
				var ids = new int[]
				{
					(int)Microsoft.VisualStudio.VSConstants.DebugTargetHandler.cmdidDebugTargetAnchorItem,
					(int)Microsoft.VisualStudio.VSConstants.DebugTargetHandler.cmdidDebugTargetAnchorItemNoAttachToProcess,
					(int)Microsoft.VisualStudio.VSConstants.DebugTargetHandler.cmdidDebugTypeCombo,
					(int)Microsoft.VisualStudio.VSConstants.DebugTargetHandler.cmdidDebugTypeItemHandler,
					(int)Microsoft.VisualStudio.VSConstants.DebugTargetHandler.cmdidGenericDebugTarget
				};
				
				var values = Enum.GetValues(typeof(LauncherType)).Cast<LauncherType>();
				foreach(var value in values)
				{
					var menuCommandID = new CommandID(CommandSet, CommandIds[(int)value]);
					var menuItem = new MenuCommand(commandCallbacks[value], menuCommandID);
					commandService.AddCommand(menuItem);
				}
			}
		}

		/// <summary>
		/// Initializes the singleton instance of the command.
		/// </summary>
		/// <param name="package">Owner package, not null.</param>
		public static void Initialize(Package package)
		{
			Instance = new LauncherCommands(package);
		}

		/// <summary>
		/// This function is the callback used to execute the command when the menu item is clicked.
		/// See the constructor to see how the menu item is associated with this function using
		/// OleMenuCommandService service and MenuCommand class.
		/// </summary>
		/// <param name="sender">Event sender.</param>
		/// <param name="e">Event args.</param>
		private void MenuItemGameLauncherCallback(object sender, EventArgs e)
		{
			CryEngineDebuggableConfig.SetCurrentLauncherTarget(LauncherType.GameLauncher);
			//TODO Launch the debugger
		}

		/// <summary>
		/// This function is the callback used to execute the command when the menu item is clicked.
		/// See the constructor to see how the menu item is associated with this function using
		/// OleMenuCommandService service and MenuCommand class.
		/// </summary>
		/// <param name="sender">Event sender.</param>
		/// <param name="e">Event args.</param>
		private void MenuItemSandboxCallback(object sender, EventArgs e)
		{
			CryEngineDebuggableConfig.SetCurrentLauncherTarget(LauncherType.Sandbox);
			//TODO Launch the debugger
		}

		/// <summary>
		/// This function is the callback used to execute the command when the menu item is clicked.
		/// See the constructor to see how the menu item is associated with this function using
		/// OleMenuCommandService service and MenuCommand class.
		/// </summary>
		/// <param name="sender">Event sender.</param>
		/// <param name="e">Event args.</param>
		private void MenuItemServerCallback(object sender, EventArgs e)
		{
			CryEngineDebuggableConfig.SetCurrentLauncherTarget(LauncherType.Server);
			//TODO Launch the debugger
		}
	}
}
