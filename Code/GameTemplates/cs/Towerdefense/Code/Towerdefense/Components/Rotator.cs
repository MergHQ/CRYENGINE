using System;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Framework;


namespace CryEngine.Towerdefense
{
    public class Rotator : GameComponent
    {
        public float Speed = 1f;

        public override void OnUpdate()
        {
            GameObject.Rotation *= Quat.CreateRotationZ(Speed * FrameTime.Delta);
        }
    }
}