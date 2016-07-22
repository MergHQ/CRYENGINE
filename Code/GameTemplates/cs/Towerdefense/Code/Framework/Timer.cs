using System;
using CryEngine.Common;
using CryEngine.Framework;

namespace CryEngine.Framework
{
    /// <summary>
    /// Creates a Timer that will callback after expiring.
    /// </summary>
    public class Timer : IGameUpdateReceiver
    {
        Action onTimerExpired;
        float startTime;
        float time;
        float timeToLive;

        public float Time { get { return time; } }

        public Timer(float time, Action onTimerExpired = null)
        {
            GameFramework.RegisterForUpdate(this);

            this.timeToLive = time;
            startTime = Global.gEnv.pTimer.GetCurrTime();
            this.onTimerExpired = onTimerExpired;
        }

        public void Destroy()
        {
            GameFramework.UnregisterFromUpdate(this);
            onTimerExpired = null;
        }

        void IGameUpdateReceiver.OnUpdate()
        {
            if (Global.gEnv.pTimer.GetCurrTime() > startTime + timeToLive)
            {
                onTimerExpired?.Invoke();
                Destroy();
            }
            else
            {
                time += FrameTime.Current;
            }
        }
    }
}

