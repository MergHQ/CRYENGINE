using System;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Framework;


namespace CryEngine.Towerdefense
{
    public class SineWaveMovement : GameComponent
    {
        Vec3 position;

        public float Period { get; set; } = 0.1f;
        public float Amplitude { get; set; } = 0.01f;

        protected override void OnInitialize()
        {
            position = GameObject.Entity.Position;
        }

        public override void OnUpdate()
        {
            var theta = Global.gEnv.pTimer.GetCurrTime() / Period;
            var distance = Amplitude * (float)Math.Sin(theta);
            GameObject.Position = position + (Vec3.Up * distance);
        }
    }
}