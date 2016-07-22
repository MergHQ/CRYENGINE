using CryEngine.UI;

namespace CryEngine.LevelSuite.UI
{
    public class Slider : ScrollBar
    {
        public event EventHandler<float> ValueChanged;
        public event EventHandler<float> MinValueChanged;
        public event EventHandler<float> MaxValueChanged;

        private float _value;
        public float Value
        {
            set
            {
                _value = value;
                MoveScrollAreaPos(_value / _maxValue);
            }
        }

        private float _minValue;
        public float MinValue
        {
            set
            {
                _minValue = value;
                MinValueChanged?.Invoke(value);
            }
        }

        private float _maxValue;
        public float MaxValue
        {
            set
            {
                _maxValue = value;
                MaxValueChanged?.Invoke(value);
            }
        }

        public override void OnAwake()
        {
            base.OnAwake();
            IsVertical = false;
            Scrolling += Slider_Scrolling;
        }

        private void Slider_Scrolling(float arg)
        {
            _value = _maxValue * arg;
            ValueChanged?.Invoke(_value);
        }

        public void OnDestroy()
        {
            Scrolling -= Slider_Scrolling;
        }
    }
}
