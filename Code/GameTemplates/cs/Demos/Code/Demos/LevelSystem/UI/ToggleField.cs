using CryEngine.UI;
using CryEngine.UI.Components;

namespace CryEngine.LevelSuite.UI
{
    public class ToggleField : FieldElement
    {
        public event EventHandler<bool> ValueChanged;

        private Button _toggle;
        private bool _on;

        public Text ValueText { get; private set; }

        public bool On
        {
            set
            {
                _on = value;
                ValueText.Content = _on ? "True" : "False";
                ValueChanged?.Invoke(value);
            }
        }

        public override void OnAwake()
        {
            base.OnAwake();

            ValueText = AddComponent<Text>();
            ValueText.Alignment = Alignment.Center;
            ValueText.Content = "N/A";
            ValueText.Height = FieldSettings.TextHeight;

            _toggle = Instantiate<Button>(this);
            _toggle.RectTransform.Alignment = Alignment.Left;
            _toggle.RectTransform.Pivot = new Point(1f, 0.5f);
            _toggle.RectTransform.Size = new Point(20f, 20f);
            _toggle.Ctrl.OnPressed += Ctrl_OnPressed;
        }

        private void Ctrl_OnPressed()
        {
            On = !_on;
        }
    }
}
