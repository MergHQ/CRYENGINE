using System;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Framework;
using CryEngine.Towerdefense.Components;
using CryEngine.UI;

namespace CryEngine.Towerdefense
{
    public class Enemy : Unit
    {
        public event EventHandler<Enemy> OnReachedPathEnd;

        int currentPathPoint;
        Path currentPath;
        UnitMovementController movementController;
        bool shouldLoop;

        public override void OnInitialize()
        {
            base.OnInitialize();
            movementController = AddComponent<UnitMovementController>();
            movementController.OnMoveFinished += UpdatePosition;

            Health.OnHealthChanged += OnHealthChanged;
            Health.OnDamageReceived += Health_OnDamageReceived;

            SetUnitData(AssetLibrary.Data.Enemies.Easy);
        }

        private void Health_OnDamageReceived(int arg)
        {
            UiHud.CreateHitMarker(Position, (text) =>
            {
                text.Label.ApplyStyle(AssetLibrary.TextStyles.HudWorldSpaceText);
                text.Label.Color = Color.Yellow;
                text.Label.Content = "-" + arg.ToString();
            });
        }

        /// <summary>
        /// Moves the Enemy to the target position.
        /// </summary>
        /// <param name="targetPosition">Target position.</param>
        public void MoveToPosition(Vec3 targetPosition)
        {
            currentPath = new Path();
            currentPath.AddPoint(targetPosition);
            UpdatePosition();
        }

        /// <summary>
        /// Moves the Enemy along a specified path of points.
        /// </summary>
        /// <param name="path">Path.</param>
        public void MoveAlongPath(Path path)
        {
            currentPath = path;
            UpdatePosition();
        }

        protected override void OnSetUnitData(UnitData unitData)
        {
            // Rephysicalize this entity after the geometry has been changed to that specified inside the UnitData.
            Physics.Physicalize();

            var movementData = unitData as IMovementData;
            if (movementData != null)
                movementController.MoveSpeed = movementData.MoveSpeed;
        }

        public override void OnDestroy()
        {
            base.OnDestroy();
            movementController.OnMoveFinished -= UpdatePosition;
            Health.OnHealthChanged -= OnHealthChanged;
        }

        protected override UiWorldSpaceElement CreateUI(Canvas canvas)
        {
            return SceneObject.Instantiate<UiEnemy>(canvas);
        }

        /// <summary>
        /// Updates the position of the Enemy by moving it along the currently assigned path.
        /// </summary>
        void UpdatePosition()
        {
            if (currentPath == null)
            {
                Log.Warning(Name + ": Cannot UpdatePosition as no path is currently assigned");
                return;
            }

            if (currentPathPoint == currentPath.Points.Count)
            {
                OnPathComplete();
            }
            else
            {
                movementController.MoveToPosition(currentPath.Points[currentPathPoint]);
                currentPathPoint += 1;
            }
        }

        void OnPathComplete()
        {
            // TODO: Implement logic. What *should* happen when we reach the end of the path?
            currentPathPoint = 0;

            if (shouldLoop)
                UpdatePosition();

            OnReachedPathEnd?.Invoke(this);
        }

        void OnHealthChanged ()
        {
            (UI as UiEnemy).Health = (float)Health.CurrentHealth / (float)Health.MaxHealth;
        }
    }
}