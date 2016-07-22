using System;
using System.Collections.Generic;
using CryEngine.UI;
using CryEngine.LevelSuite.UI;

namespace CryEngine.LevelSuite
{
    public class LevelUI : UIElement
    {
        private VerticalLayoutGroup _verticalLayout;
        private TextElement _header;
        private TextElement _footer;
        private List<TextElement> _textElements;

        private const float fieldHeight = 25f;
        private const float fieldWidth = 400f;

        public string Header { set { _header.Text.Content = value; } }
        public string Footer { set { _footer.Text.Content = value; } }

        public void OnAwake()
        {
            ActiveChanged += LevelUI_ActiveChanged;

            _verticalLayout = Instantiate<VerticalLayoutGroup>(Parent);
            _verticalLayout.RectTransform.Size = new Point(600f, Screen.Height);
            _verticalLayout.RectTransform.Alignment = Alignment.TopLeft;
            _verticalLayout.RectTransform.Pivot = new Point(1f, 1f);
            _verticalLayout.RectTransform.Padding = new Padding(10f, 10f);
            _verticalLayout.Spacing = 5;

            // Create UI.
            _header = AddText(_verticalLayout, (element) =>
            {
                element.Text.Content = "Header";
                element.Text.Height = 32;
                element.RectTransform.Height = 40;
            });

            // Assign text elements to vertical layout so they get displayed in rows.
            _verticalLayout.Add(_header);

            _footer = Instantiate<TextElement>(Parent);
            _footer.Text.Content = "";
            _footer.Text.Height = 32;
            _footer.RectTransform.Alignment = Alignment.Bottom;
            _footer.Text.Alignment = Alignment.Center;
            _footer.RectTransform.Padding = new Padding(0f, -40f);
        }

        private void LevelUI_ActiveChanged(SceneObject arg)
        {
            _verticalLayout.Active = arg.Active;
        }

        public void OnDestroy()
        {
            _textElements.ForEach(x => x.Destroy());
            _textElements.Clear();
            _verticalLayout.Destroy();
        }

        public TextElement AddInfo(string label, Action<TextElement> onCreate = null)
        {
            var instance = AddText(_verticalLayout);
            instance.Text.Content = label;
            _verticalLayout.Add(instance);

            if (onCreate != null)
                onCreate(instance);

            return instance;
        }

        public TextElement AddInfo(Action<TextElement> onCreate = null)
        {
            return AddInfo("", onCreate);
        }

        TextElement AddText(UIElement parent, Action<TextElement> onCreate = null)
        {
            if (_textElements == null)
                _textElements = new List<TextElement>();

            var instance = Instantiate<TextElement>(_verticalLayout);

            instance.RectTransform.Alignment = Alignment.Top;
            instance.RectTransform.Pivot = new Point(0.5f, 1f);
            instance.Text.Alignment = Alignment.Left;
            instance.Height = 20;
            instance.Text.Color = Color.White;
            instance.Text.Content = "N/A";

            _textElements.Add(instance);

            if (onCreate != null)
                onCreate(instance);

            return instance;
        }

        public SliderField AddSlider(string label, float min, float max, float defaultValue)
        {
            var field = AddField<SliderField>(label);

            field.ValueText.Content = defaultValue.ToString();
            field.Slider.MinValue = min;
            field.Slider.MaxValue = max;
            field.Slider.Value = defaultValue;

            return field;
        }

        public ToggleField AddToggle(string label, bool defaultValue)
        {
            var field = AddField<ToggleField>(label);
            field.On = defaultValue;

            return field;
        }

        public OptionsList<T> AddOptions<T>(string label, Action<OptionsList<T>> onCreate = null) where T : struct
        {
            var field = AddField<OptionField>(label);

            var optionList = new OptionsList<T>();
            field.LeftPressed += () => optionList.MovePrevious();
            field.RightPressed += () => optionList.MoveNext();

            optionList.ValueChanged += (arg) =>
            {
                field.OptionText = arg.Name;
            };

            if (onCreate != null)
                onCreate(optionList);

            return optionList;
        }

        public Divider AddDivider()
        {
            return AddElement<Divider>(5f);
        }

        /// <summary>
        /// Adds the UI Element to the vertical layout and prepares it to be displayed.
        /// </summary>
        private T AddField<T>(string name, float height = fieldHeight) where T : FieldElement
        {
            var instance = AddElement<Field<T>>(height);
            instance.FieldName = name;
            instance.RectTransform.Width = fieldWidth;
            instance.RectTransform.Alignment = Alignment.TopLeft;
            instance.RectTransform.Pivot = new Point(1f, 1f);

            // Add to the vertical list. This will also update its layout.
            _verticalLayout.Add(instance);

            return instance.GetControl<T>();
        }

        private T AddElement<T>(float height = fieldHeight) where T : UIElement
        {
            var instance = Instantiate<T>(_verticalLayout);

            instance.RectTransform.Alignment = Alignment.Top;
            instance.RectTransform.Pivot = new Point(0.5f, 1f);
            instance.RectTransform.Width = _verticalLayout.RectTransform.Width;
            instance.RectTransform.Height = height;

            _verticalLayout.Add(instance);

            return instance;
        }
    }
}