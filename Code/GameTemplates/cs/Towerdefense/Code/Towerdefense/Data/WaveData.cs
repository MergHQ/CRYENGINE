using System;
using System.Collections.Generic;

namespace CryEngine.Towerdefense
{
    public class WaveData
    {
        public List<WaveGroupData> Waves = new List<WaveGroupData>();
    }

    public class WaveGroupData
    {
        public List<WaveEnemyGroupData> EnemyGroups = new List<WaveEnemyGroupData>();
    }

    public class WaveEnemyGroupData
    {
        public EnemyType Type { get; set; }
        public int Amount { get; set; }
        public float? PostDelay { get; set; }
    }
}

