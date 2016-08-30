using System;
using CryEngine.Common;
using CryEngine.Framework;


namespace CryEngine.Towerdefense.Components
{
    /// <summary>
    /// Controls the logic for rotating the tower's turret.
    /// </summary>
    public class TurretRotator : GameComponent
    {
        public event EventHandler AimingFinished;

        enum State
        {
            Idle,
            Aiming,
            Tracking,
            WaitingForNewTarget,
            ReturningToIdle,
        }

        State state;
        GameObject target;
        Timer timer;
        TurretRotatorTweener rotator;

        public float RotateSpeed { get; set; }

        public GameObject Target
        {
            set
            {
                target = value;
                UpdateState();
            }
        }

        public Quat LookAt
        {
            get
            {
                return target != null && target.Exists ? QuatEx.LookAt(GameObject.Position, target.Position) : Quat.CreateIdentity();
            }
        }

        protected override void OnInitialize()
        {
            rotator = new TurretRotatorTweener();
        }

        protected override void OnDestroy()
        {
            rotator.Destroy();
            timer?.Destroy();
        }

        void UpdateState()
        {
            if (target != null)
            {
                if (state == State.Idle)
                {
                    // Start aiming
                    rotator.Rotate(GameObject, RotateSpeed, LookAt, () =>
                    {
                        AimingFinished?.Invoke();
                        state = State.Tracking;
                        UpdateState();
                    });
                }
                else if (state == State.Tracking || state == State.Aiming)
                {
                    // Switch targets
                    rotator.Rotate(GameObject, RotateSpeed, LookAt, () =>
                    {
                        AimingFinished?.Invoke();
                    });
                }
                else if (state == State.ReturningToIdle)
                {
                    // Interrupt returning to idle state and return to aiming state.
                    timer?.Destroy();
                    state = State.Aiming;
                    UpdateState();
                }
            }
            else
            {
                if ((state == State.Tracking || state == State.Aiming) && state != State.WaitingForNewTarget)
                {
                    state = State.WaitingForNewTarget;

                    // Wait 2 seconds after a target has lost for another target to appear.
                    timer = new Timer(2f, () =>
                    {
                        state = State.ReturningToIdle;
                        UpdateState();
                    });
                }
                else if (state == State.ReturningToIdle)
                {
                    rotator.Rotate(GameObject, RotateSpeed, Quat.Identity, () =>
                    {
                        state = State.Idle;
                    });
                }
            }
        }

        public void ClearTarget()
        {
            Target = null;
        }

        /// <summary>
        /// Performs the actual rotation of the Turret.
        /// </summary>
        class TurretRotatorTweener : IGameUpdateReceiver
        {
            GameObject gameObject;
            Quat startRotation;
            Quat targetRotation;
            float rotateTime;
            Action onComplete;
            bool isRotating;

            public TurretRotatorTweener()
            {
                GameFramework.RegisterForUpdate(this);
            }

            public void Rotate(GameObject gameObject, float rotateTime, Quat targetRotation, Action onComplete)
            {
                this.gameObject = gameObject;
                this.rotateTime = rotateTime;
                this.startRotation = gameObject.Rotation;
                this.targetRotation = targetRotation;
                this.onComplete = onComplete;

                isRotating = true;
            }

            public void OnUpdate()
            {
                if (!isRotating)
                    return;

                gameObject.Rotation = Quat.CreateSlerp(startRotation, targetRotation, FrameTime.Delta / rotateTime);

                if (gameObject.Rotation.Forward.Dot(targetRotation.Forward) > 0.9f)
                {
                    gameObject.Rotation = targetRotation;
                    onComplete();
                    isRotating = false;
                }
            }

            public void Destroy()
            {
                isRotating = false;
                GameFramework.UnregisterFromUpdate(this);
                onComplete = null;
            }
        }
    }
}
