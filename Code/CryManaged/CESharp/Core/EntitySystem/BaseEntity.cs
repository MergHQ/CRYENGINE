// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

namespace CryEngine.EntitySystem
{
    /// <summary>
    /// Base class for CESharp entities. Classes inheriting from this class will automatically be registered with CRYENGINE.
    /// </summary>
    public abstract class BaseEntity
    {
        #region Fields
        /// <summary>
        /// Reference to the native engine entity.
        /// </summary>
        protected IEntity _entity;
        /// <summary>
        /// Reference to the managed entity class.
        /// </summary>
        protected EntityClass _entityClass;

        private bool _isGameRunning;
        private Vec3 _lastPos;
        private Quat _lastRot;
        private Vec3 _lastScale;
        private EntityPhysics _physics;
        private List<KeyValuePair<EntityComponent, IEntityComponentHandler>>_components;
        #endregion

        #region Properties
        /// <summary>
        /// EntityId of the native CRYENGINE entity.
        /// </summary>
        public uint Id { get { return _entity != null ? _entity.GetId() : 0; } }

        /// <summary>
        /// Reference to the native CRYENGINE entity.
        /// </summary>
        public IEntity NativeEntity { get { return _entity; } }

        /// <summary>
        /// Reference to the managed entity class.
        /// </summary>
        public EntityClass EntityClass { get { return _entityClass; } }

        /// <summary>
        /// The name of the native CRYENGINE entity.
        /// </summary>
        public string Name { get { return NativeEntity.GetName(); } }

        public Vec3 Position { get { return Exists ? NativeEntity.GetPos() : _lastPos; } set { NativeEntity?.SetPos(value); } }
        public Quat Rotation { get { return Exists ? NativeEntity.GetRotation() : _lastRot; } set { NativeEntity?.SetRotation(value); } }
        public Vec3 Scale { get { return Exists ? NativeEntity.GetScale() : _lastScale; } set { NativeEntity?.SetScale(value); } }
        public bool Exists { get { return _entity != null; } }
        public ReadOnlyCollection<EntityComponent> Components { get { return _components.Select(x => x.Key).ToList().AsReadOnly(); } }
        public EntityPhysics Physics { get { return _physics; } }
        public EntityEventListener EventListener { get; private set; }
        #endregion

        #region Methods
        /// <summary>
        /// Called by the EntityFramework. Do not call directly.
        /// </summary>
        public void Initialize()
        {
            _physics = new EntityPhysics(this);
            _components = new List<KeyValuePair<EntityComponent, IEntityComponentHandler>>();

            EventListener = new EntityEventListener(this);

            SystemHandler.EditorGameStart += OnStart;
            SystemHandler.EditorGameEnded += OnEnd;

            OnInitialize();

            // Continue straight to OnStart if running in Standalone.
            if (!Env.IsSandbox)
                OnStart();
        }

        /// <summary>
        /// Called when initializing (creating) the entity.
        /// </summary>
        public virtual void OnInitialize()
        {

        }

        /// <summary>
        /// Called when starting play mode in Sandbox or when running the standalone game.
        /// </summary>
        /// <param name="arg"></param>
        public virtual void OnStart()
        {
            _isGameRunning = true;
            _components.ForEach(x => x.Value.Start());
        }

        /// <summary>
        /// Called when Sandbox playback ends, before the entity is removed, and when the CE Sharp framework is shutting down.
        /// </summary>
        public virtual void OnEnd()
        {
            _isGameRunning = false;
            _components.ForEach(x => x.Value.End());
        }

        /// <summary>
        /// Called once per frame.
        /// </summary>
        public virtual void OnUpdate(float frameTime, PauseMode pauseMode)
        {
            if (Exists)
            {
                _lastPos = NativeEntity.GetPos();
                _lastRot = NativeEntity.GetRotation();
                _lastScale = NativeEntity.GetScale();
            }

            _components.ForEach(x => x.Value.Update(frameTime, pauseMode));
        }

        /// <summary>
        /// Called when CRYENGINE's entity system issues an event.
        /// </summary>
        public virtual void OnEvent(SEntityEvent arg)
        {
            _components.ForEach(x => x.Value.NotifyEvent(arg));
        }

        /// <summary>
        /// Called before removing the entity from CRYENGINE's entity system.
        /// </summary>
        public virtual void OnRemove()
        {
            OnEnd();

            _components.ForEach(x => x.Key.Removed -= OnComponentRemoved);
            _components.ForEach(x => x.Key.Remove());
            _components.Clear();
        }

        /// <summary>
        /// Called when the entity's properties have been changed inside Sandbox.
        /// </summary>
        public virtual void OnPropertiesChanged() { }

        /// <summary>
        /// Called when the CESharp framework is shutting down (e.g. when reloading the code).
        /// </summary>
        public virtual void OnShutdown()
        {
            OnEnd();
        }

        /// <summary>
        /// Removes the entity from the entity system.
        /// </summary>
        public void Remove()
        {
            Env.EntitySystem.RemoveEntity(Id);
        }

        /// <summary>
        /// Fired when the component is removed. Removes the component from the internal list.
        /// </summary>
        /// <param name="component"></param>
        private void OnComponentRemoved(EntityComponent component)
        {
            var find = _components.FirstOrDefault(x => x.Key == component);

            if (find.Key != null)
            {
                component.Removed -= OnComponentRemoved;
                _components.Remove(find);
            }
        }

        #region Components
        /// <summary>
        /// Adds and initializes an EntityComponent.
        /// </summary>
        public T AddComponent<T>() where T : EntityComponent
        {
            var component = Activator.CreateInstance<T>();
            component.Removed += OnComponentRemoved;

            var handler = component as IEntityComponentHandler;
            handler.Initialize(this);

            if (_isGameRunning)
                handler.Start();

            _components.Add(new KeyValuePair<EntityComponent, IEntityComponentHandler>(component, handler));

            return component;
        }

        /// <summary>
        /// Returns the first EntityComponent that matches the specified type. If nothing is found, returns null.
        /// </summary>
        public T GetComponent<T>() where T : EntityComponent
        {
            return _components.Select(x => x.Key).OfType<T>().FirstOrDefault();
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
        public T GetOrAddComponent<T>() where T : EntityComponent
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
        /// Gets or adds a EntityComponent of type T, then performs an action on it.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="onGet"></param>
        /// <returns></returns>
        public T GetOrAddComponent<T>(Action<T> onGet) where T : EntityComponent
        {
            var component = GetOrAddComponent<T>();
            onGet(component);
            return component;
        }

        /// <summary>
        /// Returns the all EntityComponents that match the specified type. If none are found, returns an empty list.
        /// </summary>
        /// <returns>The components.</returns>
        /// <typeparam name="T">The 1st type parameter.</typeparam>
        public List<T> GetComponents<T>() where T : EntityComponent
        {
            return _components.OfType<T>().ToList();
        }

        /// <summary>
        /// Performs an action on all EntityComponents that match the specified type, if any exist.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="onGet"></param>
        public void GetComponents<T>(Action<T> onGet) where T : EntityComponent
        {
            var components = GetComponents<T>();
            components.ForEach(x => onGet(x));
        }
        #endregion

        #endregion
    }
}

