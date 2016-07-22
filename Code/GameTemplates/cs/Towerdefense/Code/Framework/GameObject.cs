using System;
using System.Linq;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using CryEngine.EntitySystem;
using CryEngine.Common;
using CryEngine.Framework;

namespace CryEngine
{
    /// <summary>
    /// Wraps a BaseEntity, adding support for GameComponents and providing several helper methods and properties for accessing the native Entity.
    /// </summary>
    public class GameObject : BaseEntity
    {
        Vec3 lastPos;
        Quat lastRot;
        Vec3 lastScale;

        public GameObject Entity { get { return this; } }
        public string Name { get { return NativeEntity.GetName(); } }
        public uint ID { get { return NativeEntity.GetId(); } }
        public Vec3 Position { get { return Exists ? NativeEntity.GetPos() : lastPos; } set { NativeEntity?.SetPos(value); } }
        public Quat Rotation { get { return Exists ? NativeEntity.GetRotation() : lastRot; } set { NativeEntity?.SetRotation(value); } }
        public Vec3 Scale { get { return Exists ? NativeEntity.GetScale() : lastScale; } set { NativeEntity?.SetScale(value); } }
        public bool Exists { get; private set; }
        public EntityEventListener EventListener { get; private set; }
        public ReadOnlyCollection<GameComponent> Components { get { return components.AsReadOnly(); } }
        public PhysicsComponent Physics { get; private set; }

        protected Logger Logger { get; private set; }

        List<GameComponent> components;
        List<IGameObjectEventHandler> handlers;

        /// <summary>
        /// Called when initializing (creating) the entity. If overriding, you must call the base method.
        /// </summary>
        public override void OnInitialize()
        {
            Logger = new Logger(Name);
            Logger.Enabled = true;
            Logger.LogInfo("Initialized");
            components = new List<GameComponent>();
            handlers = new List<IGameObjectEventHandler>();
            EventListener = new EntityEventListener(this);
            EventListener.AddCallback(new EntityEventCallback(OnStart, EEntityEvent.ENTITY_EVENT_RESET));
            Physics = AddComponent<PhysicsComponent>();
            Exists = true;
        }

        /// <summary>
        /// Called when the game starts or ends, and when AI/Physics simulation starts and ends in Sandbox.
        /// </summary>
        /// <param name="arg"></param>
        public virtual void OnStart(SEntityEvent arg)
        {
            Logger.LogInfo("OnStart");
            handlers.ForEach(x => x.Start());
        }

        /// <summary>
        /// Called by the Entity Framework. If overriding, you must call the base method.
        /// </summary>
        /// <param name="frameTime">Frame time.</param>
        /// <param name="pauseMode">Pause mode.</param>
        public override void OnUpdate(float frameTime, PauseMode pauseMode)
        {
            if (Exists)
            {
                lastPos = NativeEntity.GetPos();
                lastRot = NativeEntity.GetRotation();
                lastScale = NativeEntity.GetScale();
            }

            handlers.ToList().ForEach(x => x.Update(frameTime, pauseMode));
        }

        /// <summary>
        /// Called by the Entity Framework(?). Fired when this GameObject's native Entity recieves an Entity event. If overriding, you must call the base method.
        /// </summary>
        public override void OnEvent(SEntityEvent arg)
        {
            handlers.ToList().ForEach(x => x.NotifyEvent(arg));
        }

        /// <summary>
        /// Called before removing the entity from CRYENGINE's entity system. Do not call directly.
        /// </summary>
        public override void OnRemove()
        {
            Logger.LogInfo("OnRemove");
            EventListener.Destroy();
            components.ForEach(x => x.Removed -= OnComponentRemoved);
            components.Clear();
            handlers.ToList().ForEach(x => x.Destroy());
            handlers.Clear();
            gameObjects.Remove(this);
        }

        /// <summary>
        /// Destroys this GameObject and all its component before removing its native Entity.
        /// </summary>
        public void Destroy()
        {
            Exists = false;
            Logger.LogInfo("Destroy");
            OnDestroy();
            Env.EntitySystem.RemoveEntity(ID);
        }

        /// <summary>
        /// Adds and initializes a GameComponent.
        /// </summary>
        public T AddComponent<T>() where T : GameComponent
        {
            var component = Activator.CreateInstance<T>();
            component.Removed += OnComponentRemoved;
            components.Add(component);

            // Register handler for gameobject events such as OnUpdate.
            var handler = component as IGameComponent;
            handler.Initialize(this);
            handlers.Add(handler);

            return component;
        }

        /// <summary>
        /// Fired when the component is removed. Removes the component from the internal list and invokes it's Destroy method.
        /// </summary>
        /// <param name="component"></param>
        void OnComponentRemoved(GameComponent component)
        {
            if (components.Contains(component))
                components.Remove(component);

            var handler = component as IGameObjectEventHandler;
            if (handlers.Contains(handler))
                handlers.Remove(handler);

            handler.Destroy();
        }

        /// <summary>
        /// Returns the first GameComponent that matches the specified type. If nothing is found, returns null.
        /// </summary>
        public T GetComponent<T>() where T : GameComponent
        {
            return components.OfType<T>().FirstOrDefault();
        }

        /// <summary>
        /// Performs an action on the first GameComponent that matches the specified type. The Action will not be invoked if no component is found.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="onGet"></param>
        public T GetComponent<T>(Action<T> onGet) where T : GameComponent
        {
            var component = GetComponent<T>();
            if (component != null)
                onGet(component);
            return component;
        }

        /// <summary>
        /// Returns the first GameComponent that matches the specified type, if it exists. Otherwise, adds a new instance of the component.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        public T GetOrAddComponent<T>() where T : GameComponent
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
        /// Gets or adds a GameComponent of type T, then performs an action on it.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="onGet"></param>
        /// <returns></returns>
        public T GetOrAddComponent<T>(Action<T> onGet) where T : GameComponent
        {
            var component = GetOrAddComponent<T>();
            onGet(component);
            return component;
        }

        /// <summary>
        /// Returns the all GameComponents that match the specified type. If none are found, returns an empty list.
        /// </summary>
        /// <returns>The components.</returns>
        /// <typeparam name="T">The 1st type parameter.</typeparam>
        public List<T> GetComponents<T>() where T : GameComponent
        {
            return components.OfType<T>().ToList();
        }

        /// <summary>
        /// Performs an action on all GameComponents that match the specified type, if any exist.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="onGet"></param>
        public void GetComponents<T>(Action<T> onGet) where T : GameComponent
        {
            var components = GetComponents<T>();
            components.ForEach(x => onGet(x));
        }

        /// <summary>
        /// Called by the GameObject framework after invoking Destroy and before the native Entity is removed.
        /// </summary>
        public virtual void OnDestroy() { }

        #region Static
        static List<GameObject> gameObjects = new List<GameObject>();
        static Logger logger = new Logger("GameObject");

        public static int Count { get { return gameObjects.Count; } }

        public static GameObject Instantiate(string name = null)
        {
            return Instantiate<GameObject>(name);
        }

        public static T Instantiate<T>(string name = null) where T : GameObject
        {
            var instance = EntityFramework.Spawn<T>(name);

            if (gameObjects.Contains(instance))
                logger.LogError("Already contains GameObject with ID {0}. This shouldn't happen!", instance.ID);

            gameObjects.Add(instance);
            return instance;
        }
        #endregion

        #region Static Helpers
        /// <summary>
        /// Returns a GameObject that has the associated Entity ID.
        /// </summary>
        public static GameObject ById(uint id)
        {
            return ById<GameObject>(id);
        }

        /// <summary>
        /// Returns a GameObject that has the associated Entity ID and matches the specified type.
        /// </summary>
        public static T ById<T>(uint id) where T : GameObject
        {
            return EntityFramework.GetEntity(id) as T;
        }
        #endregion

        #region Helpers
        public bool Visible
        {
            get
            {
                return !NativeEntity.IsInvisible();
            }
            set
            {
                NativeEntity.Invisible(!value);
            }
        }

        public void Show()
        {
            Visible = true;
        }

        public void Hide()
        {
            Visible = false;
        }
        #endregion
    }
}