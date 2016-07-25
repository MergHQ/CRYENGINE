using System;

namespace CryEngine.Framework
{
    public static class MathEx
    {
        /// <summary>
        /// Clamps the value between min and max. Works with int, float, and any other type that implements IComparable.
        /// </summary>
        /// <param name="value">Value to be clamped</param>
        /// <param name="min">Minimum value</param>
        /// <param name="max">Maximum value</param>
        public static T Clamp<T>(T value, T min, T max) where T : IComparable
        {
            if (value.CompareTo(min) < 0)
                return min;
            if (value.CompareTo(max) > 0)
                return max;
            return value;
        }
    }
}

