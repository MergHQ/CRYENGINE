using System;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine.Towerdefense
{
    public class Path : BaseEntity
    {
        List<Vec3> points;

        // TODO: Make an array of Vec3 once (or if) support comes along
        [EntityProperty]
        public Vec3 Start { get; set; }

        [EntityProperty]
        public Vec3 End { get; set; }

        public List<Vec3> Points { get { return points; } }

        public override void OnInitialize()
        {
            points = new List<Vec3>();
        }

        public Path()
        {
            points = new List<Vec3>();
        }

        public Path(Vec3 start, Vec3 end) : this()
        {
            points.Add(start);
            points.Add(end);
        }

        public void AddPoint(Vec3 point)
        {
            points.Add(point);
        }
    }
}

