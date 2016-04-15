// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using CryEngine.Resources;

namespace CryEngine.Resources
{
	/// <summary>
	/// Holds a reference to all ImageSource objects which have been instanciated though it to avoid multiple instancing.
	/// </summary>
	public class ResourceManager : Singleton<ResourceManager>
	{
		Dictionary<string, ImageSource> _imageSources = new Dictionary<string, ImageSource>();

		/// <summary>
		/// Creates an ImageSource if not yet existing, hands out the existing one otherwise.
		/// </summary>
		/// <returns>The created or registered ImageSource.</returns>
		/// <param name="url">Image source file.</param>
		/// <param name="filtered">If set to <c>true</c>, texture's high quality filtering is enabled.</param>
		public static ImageSource ImageFromFile(string url, bool filtered = true)
		{
			if (Instance._imageSources.ContainsKey (url))
				return Instance._imageSources[url];				
			return Instance._imageSources [url] = new ImageSource (url, filtered);
		}

		/// <summary>
		/// Removes all cached resources from manager.
		/// </summary>
		public static void Clear()
		{
			Instance._imageSources.Clear ();
		}
	}
}
