using CryEngine.Common;
using CryEngine.Framework;

namespace CryEngine.Framework
{
    public interface IMouseUp
    {
        void OnMouseUp(MouseButton mouseButton);
    }

    public interface IMouseDown
    {
        void OnMouseDown(MouseButton mouseButton);
    }

    public interface IMouseEnter
    {
        void OnMouseEnter();
    }

    public interface IMouseLeave
    {
        void OnMouseLeave();
    }

    public class WorldMouse : IGameUpdateReceiver
    {
        public event EventHandler<GameObject> OnMouseOver;

        public bool MouseOverGameObject { get; private set; }
        public GameObject Current { get; private set; }

        GameObject previous;

        public WorldMouse()
        {
            Mouse.OnLeftButtonDown += OnLeftButtonDown;
            Mouse.OnLeftButtonUp += OnLeftButtonUp; 
            Mouse.OnRightButtonDown += OnRightButtonDown;
            Mouse.OnRightButtonUp += OnRightButtonUp;

            GameFramework.RegisterForUpdate(this);
        }

        public void Destroy()
        {
            Mouse.OnLeftButtonDown -= OnLeftButtonDown;
            Mouse.OnLeftButtonUp -= OnLeftButtonUp; 
            Mouse.OnRightButtonDown -= OnRightButtonDown;
            Mouse.OnRightButtonUp -= OnRightButtonUp;

            GameFramework.UnregisterFromUpdate(this);
        }

        void IGameUpdateReceiver.OnUpdate()
        {
            // Get the ID of the current Entity under the mouse.
            var id = Mouse.HitEntityId;

            // Get the current GameObject based on ID. If nothing is currently under the cursor, this will return null.
            Current = GameObject.ById((uint)id);

            // Fire callback
            if (OnMouseOver != null)
                OnMouseOver(Current);

            if (previous != Current)
            {
                // Call IMouseLeave in the Previous object if it implements the interface.
                var mouseLeave = previous as IMouseLeave;
                if (mouseLeave != null)
                    mouseLeave.OnMouseLeave();

                // Call IMouseEnter in the Current object if it implements the interface.
                var mouseEnter = Current as IMouseEnter;
                if (mouseEnter != null)
                    mouseEnter.OnMouseEnter();
            }

            previous = Current;
        }

        void OnLeftButtonDown(int x, int y)
        {
            OnMouseDown(MouseButton.Left);
        }

        void OnLeftButtonUp (int x, int y)
        {
            OnMouseUp(MouseButton.Left);
        }

        void OnRightButtonDown(int x, int y)
        {
            OnMouseDown(MouseButton.Right);
        }

        void OnRightButtonUp (int x, int y)
        {
            OnMouseUp(MouseButton.Right);
        }

        void OnMouseDown(MouseButton button)
        {
            if (Current != null)
            {
                var mouseDown = Current as IMouseDown;
                if (mouseDown != null)
                    mouseDown.OnMouseDown(button);
            }
        }

        void OnMouseUp(MouseButton button)
        {
            if (Current != null)
            {
                var mouseUp = Current as IMouseUp;
                if (mouseUp != null)
                    mouseUp.OnMouseUp(button);
            }
        }
    }
}

