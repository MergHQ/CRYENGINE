using CryEngine.Common;

namespace CryEngine.EntitySystem
{
    public interface IEntityComponentHandler
    {
        void Initialize(BaseEntity owner);
        void Start();
        void End();
        void NotifyEvent(SEntityEvent arg);
        void Update(float frameTime, PauseMode pauseMode);
        void Remove();
    }

    public class EntityComponent : IEntityComponentHandler
    {
        public event EventHandler<EntityComponent> Removed;

        public BaseEntity Entity { get; private set; }

        protected Logger Logger { get; private set; }

        void IEntityComponentHandler.Initialize(BaseEntity owner)
        {
            Logger = new Logger(GetType().Name);
            Logger.Enabled = false;
            Logger.LogInfo("Initialize");
            Entity = owner;
            OnInitialize();
        }

        void IEntityComponentHandler.Start()
        {
            Logger.LogInfo("Start");
            OnStart();
        }

        void IEntityComponentHandler.End()
        {
            Logger.LogInfo("OnEditorGameEnded");
            OnEnd();
        }

        void IEntityComponentHandler.NotifyEvent(SEntityEvent arg)
        {
            Logger.LogInfo("Event: {0}", arg._event);
            OnEvent(arg);
        }

        void IEntityComponentHandler.Update(float frameTime, PauseMode pauseMode)
        {
            OnUpdate(frameTime, pauseMode);
        }

        /// <summary>
        /// Removes the component from it's owning GameObject.
        /// </summary>
        public void Remove()
        {
            Logger.LogInfo("Remove");
            OnRemove();
            Removed?.Invoke(this);
        }

        protected virtual void OnInitialize() { }
        protected virtual void OnStart() { }
        protected virtual void OnEnd() { }
        protected virtual void OnReset() { }
        protected virtual void OnEvent(SEntityEvent arg) { }
        protected virtual void OnUpdate(float frameTime, PauseMode pauseMode) { }
        protected virtual void OnRemove() { }
    }
}
