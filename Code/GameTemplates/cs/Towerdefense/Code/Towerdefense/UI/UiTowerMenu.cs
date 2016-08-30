using System;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Framework;
using CryEngine.Resources;
using CryEngine.Towerdefense;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// The UI that is shown to the Player to allow them to Upgrade and Sell the tower.
    /// </summary>
    public class UiTowerMenu : UiWorldSpaceElement
    {
        UiNamePlate namePlate;
        VerticalLayoutGroup menu;

        const float menuSpacing = 5f;
        const float namePlateHeight = 20f;
        readonly Point buttonSize = new Point(20f, 20f);

        public UiNamePlate NamePlate { get { return namePlate; } }
        protected VerticalLayoutGroup Menu { get { return menu; } }
        protected virtual float Width { get { return 180f; } }

        public override void OnAwake()
        {
            base.OnAwake();

            RectTransform.Size = new Point(Width, 210f);
            RectTransform.Pivot.y = 1f;

            namePlate = SceneObject.Instantiate<UiNamePlate>(this);
            namePlate.RectTransform.Size = new Point(RectTransform.Width, namePlateHeight);
            namePlate.RectTransform.Alignment = Alignment.Top;

            // Creates a vertical menu for the Sell and Upgrade buttons.
            menu = SceneObject.Instantiate<VerticalLayoutGroup>(this);
            menu.RectTransform.Size = new Point(RectTransform.Width, 10f);
            menu.RectTransform.Alignment = Alignment.Top;
            menu.RectTransform.Padding = new Padding(0f, 140f);
            menu.Background.Source = ResourceManager.ImageFromFile(AssetLibrary.UI.PanelBackground);
            menu.Background.SliceType = SliceType.Nine;
            menu.Spacing = menuSpacing;
        }

        protected UiTowerButton AddButton()
        {
            var button = SceneObject.Instantiate<UiTowerButton>(menu);
            button.Label.Content = "N/A";
            button.RectTransform.Size = new Point(menu.RectTransform.Width - (menuSpacing * 2f), 20f);
            button.RectTransform.Alignment = Alignment.Top;
            button.Button.RectTransform.Size = buttonSize;
            return button;
        }

        protected class UiTowerButton : Panel
        {
            public event EventHandler MouseEnter;
            public event EventHandler MouseLeft;

            bool wasInside;

            const float spacing = 20f;

            public Button Button { get; private set; }
            public Text Label { get; private set; }

            public override void OnAwake()
            {
                base.OnAwake();

                Background.Source = ResourceManager.ImageFromFile(AssetLibrary.UI.ButtonSquare);
                Background.SliceType = SliceType.Nine;

                Button = SceneObject.Instantiate<Button>(this);
                Button.RectTransform.Alignment = Alignment.Left;
                Button.RectTransform.Pivot = new Point(1f, 0.5f);
                Button.BackgroundImageUrl = AssetLibrary.UI.ButtonSquareBlue;
                Button.Background.SliceType = SliceType.Nine;

                Label = AddComponent<Text>();
                Label.FontStyle = System.Drawing.FontStyle.Bold;
                Label.Alignment = Alignment.Left;
                Label.Offset = new Point(Button.RectTransform.Width + spacing, Label.Offset.y);
                Label.DropsShadow = false;
                Label.FontStyle = System.Drawing.FontStyle.Bold;
                Label.Height = 14;
            }

            public void OnUpdate()
            {
                if (!wasInside && Mouse.CursorInside(this))
                {
                    wasInside = true;
                    MouseEnter?.Invoke();
                }

                if (wasInside && !Mouse.CursorInside(this))
                {
                    wasInside = false;
                    MouseLeft?.Invoke();
                }
            }
        }
    }
}

