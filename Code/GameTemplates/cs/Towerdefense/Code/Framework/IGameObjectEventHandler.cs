using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine.Framework
{
    /// <summary>
    /// Method events invoked by a GameObject.
    /// </summary>
    public interface IGameObjectEventHandler
    {
        void Start();
        void NotifyEvent(SEntityEvent arg);
        void Update(float frameTime, PauseMode pauseMode);
        void Destroy();
    }
}
