using System;
using System.Collections.Generic;
using CryEngine.UI;
using CryEngine.EntitySystem;

namespace CryEngine.SampleApp
{
    public class UserInterface : UIElement
    {
        VerticalLayoutGroup verticalLayout;
        TextElement gameObjectInfo;
        TextElement controlsInfo;
        TextElement entityInfoElement;
        List<TextElement> textElements;

        public Canvas Canvas { get; private set; }

        public BaseEntity EntityInfo
        {
            set
            {
                entityInfoElement.Text.Content = value != null ? EntityPicker.GetEntityInfo(value) : "N/A";
            }
        }

        public void OnAwake()
        {
            Canvas = Instantiate<Canvas>(null);

            verticalLayout = Instantiate<VerticalLayoutGroup>(Canvas);
            verticalLayout.RectTransform.Size = new Point(Screen.Width, Screen.Height);
            verticalLayout.RectTransform.Alignment = Alignment.TopLeft;
            verticalLayout.RectTransform.Pivot = new Point(1f, 1f);
            verticalLayout.RectTransform.Padding = new Padding(10f, 10f);
            verticalLayout.Spacing = 5;

            // Create UI.
            var header = CreateText((element) =>
            {
                element.Text.Content = "Entity Spawner";
                element.Text.Height = 24;
                element.RectTransform.Height = 40;
            });

            gameObjectInfo = CreateText();

            controlsInfo = CreateText();
            controlsInfo.Text.Content = "Left-click to destroy an Entity";

            entityInfoElement = CreateText((element) =>
            {
                element.Text.Color = Color.Yellow;
            });

            // Assign text elements to vertical layout so they get displayed in rows.
            verticalLayout.Add(header);
            verticalLayout.Add(gameObjectInfo);
            verticalLayout.Add(controlsInfo);
            verticalLayout.Add(entityInfoElement);
        }

        public void OnUpdate()
        {
            gameObjectInfo.Text.Content = string.Format("Number of Entities: {0}", EntityFramework.Count);
        }

        public void OnDestroy()
        {
            textElements.ForEach(x => x.Destroy());
            textElements.Clear();
            verticalLayout.Destroy();
        }

        TextElement CreateText(Action<TextElement> onCreate = null)
        {
            if (textElements == null)
                textElements = new List<TextElement>();

            var instance = Instantiate<TextElement>(verticalLayout);

            instance.RectTransform.Alignment = Alignment.Top;
            instance.RectTransform.Pivot = new Point(0.5f, 1f);
            instance.Text.Alignment = Alignment.Left;
            instance.Height = 18;
            instance.Text.Color = Color.White;
            instance.Text.Content = "N/A";

            textElements.Add(instance);

            if (onCreate != null)
                onCreate(instance);

            return instance;
        }
    }
}