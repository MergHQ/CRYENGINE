using System;
using CryEngine.Common;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Represents a point in the world where a Tower can be placed. Used in conjunction with the Inventory class.
    /// </summary>
    public class ConstructionPointController
    {
        public event EventHandler OnOccupiedStateChanged;
        public event EventHandler<Tower> OnUnitRemoved;
        public event EventHandler<Tower> OnUnitPlaced;

        bool occupied;
        ConstructionPoint view;

        public Tower Tower { get; private set; }
        public Vec3 Position { get; private set; }
        public Quat Rotation { get; private set; }
        public int Size { get; private set; }

        public bool Occupied
        {
            get
            {
                return occupied;
            }
            set
            {
                occupied = value;
                if (OnOccupiedStateChanged != null)
                    OnOccupiedStateChanged();
            }
        }

        public ConstructionPointController(Vec3 position, Quat rotation, int size, ConstructionPoint view)
        {
            Position = position;
            Rotation = rotation;
            Size = size;
            this.view = view;
            this.view.Controller = this;
        }

        /// <summary>
        /// Creates an instance of a Tower based on the specified UnitTowerData and places it at this Construction Point.
        /// </summary>
        /// <param name="unitTowerData"></param>
        public void PlaceTower(UnitTowerData unitTowerData)
        {
            if (Tower == null && !Occupied)
            {
                Tower = Factory.CreateUnit(unitTowerData) as Tower;
                Tower.Place(Position);
                Occupied = true;
                OnUnitPlaced?.Invoke(Tower);
            }
        }

        /// <summary>
        /// Destroys the Tower referenced by this Construction Point.
        /// </summary>
        /// <param name="onRemove"></param>
        public void RemoveTower(Action<Tower> onRemove = null)
        {
            if (Tower != null)
            {
                if (OnUnitRemoved != null)
                    OnUnitRemoved(Tower);
                if (onRemove != null)
                    onRemove(Tower);
                Occupied = false;
                Tower.Destroy();
                Tower = null;
            }
        }

        public void Destroy()
        {
            Tower?.Destroy();
            Env.EntitySystem.RemoveEntity(view.Id);
        }
    }
}

