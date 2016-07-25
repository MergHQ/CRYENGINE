using CryEngine.UI;
using CryEngine.UI.Components;

namespace CryEngine.LevelSuite.UI
{
    public class FieldSettings
    {
        public float Padding { get { return 5f; } }
        public byte TextHeight { get { return 14; } }
    }

    /// <summary>
    /// Creates a horizontal field.
    /// </summary>
    public sealed class Field<TFieldElement> : UIElement where TFieldElement : FieldElement
    {
        private float _fieldWidth = 100f;
        private TextElement _fieldText;
        private FieldElement _fieldElement;
        private FieldSettings _fieldSettings;

        public float FieldWidth { set { _fieldWidth = value; } }
        public string FieldName { set { _fieldText.Text.Content = value; } }

        public void OnAwake()
        {
            _fieldSettings = new FieldSettings();

            RectTransform.LayoutChanged += RectTransform_LayoutChanged;

            //AddComponent<Image>().Color = Color.Black.WithAlpha(0.5f);

            _fieldText = Instantiate<TextElement>(this);
            _fieldText.RectTransform.Alignment = Alignment.Left;
            _fieldText.RectTransform.Pivot = new Point(1f, 0.5f);
            _fieldText.RectTransform.Width = _fieldWidth;
            _fieldText.RectTransform.Padding = new Padding(_fieldSettings.Padding, 0f);
            _fieldText.RectTransform.Height = _fieldSettings.TextHeight;
            _fieldText.Text.Content = "N/A";

            _fieldElement = Instantiate<TFieldElement>(this);
            _fieldElement.RectTransform.Alignment = Alignment.Right;
            _fieldElement.RectTransform.Pivot = new Point(0f, 0.5f);
            _fieldElement.RectTransform.Padding = new Padding(-_fieldSettings.Padding, 0f);
        }

        public T GetControl<T>() where T : FieldElement
        {
            if (_fieldElement != null)
                return _fieldElement as T;
            return null;
        }

        public void OnDestroy()
        {
            RectTransform.LayoutChanged -= RectTransform_LayoutChanged;
        }

        private void RectTransform_LayoutChanged()
        {
            _fieldText.RectTransform.Height = RectTransform.Height;

            _fieldElement.RectTransform.Height = RectTransform.Height;
            _fieldElement.RectTransform.Width = RectTransform.Width - _fieldWidth;
        }
    }

    public class FieldElement : UIElement
    {
        protected FieldSettings FieldSettings { get; private set; }

        public virtual void OnAwake()
        {
            FieldSettings = new FieldSettings();

            RectTransform.LayoutChanged += OnRectTransformChanged;
        }

        protected virtual void OnRectTransformChanged() { }

        public virtual void OnDestroy()
        {
            RectTransform.LayoutChanged -= OnRectTransformChanged;
        }
    }
}
