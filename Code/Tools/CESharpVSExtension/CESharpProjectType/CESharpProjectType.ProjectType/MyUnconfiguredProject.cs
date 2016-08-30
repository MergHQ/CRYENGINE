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
	using System.ComponentModel.Composition;
	using System.Diagnostics.CodeAnalysis;
	using System.Threading.Tasks;
	using System.Threading.Tasks.Dataflow;
	using Microsoft.VisualStudio.ProjectSystem;
	using Microsoft.VisualStudio.ProjectSystem.Designers;
	using Microsoft.VisualStudio.ProjectSystem.Utilities;
	using Microsoft.VisualStudio.Shell;
	using Microsoft.VisualStudio.Shell.Interop;
	using Task = System.Threading.Tasks.Task;

	[Export]
	[AppliesTo(MyUnconfiguredProject.UniqueCapability)]
	[ProjectTypeRegistration(VsPackage.ProjectTypeGuid, "CE# Project", "#2", ProjectExtension, Language, resourcePackageGuid: VsPackage.PackageGuid, PossibleProjectExtensions = ProjectExtension, ProjectTemplatesDir = @"..\..\Templates\Projects\MyCustomProject")]
	[ProvideProjectItem(VsPackage.ProjectTypeGuid, "My Items", @"..\..\Templates\ProjectItems\MyCustomProject", 500)]
	internal class MyUnconfiguredProject
	{
		/// <summary>
		/// The file extension used by your project type.
		/// This does not include the leading period.
		/// </summary>
		internal const string ProjectExtension = "ceproj";

		/// <summary>
		/// A project capability that is present in your project type and none others.
		/// This is a convenient constant that may be used by your extensions so they
		/// only apply to instances of your project type.
		/// </summary>
		/// <remarks>
		/// This value should be kept in sync with the capability as actually defined in your .targets.
		/// </remarks>
		internal const string UniqueCapability = "CESharpProjectType";

		internal const string Language = "CESharpProjectType";

		[ImportingConstructor]
		public MyUnconfiguredProject(UnconfiguredProject unconfiguredProject)
		{
			this.ProjectHierarchies = new OrderPrecedenceImportCollection<IVsHierarchy>(projectCapabilityCheckProvider: unconfiguredProject);
		}

		[Import]
		internal UnconfiguredProject UnconfiguredProject { get; }

		[Import]
		internal IActiveConfiguredProjectSubscriptionService SubscriptionService { get; }

		[Import]
		internal IThreadHandling ThreadHandling { get; }

		[Import]
		internal ActiveConfiguredProject<ConfiguredProject> ActiveConfiguredProject { get; }

		[Import]
		internal ActiveConfiguredProject<MyConfiguredProject> MyActiveConfiguredProject { get; }

		[ImportMany(ExportContractNames.VsTypes.IVsProject, typeof(IVsProject))]
		internal OrderPrecedenceImportCollection<IVsHierarchy> ProjectHierarchies { get; }

		internal IVsHierarchy ProjectHierarchy
		{
			get { return this.ProjectHierarchies.Single().Value; }
		}
	}
}
