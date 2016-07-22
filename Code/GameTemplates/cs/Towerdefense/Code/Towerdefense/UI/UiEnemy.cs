using System;
using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Framework;
using CryEngine.Resources;
using CryEngine.Towerdefense;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// The UI of the Unit such as their Healthbar, Name, etc
    /// </summary>
    public class UiEnemy : UiWorldSpaceElement
    {
        FillBar fillBar;

        public float Health
        {
            set
            {
                fillBar.Fill = value;
            }
        }

        public override void OnAwake()
        {
            base.OnAwake();

            RectTransform.Size = new Point(75f, 45f);

            fillBar = SceneObject.Instantiate<FillBar>(this);
            fillBar.RectTransform.Alignment = Alignment.Center;
            fillBar.RectTransform.Pivot = new Point(0.5f, 0.5f);
            fillBar.RectTransform.Size = new Point(RectTransform.Width, 5f);
            fillBar.BackgroundUrl = AssetLibrary.UI.FillBarBackground;
            fillBar.Background.SliceType = SliceType.Nine;
            fillBar.FillImage.Color = AssetLibrary.Colors.Blue;
            fillBar.FillBarPadding =  2f;
        }
    }
}

