using System;

namespace CryEngine.Towerdefense
{
    public class UnitDataEnemyEasy : UnitData, IMovementData
    {
        public override int MaxHealth { get { return 25; } }
        public override UnitType Type { get { return UnitType.Enemy; }}
        public override string Mesh { get { return "objects/default/primitive_sphere.cgf"; } }
        public override string Material { get { return AssetLibrary.Materials.Green; } }

        #region Movement Info
        float IMovementData.MoveSpeed { get { return 1f; } }
        #endregion
    }
}