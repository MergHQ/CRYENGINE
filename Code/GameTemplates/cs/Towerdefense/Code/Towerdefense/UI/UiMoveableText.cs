using System;
using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class UiMoveableText : UIElement
    {
        bool isMoving;
        Timer timer;

        public Text Label { get; private set; }
        public bool FadeOut { get; set; } = true;
        public float Speed { get; set; } = 5f;
        public Vec2 Direction { get; set; } = Vec2.Up;
        public float TimeToLive { get; set; } = 5f;

        public void OnAwake()
        {
            Label = AddComponent<Text>();
            Label.ApplyStyle(AssetLibrary.TextStyles.HudProperty);
            Label.DropsShadow = true;
        }

        public void Move(float speed, Vec2 direction, float timeToLive)
        {
            Speed = speed;
            Direction = direction;
            TimeToLive = timeToLive;

            Move();
        }

        public void Move()
        {
            isMoving = true;
            timer = new Timer(TimeToLive, () => Destroy());
        }

        public void OnUpdate()
        {
            if (isMoving)
            {
                float t = FrameTime.Delta * Speed;
                RectTransform.Padding = new Padding(RectTransform.Padding.Left + Direction.x * t, RectTransform.Padding.Top - Direction.y * t);

                // Fade out label
                if (FadeOut && timer.Time > TimeToLive / 2f && Label.Color.A > 0f)
                {
                    Label.Color.A -= 0.1f;
                }
                else if (timer.Time > TimeToLive)
                {
                    Label.Color.A = 0f;
                }

                if (timer.Time > TimeToLive)
                    Destroy();
            }
        }
    }
}

