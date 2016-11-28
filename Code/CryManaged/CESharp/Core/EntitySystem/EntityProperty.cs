// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Attributes;
using CryEngine.Common;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;

namespace CryEngine
{
    public enum EntityPropertyType : UInt32
    {
        /// <summary>
        /// Used by default for default types such as numerics and strings
        /// </summary>
        Primitive = 0,
        Object,
        Texture,
        Particle,
        AnyFile,
        Sound,
        Material,
        Animation
    }

    /// <summary>
	/// Marked properties will be available in the Sandbox
	/// </summary>
	[AttributeUsage(AttributeTargets.Property)]
    public sealed class EntityPropertyAttribute : Attribute
    {
        #region Properties
        /// <summary>
        /// Mouse-Over description for Sandbox.
        /// </summary>
        public string Description { get; set; }
        /// <summary>
        /// Sandbox edit type. Determines the Sandbox control for this property.
        /// </summary>
        public EntityPropertyType Type { get; set; }
        #endregion

        #region Constructors
        public EntityPropertyAttribute(EntityPropertyType type = EntityPropertyType.Primitive, string description = null)
        {
            Description = description;
            Type = type;
        }
        #endregion
    }
}

