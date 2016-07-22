using System;

namespace CryEngine.Towerdefense
{
    public class UnitDataEnemy : UnitData
    {
        public override int MaxHealth { get { return 25; } }
        public override UnitType Type { get { return UnitType.Enemy; }}
        public override string Mesh { get { return AssetLibrary.Meshes.Enemy; } }
        public override string Material { get { return AssetLibrary.Materials.Red; } }
    }
}