using System;
using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Framework;

namespace CryEngine.SampleApp
{
    public class TestUI
    {
        Text gameObjectTextInfo;
        Text mouseOverInfo;

        public TestUI(SceneObject root)
        {
            // Create UI
            var canvas = SceneObject.Instantiate<Canvas>(root);

            gameObjectTextInfo = canvas.AddComponent<Text>();
            gameObjectTextInfo.Alignment = Alignment.TopLeft;
            gameObjectTextInfo.Height = 28;
            gameObjectTextInfo.Offset = new Point(10, 10);
            gameObjectTextInfo.Color = Color.Yellow.WithAlpha(0.5f);
            gameObjectTextInfo.Content = "Number of GameObjects";

            mouseOverInfo = canvas.AddComponent<Text>();
            mouseOverInfo.Alignment = Alignment.TopLeft;
            mouseOverInfo.Height = 28;
            mouseOverInfo.Offset = new Point(10, 30);
            mouseOverInfo.Color = Color.Yellow.WithAlpha(0.5f);
        }

        public void Update(Test test)
        {
            gameObjectTextInfo.Content = "Number of GameObjects: " + GameObject.GameObjectCount;

            if (test.PlayerMouse.Current != null)
                mouseOverInfo.Content = string.Format("Mouse over gameobject: {0}[{1}]", test.PlayerMouse.Current.Name, test.PlayerMouse.Current.Entity.ID);
            mouseOverInfo.Active = test.PlayerMouse.Current != null;
        }
    }
}

