using System;
using CryEngine.Towerdefense;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Handles selection input via the Mouse.
    /// </summary>
    class InputController
    {
        public event EventHandler<InputEvent> Select;
        public event EventHandler SelectionCleared;

        public class InputEvent : EventArgs
        {
            public ConstructionPoint Selection { get; private set; }
            public bool SelectionChanged { get; private set; }

            public InputEvent(ConstructionPoint selection, bool changed = false)
            {
                Selection = selection;
                SelectionChanged = changed;
            }
        }

        MouseController mouseController;
        ConstructionPoint previousSelection;

        public ConstructionPoint Selection { get; private set; }

        public InputController(MouseController mouseController)
        {
            this.mouseController = mouseController;
            Mouse.OnLeftButtonDown += OnLeftButtonDown;
            Mouse.OnRightButtonDown += OnRightButtonDown;
        }

        public void Destroy()
        {
            Mouse.OnLeftButtonDown -= OnLeftButtonDown;
            Mouse.OnRightButtonDown -= OnRightButtonDown;
        }

        void OnLeftButtonDown(int x, int y)
        {
            if (mouseController.Current == null)
                return;

            previousSelection = Selection;
            Selection = mouseController.Current;

            if (!mouseController.CanSelect())
            {
                // Fire the Select event if the clicked object is the same as the previous one
                if (Selection == previousSelection)
                    InvokeSelect();
            }
            else
            {
                InvokeSelect();
            }
        }

        void OnRightButtonDown(int x, int y)
        {
            Selection = null;
            SelectionCleared?.Invoke();
        }

        void InvokeSelect()
        {
            Select.Invoke(new InputEvent(Selection, previousSelection != Selection));
        }
    }
}

