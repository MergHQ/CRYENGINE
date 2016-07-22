using System;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Provides information and events about which Construction Point the mouse is currently over.
    /// </summary>
    class MouseController
    {
        public event EventHandler NoCell;
        public event EventHandler<ConstructionPoint> EnterOccupiedCell;
        public event EventHandler<ConstructionPoint> EnterUnoccupiedCell;
        public event EventHandler<ConstructionPoint> CellChanged;

        public ConstructionPoint Current { get; private set; }
        public ConstructionPoint Previous { get; private set; }
        public Func<bool> CanSelect { get; set; }

        public MouseController()
        {
            Mouse.OnMove += OnMouseMove;
        }

        public void Destroy()
        {
            Mouse.OnMove -= OnMouseMove;
        }

        void OnMouseMove(int x, int y)
        {
            var id = Mouse.HitEntityId;

            Previous = Current;
            Current = EntitySystem.EntityFramework.GetEntity(id) as ConstructionPoint;

            if (Previous != Current)
                CellChanged?.Invoke(Current);

            if (Current == null)
            {
                NoCell?.Invoke();
            }
            else if (!Current.Controller.Occupied && CanSelect())
            {
                EnterUnoccupiedCell?.Invoke(Current);
            }
            else
            {
                EnterOccupiedCell?.Invoke(Current);
            }
        }
    }
}

