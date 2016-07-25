using System;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine.Towerdefense
{
    public class Grid : BaseEntity
    {
        [EntityProperty]
        public int Rows { get; set; }

        [EntityProperty]
        public int Columns { get; set; }

        [EntityProperty]
        public int CellSize { get; set; }

        public List<ConstructionPoint> Cells = new List<ConstructionPoint>();

        public Grid(int columns, int rows, int cellSize)
        {
            Columns = columns;
            Rows = rows;
            CellSize = cellSize;
            Setup();
        }

        public override void OnInitialize()
        {
            Cells = new List<ConstructionPoint>();
            Setup();
        }

        public override void OnPropertiesChanged()
        {
            Setup();
        }

        public void Setup()
        {
            Cells.ForEach(x => x.Controller.Destroy());
            Cells.Clear();

            if (Rows == 0 || Columns == 0 || CellSize == 0)
                return;

            var pos = NativeEntity.GetPos();

            // Set origin to top-left corner of the grid. This is so the pivot of the object can remain at the center.
            var origin = new Vec3(pos.x - (float)(Columns / 2) * CellSize, pos.y + (float)(Rows / 2) * CellSize, 0f);

            for (int x = 0; x < Columns; x++)
            {
                for (int y = 0; y < Rows; y++)
                {
                    //var view = GameObject.Instantiate<ConstructionPoint>();
                    //var cell = new ConstructionPoint(new Vec3(origin.x + (x * CellSize) + CellSize / 2, origin.y - (y * CellSize) - CellSize / 2, 0), Quat.Identity, 1, view);
//                    Cells.Add(cell);
                }
            }
        }
    }
}
