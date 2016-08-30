using System;
using CryEngine.Common;
using CryEngine.Resources;
using CryEngine.UI;
using CryEngine.UI.Components;

namespace CryEngine.Framework
{
    public class FillBar : Panel
    {
        public event EventHandler<float> OnValueChanged;

        Panel fill;
        float fillValue;
        float fillBarPadding;
        bool updateLayout = false;

        public float FillBarPadding
        {
            set
            {
                fillBarPadding = value;
                updateLayout = true;
            }
        }

        public int Height
        {
            set
            {
                RectTransform.Height = value;
                updateLayout = true;
            }
        }

        public float Fill
        {
            get
            {
                return fillValue;
            }
            set
            {
                fillValue = MathEx.Clamp<float>(value, 0.01f, 1f);

                var width = RectTransform.Width - fillBarPadding;
                fill.RectTransform.Padding = new Padding(fillBarPadding / 2f, 0);
                fill.RectTransform.Width = MathEx.Clamp<float>(width * fillValue, 1f, RectTransform.Width);
                fill.RectTransform.Height = RectTransform.Height - fillBarPadding;
                OnValueChanged?.Invoke(fillValue);
            }
        }

        public string BackgroundUrl
        {
            set
            {
                Background.Source = ResourceManager.ImageFromFile(value);
            }
        }

        public Image FillImage
        {
            get
            {
                return fill.Background;
            }
        }

        public string FillImageUrl
        {
            set
            {
                fill.Background.Source = ResourceManager.ImageFromFile(value);
            }
        }

        public override void OnAwake()
        {
            base.OnAwake();

            RectTransform.Alignment = Alignment.Left;
            RectTransform.Pivot = new Point(1f, 0.5f);

            RectTransform.LayoutChanged += OnLayoutChanged;

            fill = SceneObject.Instantiate<Panel>(this);
            fill.RectTransform.Size = new Point(RectTransform.Width, RectTransform.Height);
            fill.RectTransform.Alignment = Alignment.Left;
            fill.RectTransform.Pivot = new Point(1f, 0.5f);

            updateLayout = true;
        }

        public void OnUpdate()
        {
            if (updateLayout)
            {
                RectTransform.Alignment = RectTransform.Alignment;
                Fill = fillValue;
                updateLayout = false;
            }
        }

        public void OnDestroy()
        {
            RectTransform.LayoutChanged -= OnLayoutChanged;
        }

        void OnLayoutChanged ()
        {
            updateLayout = true;
        }
    }
}

