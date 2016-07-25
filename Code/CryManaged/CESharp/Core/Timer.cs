using System;
using CryEngine.Common;

namespace CryEngine
{
    /// <summary>
    /// Creates a Timer that will callback after expiring.
    /// </summary>
    public class Timer : IGameUpdateReceiver
    {
        private Action _onTimerExpired;
        private float _startTime;
        private float _time;
        private float _timeToLive;

        public float Time { get { return _time; } }
        public bool Looping { get; private set; }

        public Timer(float time, bool looping, Action onTimerExpired = null)
        {
            GameFramework.RegisterForUpdate(this);

            this._timeToLive = time;
            this._onTimerExpired = onTimerExpired;
            this.Looping = looping;

            _startTime = Env.Timer.GetCurrTime();
        }

        public Timer(float time, Action onTimerExpired = null) : this(time, false, onTimerExpired)
        {

        }

        public void Destroy()
        {
            GameFramework.UnregisterFromUpdate(this);
            _onTimerExpired = null;
        }

        void IGameUpdateReceiver.OnUpdate()
        {
            if (Env.Timer.GetCurrTime() > _startTime + _timeToLive)
            {
                _onTimerExpired?.Invoke();

                if (Looping)
                {
                    _startTime = Env.Timer.GetCurrTime();
                }
                else
                {
                    Destroy();
                }
            }
            else
            {
                _time += Env.Timer.GetCurrTime();
            }
        }
    }
}

