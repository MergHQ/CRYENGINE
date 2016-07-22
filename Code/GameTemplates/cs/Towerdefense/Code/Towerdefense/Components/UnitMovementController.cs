using System;
using System.Linq;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Framework;

using CryEngine.Towerdefense.Components;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Moves a GameObject to a specified position.
    /// </summary>
    public class UnitMovementController : GameComponent
    {
        public event EventHandler OnMoveFinished;

        float moveSpeed;
        Vec3 direction;
        float distance;
        Vec3 startPosition;
        Vec3 nextPosition;
        bool isMoving;
        List<MovementModifier> movementModifiers;

        public float MoveSpeed { set { moveSpeed = value; } }
        public bool Slowed { get { return movementModifiers.Count != 0; } }

        protected override void OnInitialize()
        {
            movementModifiers = new List<MovementModifier>();
            moveSpeed = 1f;
            nextPosition = Vec3.Zero;
        }

        public void MoveToPosition(Vec3 targetPosition)
        {
            direction = targetPosition - GameObject.Position;
            direction.Normalize();

            startPosition = new Vec3(GameObject.Position);
            nextPosition = targetPosition;

            distance = GameObject.Position.GetDistance(nextPosition);

            isMoving = true;
        }

        public override void OnUpdate()
        {
            if (isMoving)
            {
                float finalMoveSpeed = moveSpeed;

                if (movementModifiers.Any(x => x.OverridePreviousModifiers == true))
                {
                    // Take the last overriding modifier and set the speed to it's value.
                    finalMoveSpeed *= movementModifiers.Where(x => x.OverridePreviousModifiers == true).Last().SpeedModifier;
                }
                else
                {
                    // Stack all current speed modifiers
                    foreach (var modifier in movementModifiers)
                        finalMoveSpeed *= modifier.SpeedModifier;
                }

                GameObject.Position += direction * finalMoveSpeed * FrameTime.Delta;

                if (startPosition.GetDistance(GameObject.Position) >= distance)
                {
                    isMoving = false;
                    OnMoveFinished?.Invoke();
                }
            }
        }

        public void AddModifier(MovementModifier mod)
        {
            if (!movementModifiers.Contains(mod))
                movementModifiers.Add(mod);
        }

        public void RemoveModifier(MovementModifier mod)
        {
            if (movementModifiers.Contains(mod))
            {
                Log.Info("Remove Mod");
                movementModifiers.Remove(mod);
            }
        }

        protected override void OnDestroy()
        {
            movementModifiers.Clear();
        }

        public class MovementModifier
        {
            /// <summary>
            /// If true, this modifier will override any previous modifiers that are currently being applied.
            /// </summary>
            public bool OverridePreviousModifiers { get; private set; }

            /// <summary>
            /// A multiplier controlling the speed. 0.5 would be 50% slower, whilst 1.5 would be 50% faster.
            /// </summary>
            /// <value>The speed modifier.</value>
            public float SpeedModifier { get; private set; }

            public MovementModifier(float speedModifier, bool overridePreviousModifiers = false)
            {
                SpeedModifier = speedModifier;
                OverridePreviousModifiers = overridePreviousModifiers;
            }
        }
    }
}

