using CryEngine.UI;
using CryEngine.UI.Components;

namespace CryEngine.LevelSuite.UI
{
    public class OptionField : FieldElement
    {
        public event EventHandler LeftPressed;
        public event EventHandler RightPressed;

        private Button _left;
        private Button _right;
        private const float _buttonSize = 20f;

        private Text _optionText;
        public string OptionText { set { _optionText.Content = value; } }

        public override void OnAwake()
        {
            base.OnAwake();

            _optionText = AddComponent<Text>();
            _optionText.Content = "Option 1";
            _optionText.Height = 14;
            _optionText.Alignment = Alignment.Center;

            _left = MakeButton(Alignment.Left);
            _left.Ctrl.OnPressed += Left_OnPressed;

            _right = MakeButton(Alignment.Right);
            _right.Ctrl.OnPressed += Right_OnPressed;
        }

        private void Right_OnPressed()
        {
            RightPressed?.Invoke();
        }

        private void Left_OnPressed()
        {
            LeftPressed?.Invoke();
        }

        Button MakeButton(Alignment alignment)
        {
            var button = Instantiate<Button>(this);
            button.RectTransform.Size = new Point(_buttonSize, _buttonSize);
            button.RectTransform.Alignment = alignment;
            button.RectTransform.Pivot = alignment == Alignment.Left ? new Point(1f, 0.5f) : new Point(0f, 0.5f);
            return button;
        }
    }
}
