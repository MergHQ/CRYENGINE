using CryEngine.Common;
using CryEngine.UI;
using CryEngine.EntitySystem;
using CryEngine.LevelSuite;

namespace CryEngine.SampleApp
{
    public class EarthquakeLevel : LevelHandler
    {
        private Earthquake _earthquake;

        protected override string Header { get { return "Earthquake Example"; } }
        protected override bool ShowCursor { get { return true; } }

        protected override void OnStart()
        {
            UI.AddSlider("Amplitude", 0.1f, 100f, 2f).Slider.ValueChanged += MinAmplitude_Slider_ValueChanged;
            UI.AddSlider("Max Amplitude", 0.2f, 100f, 4f).Slider.ValueChanged += MaxAmplitude_Slider_ValueChanged;
            UI.AddSlider("Radius", 0.1f, 10f, 2f).Slider.ValueChanged += Radius_Slider_ValueChanged;
            UI.AddToggle("Visualize", true).ValueChanged += Visualize_ValueChanged;

            _earthquake = EntityFramework.GetEntity<Earthquake>();
            if (_earthquake == null)
            {
                Log.Error<EarthquakeLevel>("Failed to find a Earthquake entity in the current level.");
            }
            else
            {
                _earthquake.Visualize = true;
            }
        }

        private void Visualize_ValueChanged(bool arg)
        {
            _earthquake.Visualize = arg;
        }

        private void Radius_Slider_ValueChanged(float arg)
        {
            _earthquake.Radius = arg;
        }

        private void MinAmplitude_Slider_ValueChanged(float arg)
        {
            _earthquake.MinAmplitude = arg;
        }

        private void MaxAmplitude_Slider_ValueChanged(float arg)
        {
            _earthquake.MaxAmplitude = arg;
        }

        public override void OnInputKey(SInputEvent arg)
        {
            if (_earthquake == null)
                return;

            if (arg.KeyDown(EKeyId.eKI_W))
                _earthquake.Position += Vec3.Right * 4f * FrameTime.Delta;

            if (arg.KeyDown(EKeyId.eKI_S))
                _earthquake.Position += -Vec3.Right * 4f * FrameTime.Delta;

            if (arg.KeyDown(EKeyId.eKI_A))
            {
                if (_earthquake.MinAmplitude > 0.5f)
                {
                    _earthquake.MinAmplitude -= 0.5f;
                    _earthquake.MaxAmplitude -= 0.5f;
                }
            }

            if (arg.KeyDown(EKeyId.eKI_D))
            {
                _earthquake.MinAmplitude += 0.5f;
                _earthquake.MaxAmplitude += 0.5f;
            }

            if (arg.KeyDown(EKeyId.eKI_Q))
            {
                _earthquake.Radius -= 0.1f;
                _earthquake.Radius = MathExtensions.Clamp(_earthquake.Radius, 0.25f, _earthquake.Radius);
            }

            if (arg.KeyDown(EKeyId.eKI_E))
                _earthquake.Radius += 0.1f;

            if (arg.KeyPressed(EKeyId.eKI_R))
                _earthquake.RandomOffset += 1f;

            if (arg.KeyPressed(EKeyId.eKI_F))
            {
                _earthquake.RandomOffset -= 1f;
                _earthquake.RandomOffset = MathExtensions.Clamp(_earthquake.RandomOffset, 0f, _earthquake.RandomOffset);
            }

            if (arg.KeyPressed(EKeyId.eKI_V))
                _earthquake.Visualize = !_earthquake.Visualize;
        }
    }
}
