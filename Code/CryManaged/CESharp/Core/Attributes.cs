// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;

/// <summary>
/// All framework relevant Attributes.
/// </summary>
namespace CryEngine.Attributes
{
	/// <summary>
	/// Use this attribute to hide any Property of Class from any reflection driven inspection.
	/// </summary>
	[AttributeUsage(System.AttributeTargets.All)]
	public class HideFromInspectorAttribute : Attribute
	{
	}
}
