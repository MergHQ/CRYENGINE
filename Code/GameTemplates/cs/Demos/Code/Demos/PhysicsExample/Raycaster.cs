using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine.SampleApp
{
    [EntityClass("Raycaster", "Utilities", null, "TagPoint.bmp")]
    public class Raycaster : BaseEntity
    {
        public RaycastOutput RayOut { get; private set; }

        [EntityProperty("How far the ray should be cast", EEditTypes.Float, 0.1f, float.PositiveInfinity, "1")]
        public float Distance { get; set; }

        public override void OnUpdate(float frameTime, PauseMode pauseMode)
        {
            base.OnUpdate(frameTime, pauseMode);

            // Raycast from the position of this object down using the specified distance.
            RayOut = RaycastHelper.Raycast(Position, Vec3.Down, Distance);

            // Set color based on hit type.
            // Unknown = Red
            // Terrain = Green
            // Entity = Blue
            // Other (Designer/Brush) = White
            ColorB color = null;
            switch (RayOut.Type)
            {
                case RaycastOutput.HitType.Unknown:
                    color = new ColorB(255, 0, 0);
                    break;
                case RaycastOutput.HitType.Terrain:
                    color = new ColorB(0, 255, 0);
                    break;
                case RaycastOutput.HitType.Entity:
                    color = new ColorB(0, 0, 255);
                    break;
                case RaycastOutput.HitType.Static:
                    color = new ColorB(255, 255, 255);
                    break;
                default:
                    break;
            }

            // Draw debug helpers.
            var renderer = Env.AuxRenderer;
            renderer.DrawSphere(Position, 0.25f, color, false);
            renderer.DrawLine(Position, color, RayOut.Point, color, 4f);

            // Draw additional debug helpers if the ray hit something.
            if (RayOut.Intersected)
                renderer.DrawCylinder(RayOut.Point, Vec3.Up, 0.5f, 0.1f, color);

            // Draw information about what was hit. Displays 'Hit: Type' if not hitting an entity, otherwise 'Hit: Type (EntityName)'.
            var info = string.Format("Hit: {0} ({1})", RayOut.Type.ToString(), RayOut.HitBaseEntity != null ? RayOut.HitBaseEntity.Name : "");
            Env.AuxRenderer.Draw2dLabel(Screen.Width / 2f, Screen.Height - 55, 2, new ColorF(), true, info);

            var distance = string.Format("Distance: {0}", RayOut.Distance);
            Env.AuxRenderer.Draw2dLabel(Screen.Width / 2f, Screen.Height - 35, 2, new ColorF(), true, distance);
        }
    }
}