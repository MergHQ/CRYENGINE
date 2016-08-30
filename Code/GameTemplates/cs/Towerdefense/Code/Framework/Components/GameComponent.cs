using CryEngine.Common;
using CryEngine.EntitySystem;
using CryEngine.Framework;
using System;

namespace CryEngine
{
    public interface IGameComponent : IGameObjectEventHandler
    {
        void Initialize(GameObject owner);
    }

    public class GameComponent : IGameComponent
    {
        public event EventHandler<GameComponent> Removed;

        public GameObject GameObject { get; private set; }

        protected EntityEventListener EventListener { get { return GameObject.EventListener; } }
        protected Logger Logger { get; private set; }

        void IGameComponent.Initialize(GameObject owner)
        {
            Logger = new Logger(GetType().Name);
            Logger.Enabled = false;
            Logger.LogInfo("Initialize");
            GameObject = owner;
            OnInitialize();
        }

        void IGameObjectEventHandler.Start()
        {
            Logger.LogInfo("Start");
            OnStart();
        }

        void IGameObjectEventHandler.NotifyEvent(SEntityEvent arg)
        {
            Logger.LogInfo("Event: {0}", arg._event);
            OnEvent(arg);
        }

        void IGameObjectEventHandler.Update(float frameTime, PauseMode pauseMode)
        {
            //Logger.LogInfo("Update: FrameTime({0}), PauseMode({1})", frameTime, pauseMode);
            OnUpdate(frameTime, pauseMode);
            OnUpdate();
        }

        void IGameObjectEventHandler.Destroy()
        {
            Logger.LogInfo("Destroy");
            EventListener.Destroy();
            OnDestroy();
        }

        /// <summary>
        /// Removes the component from it's owning GameObject.
        /// </summary>
        public void Remove()
        {
            Logger.LogInfo("Remove");
            Removed?.Invoke(this);
        }

        protected virtual void OnInitialize() { }
        protected virtual void OnStart() { }
        protected virtual void OnReset() { }
        protected virtual void OnEvent(SEntityEvent arg) { }
        protected virtual void OnUpdate(float frameTime, PauseMode pauseMode) { }
        public virtual void OnUpdate() { } // TODO: Remove

        /// <summary>
        /// Invoked when the component is removed or the owning GameObject is destroyed.
        /// </summary>
        protected virtual void OnDestroy() { }
    }
}
