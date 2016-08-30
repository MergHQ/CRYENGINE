using System;
using System.Collections.Generic;
using CryEngine.Common;

namespace CryEngine.Towerdefense
{
    public interface IUnitWeaponData
    {
        int DamageOutput { get; }
        float FireRate { get; }
        float Range { get; }
    }

    public interface IUnitBuildData
    {
        List<TowerData> UpgradePath { get; }
    }

    public interface IMovementData
    {
        float MoveSpeed { get; }
    }

    public abstract class UnitData
    {
        public abstract int MaxHealth { get; }
        public abstract UnitType Type { get; }

        public virtual string Name { get { return ""; } }
        public virtual int Level { get { return 0; } }
        public virtual string Mesh { get { return "objects/default/primitive_sphere.cgf"; } }
        public virtual string Material { get { return AssetLibrary.Materials.White; } }
    }
}