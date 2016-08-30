using System;
using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine.SampleApp
{
    class FireworkLauncher : BaseEntity
    {
        public float MinMaxAngle { get; set; } = 0f;

        public override void OnStart()
        {
            base.OnStart();
            Input.OnKey += Input_OnKey;
        }

        public override void OnEnd()
        {
            base.OnEnd();
            Input.OnKey -= Input_OnKey;
        }

        private void Input_OnKey(SInputEvent arg)
        {
            if (arg.KeyPressed(EKeyId.eKI_Space))
                Spawn();
        }

        public void Spawn()
        {
            var firework = EntityFramework.Spawn<Firework>();
            firework.Position = Position;

            //// Rotate so forward is facing up.
            firework.Rotation = Quat.CreateRotationAA(1.5f, Vec3.Right);

            //// Rotate to a random direction.
            var radians = Utils.Deg2Rad(MinMaxAngle);
            var rndAngle = new Random().Next((int)(-radians * 10), (int)(radians * 10)) / 10f;
            firework.Rotation *= Quat.CreateRotationAA(rndAngle, Vec3.Up);

            firework.Launch();
        }
    }
}