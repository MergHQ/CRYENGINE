// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

namespace CryEngine.EntitySystem
{
	/// <summary>
	/// Wrapper for the IEntity class of CRYENGINE. Provides properties and wraps functions for easier access of IEntity functions.
	/// Entity must not inherit from IEntity but keep an ID reference to it, for reasons of CRYENGINE internal entity representation.
	/// </summary>
	public partial class Entity
	{
		#region Fields
		private bool _isGameRunning;
		private EntityPhysics _physics;
		private List<EntityComponent> _components;

		private EntityId _id;
		#endregion

		public event EventHandler<Entity> OnEntityCollision; ///< Raised if an entity collision occurred.

		/// <summary>
		/// Gets the unique identifier associated with this entity
		/// </summary>
		public EntityId Id
		{
			get { return _id; }

			// This should only be called once!
			internal set
			{
				s_managedEntities.Add(value, this);

				_id = value;
			}
		}

		public EntityEventListener EventListener { get; private set; }

		public Vector3 Position { get { return NativeHandle.GetPos(); } set { NativeHandle.SetPos(value); } } ///< Position of the wrapped IEntity.
		public Quaternion Rotation { get { return NativeHandle.GetRotation(); } set { NativeHandle.SetRotation(value); } } ///< Rotation of the wrapped IEntity.
		public float Scale { get { return NativeHandle.GetScale().x; } set { NativeHandle.SetScale(new Vec3(value)); } } ///< Scale of the wrapped IEntity.
		public string Name { get { return NativeHandle.GetName(); } } ///< Name of the wrapped IEntity.
		public IMaterial Material { get { return NativeHandle.GetMaterial(); } set { NativeHandle.SetMaterial(value); } } ///< Material of the wrapped IEntity.
		public bool Exists { get { return NativeHandle != null; } } ///< Determines whether the IEntity object still exists.
		public Matrix3x4 Transform { get { return NativeHandle.GetLocalTM(); } set { NativeHandle.SetLocalTM(value); } } ///< Entities local transform matrix. 

		public ReadOnlyCollection<EntityComponent> Components { get { return _components.AsReadOnly(); } }

		public IEntity NativeHandle { get; internal set; }

		/// <summary>
		/// Reference to the managed entity class.
		/// </summary>
		public EntityClass EntityClass { get; internal set; }

		public EntityPhysics Physics
		{
			get
			{
				if (_physics == null)
					_physics = new EntityPhysics(this);

				return _physics;
			}
		}

		internal void InitializeManagedEntity(Type type)
		{
			_components = new List<EntityComponent>();

			EventListener = new EntityEventListener(this);

			SystemEventHandler.EditorGameStart += OnStart;
			SystemEventHandler.EditorGameEnded += OnEnd;

			// Call the default constructor
			var constructor = type.GetConstructor(Type.EmptyTypes);
			constructor.Invoke(this, null);

			// Continue straight to OnStart if running in Standalone.
			if (!Engine.IsSandbox)
				OnStart();
		}

		public void LoadMaterial(string path)
		{
			var material = Global.gEnv.p3DEngine.GetMaterialManager().LoadMaterial(path);
			if (material != null)
				NativeHandle.SetMaterial(material);
		}

		/// <summary>
		/// Called when starting play mode in Sandbox or when running the standalone game.
		/// </summary>
		/// <param name="arg"></param>
		public virtual void OnStart()
		{
			_isGameRunning = true;
			_components.ForEach(x => x.OnStart());
		}

		/// <summary>
		/// Called when Sandbox playback ends, before the entity is removed, and when the CE Sharp framework is shutting down.
		/// </summary>
		public virtual void OnEnd()
		{
			_isGameRunning = false;

			if (_components != null)
			{
				_components.ForEach(x => x.OnEnd());
			}
		}

		/// <summary>
		/// Called once per frame.
		/// </summary>
		public virtual void OnUpdate()
		{
			if (_components != null)
			{
				_components.ForEach(x => x.OnUpdate());
			}
		}

		/// <summary>
		/// Called when CRYENGINE's entity system issues an event.
		/// </summary>
		public virtual void OnEvent(SEntityEvent arg)
		{
			switch (arg._event)
			{
				case EEntityEvent.ENTITY_EVENT_EDITOR_PROPERTY_CHANGED:
					{
						var propertyHandler = EntityClass.NativeClass.GetPropertyHandler();
						if (propertyHandler != null)
						{
							foreach (var property in EntityClass.Properties)
							{
								var propertyValue = propertyHandler.GetProperty(NativeHandle, property.Key);

								property.Value.Set(this, propertyValue);
							}
						}
					}
					break;
			}

			_components.ForEach(x => x.OnEvent(arg));
		}

		/// <summary>
		/// Called before removing the entity from CRYENGINE's entity system.
		/// </summary>
		public virtual void OnRemove()
		{
			NativeHandle = null;

			OnEnd();

			if (_components != null)
			{
				_components.ForEach(x => x.Removed -= OnComponentRemoved);
				_components.ForEach(x => x.Remove());
				_components.Clear();
			}
		}

		/// <summary>
		/// Used by Collision Manager. Do not call directly.
		/// </summary>
		/// <param name="hitEntity">Hit entity.</param>
		internal void RaiseCollision(Entity hitEntity)
		{
			if (OnEntityCollision != null)
				OnEntityCollision(hitEntity);
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
		/// Fired when the component is removed. Removes the component from the internal list.
		/// </summary>
		/// <param name="component"></param>
		private void OnComponentRemoved(EntityComponent component)
		{
			var find = _components.FirstOrDefault(x => x == component);

			if (find != null)
			{
				component.Removed -= OnComponentRemoved;
				_components.Remove(find);
			}
		}

		/// <summary>
		/// Adds and initializes an EntityComponent.
		/// </summary>
		public T AddComponent<T>() where T : EntityComponent, new()
		{
			var component = Activator.CreateInstance<T>() as EntityComponent;
			component.Removed += OnComponentRemoved;

			component.Initialize(this);

			if (_isGameRunning)
				component.OnStart();

			_components.Add(component);

			return component as T;
		}

		/// <summary>
		/// Returns the first EntityComponent that matches the specified type. If nothing is found, returns null.
		/// </summary>
		public T GetComponent<T>() where T : EntityComponent
		{
			return _components.OfType<T>().FirstOrDefault();
		}

		/// <summary>
		/// Performs an action on the first EntityComponent that matches the specified type. The Action will not be invoked if no component is found.
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="onGet"></param>
		public T GetComponent<T>(Action<T> onGet) where T : EntityComponent
		{
			var component = GetComponent<T>();
			if (component != null)
				onGet(component);
			return component;
		}

		/// <summary>
		/// Returns the first EntityComponent that matches the specified type, if it exists. Otherwise, adds a new instance of the component.
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <returns></returns>
		public T GetOrAddComponent<T>() where T : EntityComponent, new()
		{
			var existingComponent = GetComponent<T>();
			if (existingComponent != null)
			{
				return existingComponent;
			}
			else
			{
				return AddComponent<T>();
			}
		}

		/// <summary>
		/// Returns the all EntityComponents that match the specified type. If none are found, returns an empty list.
		/// </summary>
		/// <returns>The components.</returns>
		/// <typeparam name="T">The 1st type parameter.</typeparam>
		public IEnumerable<T> GetComponents<T>() where T : EntityComponent
		{
			return _components.OfType<T>();
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

		/// <summary>
		/// Subscribes the entity to collision events.
		/// </summary>
		public void SubscribeToCollision()
		{
			CollisionEventListener.Instance.Subscribe(Id);
		}

		/// <summary>
		/// Unsubscribes the entity from collision events.
		/// </summary>
		public void UnsubscribeFromCollision()
		{
			CollisionEventListener.Instance.Unsubscribe(Id);
		}
	}
}
