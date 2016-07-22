using System;
using System.Linq;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Framework;

namespace CryEngine.SampleApp
{
    public class Test : SceneObject
    {
        List<Ball> balls = new List<Ball>();
        TestUI ui;

        public PlayerMouse PlayerMouse { get; private set; }

        public void OnAwake()
        {
            Input.OnKey += OnKey;

            // Load initial map
            Env.Console.ExecuteString("map test");

            // Player mouse interaction
            PlayerMouse = AddComponent<PlayerMouse>();

            // Create UI
            ui = new TestUI(Root);

            // Create kill plane
            GameObject.Instantiate<KillPlane>(Root);
        }

        public void OnUpdate()
        {
            ui.Update(this);
        }

        public void OnDestroy()
        {
            Input.OnKey -= OnKey;
        }

        void OnKey(SInputEvent e)
        {
            if (e.KeyPressed(EKeyId.eKI_E))
                CreateBall();

            if (e.KeyPressed(EKeyId.eKI_V))
                AddImpulse();

            if (e.KeyPressed(EKeyId.eKI_T))
            {
                balls.ToList().ForEach(x => x.Destroy());
                balls.Clear();
            }
        }

        void AddImpulse()
        {
            balls.ForEach(x =>
            {
                var physImpulse = new pe_action_impulse()
                {
                    impulse = Vec3.Up * 100f,
                    point = Vec3.Zero,
                };
                x.Physics.Action(physImpulse);
            });
        }

        void CreateBall()
        {
            var ball = GameObject.Instantiate<Ball>();
            balls.Add(ball);
            ball.OnDestroy += () => balls.Remove(ball);

            var rnd = new Random();

            ball.Position = new Vec3(0, 0, rnd.Next(8, 12));
            ball.Scale = new Vec3(Vec3.One * (float)(rnd.Next(5, 7) / 10f));
            ball.Rotation = new Quat(new Ang3(rnd.Next(1, 10), rnd.Next(1, 10), rnd.Next(1, 10)));

            var physParams = new SEntityPhysicalizeParams()
            {
                mass = 10,
                density = -1,
                type = (int)EPhysicalizationType.ePT_Rigid,
            };

            ball.Entity.BaseEntity.Physicalize(physParams);

            var physImpulse = new pe_action_set_velocity()
            {
                v = new Vec3(rnd.Next(-5, 5), rnd.Next(-5, 5), rnd.Next(1, 5)),
            };
            ball.Physics.Action(physImpulse);
        }
    }
}
