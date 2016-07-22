using System;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class UiTextMessage : Panel
    {
        float timeStamp;
        float displayTime;

        public Text Text { get; private set; }

        public override void OnAwake()
        {
            base.OnAwake();
            Text = AddComponent<Text>();
            Background.Color = AssetLibrary.Colors.DarkGrey.WithAlpha(0f);
            Text.Color = Text.Color.WithAlpha(0f);
        }

        public void Show(string message, float time = -1f)
        {
            Text.Content = message;
            displayTime = time;
            timeStamp = Env.Timer.GetCurrTime();

            Background.Color.A = 0.8f;
            Text.Color.A = 1f;
        }

        public void OnUpdate()
        {
            if (Env.Timer.GetCurrTime() > timeStamp + displayTime)
            {
                if (Background.Color.A > 0.01f)
                {
                    Background.Color.A -= FrameTime.Delta;
                    Text.Color = new Color(Text.Color.R, Text.Color.G, Text.Color.B, Text.Color.A - FrameTime.Delta);
                }
                else
                {
                    Background.Color.A = 0f;
                    Text.Color = new Color(Text.Color.R, Text.Color.G, Text.Color.B, 0f);
                }
            }
        }
    }
}

