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
    /// Provides a world-space UI element.
    /// </summary>
    public class UiWorldSpaceElement : UIElement
    {
        Vec2 offset;

        protected Vec2 Position { get; private set; }

        public Vec2 Offset
        {
            protected get
            {
                return offset;
            }
            set
            {
                offset = value;
                UpdateLayout();
            }
        }

        public virtual void OnAwake()
        {
            Position = Vec2.Zero;
            offset = Vec2.Zero;

            RectTransform.Alignment = Alignment.TopLeft;
        }

        public void SetPosition(Vec2 screenPoint)
        {
            Position = new Vec2(screenPoint.x * Screen.Width, screenPoint.y * Screen.Height);
            UpdateLayout();
        }

        public void SetPosition(Vec3 worldPoint)
        {
            SetPosition(Env.Renderer.ProjectToScreen(worldPoint));
        }

        void UpdateLayout()
        {
            RectTransform.Padding = new Padding(Position.x + offset.x, Position.y + offset.y);
            RectTransform.PerformLayout(true);
        }
    }
}

