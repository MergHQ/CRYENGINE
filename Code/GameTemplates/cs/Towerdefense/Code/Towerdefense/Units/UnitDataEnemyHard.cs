using System;
using CryEngine.Common;

namespace CryEngine.Towerdefense
{
    public class UnitDataEnemyHard : UnitData, IMovementData
    {
        public override int MaxHealth { get { return 100; } }
        public override UnitType Type { get { return UnitType.Enemy; }}
        public override string Mesh { get { return AssetLibrary.Meshes.Enemy; } }
        public override string Material { get { return AssetLibrary.Materials.Red; } }

        #region Movement Info
        float IMovementData.MoveSpeed { get { return 0.5f; } }
        #endregion
    }
}