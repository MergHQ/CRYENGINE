// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;

using CryEngine.EntitySystem;

namespace CryEngine
{
	/// <summary>
	/// Wrapper for the IEntity class of CRYENGINE. Provides properties and wraps functions for easier access of IEntity functions.
	/// Entity must not inherit from IEntity but keep an ID reference to it, for reasons of CRYENGINE internal entity representation.
	/// </summary>
	public sealed partial class Entity
	{
		#region Fields
		private PhysicsEntity _physics;
        #endregion

        #region Properties
        /// <summary>
        /// Gets the unique identifier associated with this entity
        /// </summary>
        public EntityId Id { get; internal set; }
        
		public Vector3 Position { get { return NativeHandle.GetPos(); } set { NativeHandle.SetPos(value); } } ///< Position of the wrapped IEntity.
		public Quaternion Rotation { get { return NativeHandle.GetRotation(); } set { NativeHandle.SetRotation(value); } } ///< Rotation of the wrapped IEntity.
		public float Scale { get { return NativeHandle.GetScale().x; } set { NativeHandle.SetScale(new Vec3(value)); } } ///< Scale of the wrapped IEntity.
		public string Name { get { return NativeHandle.GetName(); } } ///< Name of the wrapped IEntity.
		public IMaterial Material { get { return NativeHandle.GetMaterial(); } set { NativeHandle.SetMaterial(value); } } ///< Material of the wrapped IEntity.
		public bool Exists { get { return NativeHandle != null; } } ///< Determines whether the IEntity object still exists.
		public Matrix3x4 Transform { get { return NativeHandle.GetLocalTM(); } set { NativeHandle.SetLocalTM(value); } } ///< Entities local transform matrix. 
            
		public IEntity NativeHandle { get; internal set; }

        private IntPtr NativeEntityPointer { get; set; }
        
		public PhysicsEntity Physics
		{
			get
			{
				if (_physics == null)
					_physics = new PhysicsEntity(this);

				return _physics;
			}
		}
        #endregion

        #region Constructors
        internal Entity(IEntity handle, EntityId id)
        {
            NativeHandle = handle;
            Id = id;
            
            // Access the raw pointer
            NativeEntityPointer = IEntity.getCPtr(handle).Handle;
        }
        #endregion
        
		public void LoadMaterial(string path)
		{
			var material = Global.gEnv.p3DEngine.GetMaterialManager().LoadMaterial(path);
			if (material != null)
				NativeHandle.SetMaterial(material);
		}
        
		/// <summary>
		/// Called before removing the entity from CRYENGINE's entity system.
		/// </summary>
		public void OnRemove()
		{
			NativeHandle = null;
		}
        
		/// <summary>
		/// Removes the IEntity object from the scene.
		/// </summary>
		public void Remove()
		{
			Global.gEnv.pEntitySystem.RemoveEntity(Id);
		}

		#region Components
		/// <summary>
		/// Adds a new instance of the component.
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <returns></returns>
		public T AddComponent<T>() where T : EntityComponent, new()
		{
			return NativeInternals.Entity.AddComponent(NativeEntityPointer, typeof(T)) as T;
		}

		/// <summary>
		/// Returns the first EntityComponent that matches the specified type. If nothing is found, returns null.
		/// </summary>
		public T GetComponent<T>() where T : EntityComponent
		{
			return NativeInternals.Entity.GetComponent(NativeEntityPointer, typeof(T)) as T;
		}
		
		/// <summary>
		/// Returns the first EntityComponent that matches the specified type, if it exists. Otherwise, adds a new instance of the component.
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <returns></returns>
		public T GetOrCreateComponent<T>() where T : EntityComponent, new()
		{
			return NativeInternals.Entity.GetOrCreateComponent(NativeEntityPointer, typeof(T)) as T;
		}

		public bool HasComponent<T>() where T : EntityComponent
		{
			return GetComponent<T>() != null;
		}
		#endregion

		/// <summary>
		/// Attaches a particle emitter object to a specific entity slot.
		/// </summary>
		/// <param name="slot">Slot.</param>
		/// <param name="emitter">Emitter.</param>
		/// <param name="scale">Scale.</param>
		public void LoadParticleEmitter(int slot, IParticleEffect emitter, float scale = 1.0f)
		{
			var sp = new SpawnParams { fSizeScale = scale };
			NativeHandle.LoadParticleEmitter(slot, emitter, sp, false, false);
		}

		/// <summary>
		/// Returns the world coordinates of a specific helper in a specific entity slot.
		/// </summary>
		/// <returns>The helper position.</returns>
		/// <param name="slot">Slot.</param>
		/// <param name="helperName">Helper name.</param>
		public Vector3 GetHelperPos(int slot, string helperName)
		{
			var statObj = NativeHandle.GetStatObj(slot);

			if (statObj != null)
			{
				return NativeHandle.GetStatObj(slot).GetHelperPos(helperName);
			}

			return Vector3.Zero;
		}

		/// <summary>
		/// Sets position, rotation and scale of an entity slot, ba a matrix.
		/// </summary>
		/// <param name="slot">Slot.</param>
		/// <param name="mx">Mx.</param>
		public void SetGeometrySlotLocalTransform(int slot, Matrix3x4 mx)
		{
			NativeHandle.SetSlotLocalTM(slot, mx);
		}

		/// <summary>
		/// Removes any geometry that was previously attached to an entity slot.
		/// </summary>
		/// <param name="slot">Slot.</param>
		public void FreeGeometrySlot(int slot)
		{
			NativeHandle.FreeSlot(slot);
		}

		/// <summary>
		/// Loads a specific geometry into an entity slot.
		/// </summary>
		/// <param name="slot">Slot.</param>
		/// <param name="url">URL.</param>
		public void LoadGeometry(int slot, string url)
		{
			NativeHandle.LoadGeometry(slot, url);
		}

		public void SetSlotFlag(int slot, EEntitySlotFlags flags)
		{
			NativeHandle.SetSlotFlags(slot, (uint)flags);
		}

		/// <summary>
		/// Returns a particle emitter on the basis of a given slot.
		/// </summary>
		/// <returns>The particle emitter.</returns>
		/// <param name="slot">Slot.</param>
		public IParticleEmitter GetParticleEmitter(int slot)
		{
			return NativeHandle.GetParticleEmitter(slot);
		}

		/// <summary>
		/// Loads a light into a given slot.
		/// </summary>
		/// <param name="slot">Slot.</param>
		/// <param name="light">Light.</param>
		public void LoadLight(int slot, CDLight light)
		{
			NativeHandle.LoadLight(slot, light);
		}
	}
}
