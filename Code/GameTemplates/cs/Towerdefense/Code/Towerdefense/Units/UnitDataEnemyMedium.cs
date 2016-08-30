using System;

namespace CryEngine.Towerdefense
{
    public class UnitDataEnemyMedium : UnitData, IMovementData
    {
        public override int MaxHealth { get { return 50; } }
        public override UnitType Type { get { return UnitType.Enemy; }}
        public override string Mesh { get { return AssetLibrary.Meshes.Enemy; } }
        public override string Material { get { return AssetLibrary.Materials.Yellow; } }

        #region Movement Info
        float IMovementData.MoveSpeed { get { return 0.75f; } }
        #endregion
    }
}