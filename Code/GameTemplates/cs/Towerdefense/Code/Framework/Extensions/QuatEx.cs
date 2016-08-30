using System;
using CryEngine.Common;

namespace CryEngine.Framework
{
    public static class QuatEx
    {
        public static Quat LookAt(Vec3 from, Vec3 to)
        {
            var forwardVector = (to - from).Normalized;

            var dot = Vec3.Forward.Dot(forwardVector);

            if (Math.Abs(dot - (-1.0f)) < 0.000001f)
                return new Quat(Vec3.Up.x, Vec3.Up.y, Vec3.Up.z, (float)Math.PI);
            
            if (Math.Abs(dot - (1.0f)) < 0.000001f)
                return Quat.CreateIdentity();

            var rotAngle = (float)Math.Acos(dot);
            var rotAxis = Vec3.Forward.Cross(forwardVector);
            rotAxis = rotAxis.Normalized;

            var halfAngle = rotAngle * 0.5f;
            return CreateFromAxisAngle(rotAxis, rotAngle);
        }

        public static Quat CreateFromAxisAngle(Vec3 axis, float angle)
        {
            var halfAngle = angle * .5f;
            var s = (float)Math.Sin(halfAngle);
            var q = Quat.CreateIdentity();
            q.v.x = axis.x * s;
            q.v.y = axis.y * s;
            q.v.z = axis.z * s;
            q.w = (float)Math.Cos(halfAngle);
            return q;
        }
    }
}

