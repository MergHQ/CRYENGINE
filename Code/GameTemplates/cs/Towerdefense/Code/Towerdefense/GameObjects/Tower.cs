using System;
using System.Collections.Generic;
using System.Linq;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Framework;

using CryEngine.Towerdefense.Components;
using CryEngine.EntitySystem;
using CryEngine.UI;
using CryEngine.UI.Components;

namespace CryEngine.Towerdefense
{
    public class Tower : Unit
    {
        TurretRotator rotator;
        TowerSensor sensor;
        Weapon weapon;
        GameObject target;

        public int Level { get; private set; }
        public bool CanUpgrade { get { return Level != Data.UpgradePath.Count - 1; } }
        public UnitTowerData Data { get { return UnitData as UnitTowerData; } }
        public TowerData CurrentTower { get { return Data.UpgradePath[Level]; } }
        public TowerData NextTower { get { return CanUpgrade ? Data.UpgradePath[Level + 1] : null; } }

        /// <summary>
        /// Places and initializes the Tower at the specified position.
        /// </summary>
        /// <param name="position">Position.</param>
        public void Place(Vec3 position)
        {
            Entity.Position = position + new Vec3(0, 0, 2f);

            AddComponent<SineWaveMovement>();
            weapon = AddComponent<WeaponProjectile>();
            sensor = AddComponent<TowerSensor>();
            rotator = AddComponent<TurretRotator>();

            // Set weapon target when an enemy is in range
            sensor.OnTargetChanged += Sensor_OnTargetChanged;

            // Stop firing weapon once no targets are in range
            sensor.OnNoTargetsFound += Sensor_OnNoTargetsFound;

            // Set rotator which controls the rotation of the tower's turret
            rotator.AimingFinished += Rotator_AimingFinished;

            SetupWeapon();
        }

        /// <summary>
        /// Upgrades the tower if another upgrade is available.
        /// </summary>
        public void Upgrade()
        {
            if (NextTower != null)
            {
                NativeEntity.SetMaterial(NextTower.Material);
                SetupWeapon();
                Level++;
            }
            else
            {
                Log.Warning("Cannot Upgrade unit because there is no upgrade path!");
            }
        }

        public override void OnDestroy()
        {
            base.OnDestroy();
            sensor.OnTargetChanged -= Sensor_OnTargetChanged;
            sensor.OnNoTargetsFound -= Sensor_OnNoTargetsFound;
            rotator.AimingFinished -= Rotator_AimingFinished;
        }

        protected override void OnSetUnitData(UnitData unitData)
        {
            base.OnSetUnitData(unitData);

            // When the tower's unit data is set, set the material to the first object in it's UpgradPath data.
            NativeEntity.SetMaterial(CurrentTower.Material);
        }

        protected override void OnUpdateUnit(LevelManager level)
        {
            var potentialTargets = level.Enemies.Cast<GameObject>().ToList();
            sensor.PotentialTargets = potentialTargets;
            weapon.PotentialTargets = potentialTargets;
        }

        /// <summary>
        /// Sets the properties of the weapon based on the current tower's multiplier values.
        /// </summary>
        void SetupWeapon()
        {
            if (Data != null)
            {
                sensor.Range = Data.BaseRange * CurrentTower.RangeMultiplier;
                weapon.FireRate = Data.BaseFireRate * CurrentTower.FireRateMultiplier;
                weapon.DamageOutput = (int)(Data.BaseDamageOutput * CurrentTower.DamageMultiplier);
                weapon.OnHitEffects = Data.OnHitEffects;
                rotator.RotateSpeed = Data.RotateSpeed;
            }
        }

        /// <summary>
        /// Invoked by the Sensor's OnNoTargetsFound. Clears the Weapon's target.
        /// </summary>
        void Sensor_OnNoTargetsFound ()
        {
            weapon.ClearTarget();
            rotator.ClearTarget();
        }

        /// <summary>
        /// Invoked by the Sensor's OnTargetChanged callback. Sets the Weapon to the new target.
        /// </summary>
        /// <param name="target">Target.</param>
        void Sensor_OnTargetChanged (GameObject target)
        {
            this.target = target;
            rotator.Target = target;
        }

        void Rotator_AimingFinished()
        {
            weapon.Target = target;
        }
    }
}

