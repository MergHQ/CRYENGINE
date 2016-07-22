using System;
using CryEngine.Common;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public static class Factory
    {
        /// <summary>
        /// Instantiates and returns a Unit based on the provided UnitData.
        /// </summary>
        /// <param name="unitData"></param>
        /// <returns></returns>
        public static Unit CreateUnit(UnitData unitData)
        {
            Unit unit = null;
            switch (unitData.Type)
            {
                case UnitType.Tower: unit = GameObject.Instantiate<Tower>(); break;
                case UnitType.Enemy: unit = GameObject.Instantiate<Enemy>(); break;
            }

            unit.SetUnitData(unitData);

            return unit;
        }

        /// <summary>
        /// Instantiates a Ghost version of a Unit based on the provided UnitData.
        /// </summary>
        /// <returns>The ghost.</returns>
        /// <param name="unitData">Unit data.</param>
        public static Unit CreateGhost(UnitData unitData)
        {
            var unit = GameObject.Instantiate<Unit>();
            unit.SetUnitData(unitData);
            unit.NativeEntity.SetMaterial(AssetLibrary.Materials.Ghost);
            unit.ShowUI = false;

            // Remove collision
            var physParams = new SEntityPhysicalizeParams()
            {
                type = (int)EPhysicalizationType.ePT_None,
            };

            unit.NativeEntity.Physicalize(physParams);

            return unit;
        }
    }
}

