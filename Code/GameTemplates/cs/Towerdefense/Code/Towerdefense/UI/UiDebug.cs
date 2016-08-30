using System;
using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class UiDebug : UIElement
    {
        Text gameObjectTextInfo;

        public void OnAwake()
        {
            // Create UI
            var canvas = SceneObject.Instantiate<Canvas>(Parent);

            gameObjectTextInfo = canvas.AddComponent<Text>();
            gameObjectTextInfo.Alignment = Alignment.TopLeft;
            gameObjectTextInfo.Height = 14;
            gameObjectTextInfo.Offset = new Point(10, 100);
            gameObjectTextInfo.Color = Color.Yellow;
            gameObjectTextInfo.Content = "Number of GameObjects";
        }

        public void OnUpdate()
        {
            gameObjectTextInfo.Content = "Number of GameObjects: " + GameObject.Count;
        }
    }
}

