using System;
using CryEngine.Common;

namespace CryEngine.Framework
{
    public static class Vec3Ex
    {
        public static Vec3 GetRandomVec(int min, int max)
        {
            var rnd = new Random();

            float x = rnd.Next(min, max);
            float y = rnd.Next(min, max);
            float z = rnd.Next(min, max);

            return new Vec3(x, y, z);
        }

        public static bool VisibleOnScreen(this Vec3 vec)
        {
            var screenPoint = Env.Renderer.ProjectToScreen(vec);
            return screenPoint.x > 0 && screenPoint.x < 1 && screenPoint.y > 0 && screenPoint.y < 1;
        }

        public static string AsString(this Vec3 vec)
        {
            return string.Join(",", vec.x, vec.y, vec.z);
        }

        public static Vec3 FromString(string input)
        {
            var components = input.Split(',');

            if (components.Length == 3)
            {
                var x = float.Parse(components[0]);
                var y = float.Parse(components[1]);
                var z = float.Parse(components[2]);

                return new Vec3(x, y, z);
            }
            else
            {
                return new Vec3();
            }
        }
    }
}
