using System;
using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;

namespace CryEngine.Towerdefense
{
    public class UiFrontend
    {
        public event EventHandler StartGamePressed;
        public event EventHandler DebugPressed;
        public event EventHandler QuitGamePressed;

        Canvas canvas;

        public UiFrontend()
        {
            canvas = SceneObject.Instantiate<Canvas>(null);
            canvas.AddComponent<Image>().Color = Color.Black.WithAlpha(0.8f);

            // Start Game button
            AddButton(AssetLibrary.Strings.StartGame).Ctrl.OnPressed += () => StartGamePressed?.Invoke();

            // Debug button
            AddButton("Load Debug Level", (button) =>
            {
                button.Ctrl.OnPressed += () => DebugPressed?.Invoke();
                    button.RectTransform.Padding = new Padding(0, 35f);
            });

            // Quit Game button
            AddButton(AssetLibrary.Strings.Quit, (button) =>
            {
                button.Ctrl.OnPressed += () => QuitGamePressed?.Invoke();
                button.RectTransform.Padding = new Padding(0, 70f);
            });
        }

        public void Show()
        {
            canvas.Active = true;
        }

        public void Hide()
        {
            canvas.Active = false;
        }

        Button AddButton(string label, Action<Button> onAdd = null)
        {
            var button = SceneObject.Instantiate<Button>(canvas);
            button.RectTransform.Size = new Point(300f, 30f);
            button.RectTransform.Alignment = Alignment.Center;
            button.Ctrl.Text.Content = label;

            if (onAdd != null)
                onAdd(button);

            return button;
        }
    }
}

