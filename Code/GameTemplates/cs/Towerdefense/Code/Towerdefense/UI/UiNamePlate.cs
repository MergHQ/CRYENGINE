using System;
using CryEngine.Resources;
using CryEngine.Common;
using CryEngine.UI.Components;
using CryEngine.UI;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class UiNamePlate : Panel
    {
        Text displayName;
        Text level;
        Image arrow;

        public string DisplayName
        {
            set
            {
                displayName.Content = value.ToUpper();
            }
        }

        public int Level
        {
            set
            {
                level.Content = string.Format("LVL {0}", value);
            }
        }

        public bool ShowLevel
        {
            set
            {
                level.Active = value;
            }
        }

        public override void OnAwake()
        {
            base.OnAwake();

            Background.Source = ResourceManager.ImageFromFile(AssetLibrary.UI.NamePlateBackground);
            Background.SliceType = SliceType.Nine;

            var arrowContainer = Instantiate<UIElement>(this);
            arrowContainer.RectTransform.Size = new Point(10f, 5f);
            arrowContainer.RectTransform.Alignment = Alignment.Bottom;
            arrowContainer.RectTransform.Pivot = new Point(0.5f, 1f);
            arrow = arrowContainer.AddComponent<Image>();
            arrow.Source = ResourceManager.ImageFromFile(AssetLibrary.UI.NamePlateArrow);

            displayName = AddComponent<Text>();
            displayName.Height = 14;
            displayName.DropsShadow = false;
            displayName.FontStyle = System.Drawing.FontStyle.Bold;
            displayName.Alignment = Alignment.Left;
            displayName.Offset = new Point(5f, 0f);

            level = AddComponent<Text>();
            level.Height = 14;
            level.DropsShadow = false;
            level.FontStyle = System.Drawing.FontStyle.Bold;
            level.Alignment = Alignment.Right;
            level.Offset = new Point(-10f, 0f);

            DisplayName = "N/A";
            Level = 0;
        }
    }
}

