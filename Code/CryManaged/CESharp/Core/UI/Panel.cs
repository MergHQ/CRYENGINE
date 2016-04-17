// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using CryEngine.UI.Components;
using System.IO;
using System.Runtime.Serialization;

namespace CryEngine.UI
{
	/// <summary>
	/// Represents a simple Panel element.
	/// </summary>
	[DataContract]
	public class Panel : UIElement
	{
		public Image Background { get; private set; } ///< Optional Background image for the Panel.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public virtual void OnAwake()
		{
			Background = AddComponent<Image>();
		}
	}
}
