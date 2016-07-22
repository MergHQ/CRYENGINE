using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CryEngine.Towerdefense
{
    public enum UnitType
    {
        Tower,
        Enemy,
        // More unit types can be added here.
    }

    public enum EnemyType
    {
        Easy,
        Medium,
        Hard,
    }

    public enum UnitFaction
    {
        Friendly,
        Enemy,
    }

    public enum EffectType
    {
        Damage,
        Slow,
    }
}
