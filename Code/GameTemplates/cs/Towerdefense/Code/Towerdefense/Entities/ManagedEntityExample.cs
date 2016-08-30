using System;
using CryEngine.EntitySystem;

namespace CryEngine.Towerdefense
{
    public class ManagedEntityExample : BaseEntity
    {
        [EntityProperty]
        public Int32 ExposedInteger { get; set; }
    }
}

