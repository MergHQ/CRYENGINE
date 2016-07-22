using CryEngine.Common;
using CryEngine.UI;
using CryEngine.EntitySystem;
using CryEngine.LevelSuite;

namespace CryEngine.SampleApp
{
    public class RaycastLevel : LevelHandler
    {
        private Raycaster _raycaster;
        private float _baseMoveSpeed;
        private float _moveSpeedShiftModifier;

        protected override string Header { get { return "Raycast Example"; } }

        private float MoveSpeed { get { return Input.ShiftDown ? _baseMoveSpeed * _moveSpeedShiftModifier : _baseMoveSpeed; } }

        protected override void OnStart()
        {
            _baseMoveSpeed = 7.5f;
            _moveSpeedShiftModifier = 0.25f;

            UI.AddInfo("Press A or D to move the beam left or right. Press Space to spawn a ball.").Text.Color = Color.White;
            UI.AddSlider("Distance", 0.1f, 20f, 20f).Slider.ValueChanged += Distance_Slider_ValueChanged;

            _raycaster = EntityFramework.GetEntity<Raycaster>();
            if (_raycaster == null)
                Log.Error<RaycastLevel>("Failed to find a Raycaster entity in the current level.");
        }

        private void Distance_Slider_ValueChanged(float arg)
        {
            if (_raycaster != null)
                _raycaster.Distance = arg;
        }

        public override void OnInputKey(SInputEvent arg)
        {
            if (_raycaster == null)
                return;

            if (arg.keyId == EKeyId.eKI_A)
                _raycaster.Position += Vec3.Left * MoveSpeed * FrameTime.Delta;

            if (arg.keyId == EKeyId.eKI_D)
                _raycaster.Position += Vec3.Right * MoveSpeed * FrameTime.Delta;

            if (arg.keyId == EKeyId.eKI_Space && arg.state == EInputState.eIS_Pressed)
            {
                if (_raycaster.RayOut != null && _raycaster.RayOut.Intersected)
                {
                    var ball = EntityFramework.Spawn<Ball>();
                    ball.Scale = Vec3.One * 2f;
                    ball.Position = _raycaster.RayOut.Point + Vec3.Up * ball.Scale.y / 2f;
                }
            }
        }
    }
}
