using EnvDTE;
using EnvDTE80;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using System.Linq;
using Microsoft.VisualStudio.OLE.Interop;
using System.IO;
using System.Diagnostics;

namespace CryEngineVSIX
{
	[PackageRegistration(UseManagedResourcesOnly = true)]
	[InstalledProductRegistration("#110", "#112", "1.0", IconResourceID = 400)]
	[Guid(PackageGuidString)]
	[SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly", Justification = "pkgdef, VS and vsixmanifest are valid VS terms")]
	[ProvideAutoLoad(UIContextGuids80.NoSolution)]
	[ProvideAutoLoad(UIContextGuids80.SolutionExists)]
	[ProvideAutoLoad(UIContextGuids80.SolutionHasMultipleProjects)]
	[ProvideAutoLoad(UIContextGuids80.SolutionHasSingleProject)]
	public sealed class MainPackage : Package
	{
		public const string PackageGuidString = "bfde742e-00c8-4b3a-ad04-63d43b2ccd20";

		// Event containers must be held in order not to be disabled.
		private SolutionEvents _solutionEvents;
		private BuildEvents _buildEvents;
		private DebuggerEvents _debuggerEvents;

		public static DTE2 DTE { get; private set; }
		public static string CryEngineRoot { get; private set; }
		public static bool IsCESharpSolution { get { return CryEngineRoot != null; } }

		protected override void Initialize()
		{
			base.Initialize();
			DTE = (DTE2)GetService(typeof(DTE));
			AttachToEvents();
		}

		private void AttachToEvents()
		{
			_solutionEvents = DTE.Events.SolutionEvents;
			_solutionEvents.Opened += OnSolutionOpened;
			_solutionEvents.AfterClosing += OnSolutionAfterClosing;

			_buildEvents = DTE.Events.BuildEvents;
			_buildEvents.OnBuildDone += OnBuildDone;
			_buildEvents.OnBuildBegin += OnBuildBegin;

			_debuggerEvents = DTE.Events.DebuggerEvents;
			_debuggerEvents.OnEnterRunMode += _debuggerEvents_OnEnterRunMode;
		}

		private void _debuggerEvents_OnEnterRunMode(dbgEventReason Reason)
		{
			if(IsCESharpSolution)
				OutputPane.Log("Running...");
		}

		private IVsBuildPropertyStorage GetPropertyStorage(Project project)
		{
			var serviceProvider = new ServiceProvider(project.DTE as IServiceProvider);
			var sln = (IVsSolution)serviceProvider.GetService(typeof(SVsSolution));
			IVsHierarchy hiearachy = null;
			sln.GetProjectOfUniqueName(project.FullName, out hiearachy);
			return (IVsBuildPropertyStorage)hiearachy;
		}

		public string GetUserProperty(Project project, string propName)
		{
			string propValue;
			GetPropertyStorage(project).GetPropertyValue(propName, "", (uint)_PersistStorageType.PST_USER_FILE, out propValue);
			return propValue;
		}

		public void SetUserProperty(Project project, string propName, string value)
		{
			GetPropertyStorage(project).SetPropertyValue(propName, "", (uint)_PersistStorageType.PST_USER_FILE, value);
		}

		private void OnSolutionOpened()
		{
			CryEngineRoot = null;
			foreach (Project project in DTE.Solution.Projects)
			{
				var ceRoot = GetUserProperty(project, "CryEngineRoot");
				if (ceRoot != null)
					CryEngineRoot = ceRoot;
			}

			if (IsCESharpSolution)
			{
				OutputPane.Init(DTE);
				OutputPane.Clear();
				OutputPane.Log($"CE# Solution detected. CryEngine Root: \"{CryEngineRoot}\"");

				// Setup default values for CE# projects if not yet initialized.
				bool reOpenProject = false;
				foreach (Project p in DTE.Solution.Projects)
				{
					var ceRoot = GetUserProperty(p, "CryEngineRoot");
					if (ceRoot != null)
					{
						if (GetUserProperty(p, "StartAction") == null)
						{
							SetUserProperty(p, "StartAction", "Program");
							reOpenProject = true;
						}
						if (GetUserProperty(p, "StartProgram") == null)
						{
							SetUserProperty(p, "StartProgram", GetUserProperty(p, "LocalDebuggerCommand"));
							reOpenProject = true;
						}
						if (GetUserProperty(p, "StartArguments") == null)
						{
							SetUserProperty(p, "StartArguments", GetUserProperty(p, "LocalDebuggerCommandArguments"));
							reOpenProject = true;
						}
					}

					if (reOpenProject)
					{
						string solutionName = Path.GetFileNameWithoutExtension(DTE.Solution.FullName);
						DTE.Windows.Item(EnvDTE.Constants.vsWindowKindSolutionExplorer).Activate();
						DTE.ToolWindows.SolutionExplorer.GetItem(solutionName + @"\" + p.Name).Select(vsUISelectionType.vsUISelectionTypeSelect);
						DTE.ExecuteCommand("Project.UnloadProject");
						DTE.ExecuteCommand("Project.ReloadProject");
					}
				}
			}
		}

		private void OnBuildBegin(vsBuildScope Scope, vsBuildAction Action)
		{
			if (!IsCESharpSolution)
				return;
			if (Action != vsBuildAction.vsBuildActionClean)
				OutputPane.Log($"Building ({Scope}).");
		}

		private void OnBuildDone(vsBuildScope Scope, vsBuildAction Action)
		{
			if (!IsCESharpSolution)
				return;
			if (Action != vsBuildAction.vsBuildActionClean)
				CompilePDBs();
		}

		private void OnSolutionAfterClosing()
		{
			OutputPane.Shutdown();
		}

		private void CompilePDBs()
		{
			OutputPane.Log("Compiling PDB's.");

			var pdb2mdb = Path.GetFullPath($"{CryEngineRoot}\\Tools\\pdb2mdb\\pdb2mdb.exe");
			if (!File.Exists(pdb2mdb))
			{
				OutputPane.LogError($"Cannot find PDB2MDB executable ({pdb2mdb}).");
				return;
			}

			foreach (Project p in DTE.Solution.Projects)
			{
				string outputFileName = p.Properties.Item("AssemblyName").Value.ToString() + ".dll";
				var fullPath = new FileInfo(p.Properties.Item("FullPath").Value.ToString()).Directory.ToString();
				var outputPath = p.ConfigurationManager.ActiveConfiguration.Properties.Item("OutputPath").Value.ToString();
				string outputDir = Path.Combine(fullPath, outputPath);

				// Skip if MDB is newer than PDB.
				var pdbFullName = Path.Combine(outputDir, outputFileName.Substring(0, outputFileName.Length - 4) + ".pdb");
				var mdbFullName = Path.Combine(outputDir, outputFileName + ".mdb");
				if (File.Exists(pdbFullName) && File.Exists(mdbFullName))
				{
					if (new FileInfo(pdbFullName).LastWriteTime < new FileInfo(mdbFullName).LastWriteTime)
						continue;
				}

				OutputPane.Log($"Converting \"{outputFileName}\".");
				var psi = new ProcessStartInfo(pdb2mdb, $"\"{Path.Combine(outputDir, outputFileName)}\"");
				System.Diagnostics.Process.Start(psi);
			}
			OutputPane.Log("Done.");
		}
	}
}
