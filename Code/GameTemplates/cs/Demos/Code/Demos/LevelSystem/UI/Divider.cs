using CryEngine.UI;
using CryEngine.UI.Components;

namespace CryEngine.LevelSuite.UI
{
    public class Divider : UIElement
    {
        private Panel _divider;

        private float _height = 1;

        /// <summary>
        /// The height of the divider line.
        /// </summary>
        public float Height
        {
            set
            {
                _height = value;
                UpdateLayout();
            }
        }

        /// <summary>
        /// The color of the divider line.
        /// </summary>
        public Color Color
        {
            set
            {
                _divider.Background.Color = value;
            }
        }

        public void OnAwake()
        {
            RectTransform.LayoutChanged += RectTransform_LayoutChanged;

            _divider = Instantiate<Panel>(this);
            _divider.Background.Color = Color.White.WithAlpha(0.5f);
            _divider.RectTransform.Alignment = Alignment.Center;

            UpdateLayout();
        }

        private void RectTransform_LayoutChanged()
        {
            UpdateLayout();
        }

        private void UpdateLayout()
        {
            _divider.RectTransform.Width = RectTransform.Width;
            _divider.RectTransform.Height = _height;
        }
    }
}
