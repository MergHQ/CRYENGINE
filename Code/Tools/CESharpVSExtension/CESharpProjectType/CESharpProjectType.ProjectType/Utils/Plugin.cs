using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using EnvDTE;
using EnvDTE80;

namespace CESharpProjectType
{
	/// <summary>
	/// Dynamically converts PDB to MDB files, after build is finished.
	/// </summary>
	class Plugin
	{
		private const string CSharpProjectTypeGuid = "{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}";

		public DTE2 DTE { get; private set; }

		private bool _initialized = false;

		public bool Init(DTE2 applicationObject)
		{
			DTE = applicationObject;
			if (!IsCESharpSolution)
				return false;

			DTE.Events.BuildEvents.OnBuildBegin += new _dispBuildEvents_OnBuildBeginEventHandler(OnBuildBegin);
			DTE.Events.BuildEvents.OnBuildDone += new _dispBuildEvents_OnBuildDoneEventHandler(OnBuildDone);

			_initialized = true;
			return true;
		}

		public void Shutdown()
		{
			if (_initialized)
			{
				DTE.Events.BuildEvents.OnBuildDone -= new _dispBuildEvents_OnBuildDoneEventHandler(OnBuildDone);
				DTE.Events.BuildEvents.OnBuildBegin -= new _dispBuildEvents_OnBuildBeginEventHandler(OnBuildBegin);
			}
			_initialized = false;
		}
		public void OnBuildBegin(vsBuildScope Scope, vsBuildAction Action)
		{
			if (Action != vsBuildAction.vsBuildActionClean)
				VsOutputPane.LogMessage("Building (" + Scope.ToString() + ").");
		}

		public void OnBuildDone(vsBuildScope Scope, vsBuildAction Action)
		{
			if (Action != vsBuildAction.vsBuildActionClean)
			{
				CompilePDBs();
			}
		}

		private bool IsCESharpProject(Project p)
		{
			return (p.Kind.ToUpper().Contains(CESharpProjectType.VsPackage.ProjectTypeGuid));
		}

		bool IsCESharpSolution
		{
			get
			{
				bool isCESharpSolution = false;
				foreach (Project p in DTE.Solution.Projects)
					if (IsCESharpProject(p))
						isCESharpSolution = true;
				return isCESharpSolution;
			}
		}

		/// <summary>
		/// Converts PDB to MDB for each dll project.
		/// </summary>
		private void CompilePDBs()
		{
			if (!IsCESharpSolution)
			{
				VsOutputPane.LogMessage("Skipping PDB conversion (No CE# Project).");
				return;
			}

			VsOutputPane.LogMessage("Compiling PDB's.");

			var pdb2mdb = Path.GetFullPath(MonoDebuggerLaunchProvider.CERoot + "/Tools/pdb2mdb/pdb2mdb.exe");
			if (!File.Exists(pdb2mdb))
			{
				VsOutputPane.LogError("Cannot find PDB2MDB executable.");
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

				VsOutputPane.LogMessage("Converting '" + outputFileName + "'.");
				var psi = new ProcessStartInfo(pdb2mdb, "\"" + Path.Combine(outputDir, outputFileName) + "\"");
				System.Diagnostics.Process.Start(psi);
			}
			VsOutputPane.LogMessage("Done.");
		}
	}
}
