using CryEngine.UI;
using CryEngine.UI.Components;

namespace CryEngine.LevelSuite.UI
{
    public class SliderField : FieldElement
    {
        public Text MinValueText { get; private set; }
        public Text MaxValueText { get; private set; }
        public Text ValueText { get; private set; }
        public Slider Slider { get; private set; }

        public override void OnAwake()
        {
            base.OnAwake();

            MinValueText = AddComponent<Text>();
            MinValueText.Alignment = Alignment.TopLeft;
            MinValueText.Content = "Min";
            MinValueText.Height = FieldSettings.TextHeight;

            MaxValueText = AddComponent<Text>();
            MaxValueText.Alignment = Alignment.TopRight;
            MaxValueText.Content = "Max";
            MaxValueText.Height = FieldSettings.TextHeight;

            Slider = Instantiate<Slider>(this);
            Slider.RectTransform.Alignment = Alignment.Bottom;
            Slider.RectTransform.Padding = new Padding(0, -FieldSettings.Padding);
            Slider.ValueChanged += Slider_OnValueChanged;
            Slider.MinValueChanged += Slider_MinValueChanged;
            Slider.MaxValueChanged += Slider_MaxValueChanged;

            ValueText = AddComponent<Text>();
            ValueText.Alignment = Alignment.Top;
            ValueText.Content = "Value";
            ValueText.Height = FieldSettings.TextHeight;
        }

        private void Slider_MaxValueChanged(float arg)
        {
            MaxValueText.Content = arg.ToString("F2");
        }

        private void Slider_MinValueChanged(float arg)
        {
            MinValueText.Content = arg.ToString("F2");
        }

        private void Slider_OnValueChanged(float arg)
        {
            ValueText.Content = arg.ToString("F2");
        }

        public override void OnDestroy()
        {
            base.OnDestroy();
            Slider.ValueChanged -= Slider_OnValueChanged;
            Slider.MinValueChanged -= Slider_MinValueChanged;
            Slider.MaxValueChanged -= Slider_MaxValueChanged;
        }

        protected override void OnRectTransformChanged()
        {
            Slider.RectTransform.Width = RectTransform.Width;
            Slider.RectTransform.Height = RectTransform.Height;
        }
    }
}
