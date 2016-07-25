using CryEngine.Common;
using CryEngine.Components;
using CryEngine.LevelSuite;
using CryEngine.EntitySystem;

namespace CryEngine.SampleApp
{
    public class ParticlesLevel : LevelHandler
    {
        private FireworkLauncher _fireworkLauncher;

        protected override string Header { get { return "Particles Example"; } }

        protected override void OnStart()
        {
            base.OnStart();

            UI.AddInfo("Press Space to launch a firework.");
            UI.AddSlider("Angle", 0f, 90f, 0f).Slider.ValueChanged += OnAngleValueChanged;

            _fireworkLauncher = GetEntity<FireworkLauncher>();
        }

        private void OnAngleValueChanged(float arg)
        {
            if (_fireworkLauncher != null)
                _fireworkLauncher.MinMaxAngle = arg;
        }
    }
}