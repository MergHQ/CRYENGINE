using System;

namespace CryEngine
{
	public static class MathHelpers
	{
		/// <summary>
		/// Convert degrees to radians.
		/// </summary>
		/// <returns>Angle in radians.</returns>
		/// <param name="degrees">Angle in degrees.</param>
		public static float DegreesToRadians(float degrees)
		{
			return degrees * ((float)Math.PI / 180f);
		}

		/// <summary>
		/// Convert Radians to degrees.
		/// </summary>
		/// <returns>Angle in degrees.</returns>
		/// <param name="rad">Angle in radians.</param>
		public static float RadiansToDegrees(float rad)
		{
			return rad * (180f / (float)Math.PI);
		}

		/// <summary>
		/// Determines whether a value is inside the specified range.
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="value"></param>
		/// <param name="min"></param>
		/// <param name="max"></param>
		/// <returns></returns>
		public static bool IsInRange<T>(T value, T min, T max) where T : IComparable<T>
		{
			if (value.CompareTo(min) >= 0 && value.CompareTo(max) <= 0)
				return true;

			return false;
		}

		/// <summary>
		/// Clamps a value given a specified range.
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="value"></param>
		/// <param name="min"></param>
		/// <param name="max"></param>
		/// <returns></returns>
		public static T Clamp<T>(T value, T min, T max) where T : IComparable<T>
		{
			if (value.CompareTo(min) < 0)
				return min;
			if (value.CompareTo(max) > 0)
				return max;

			return value;
		}

		public static float ClampAngleDegrees(float angle, float min, float max)
		{
			if (angle < -360)
				angle += 360;
			if (angle > 360)
				angle -= 360;

			return Clamp(angle, min, max);
		}

		public static T Max<T>(T val1, T val2) where T : IComparable<T>
		{
			if (val1.CompareTo(val2) > 0)
				return val1;

			return val2;
		}

		public static T Min<T>(T val1, T val2) where T : IComparable<T>
		{
			if (val1.CompareTo(val2) < 0)
				return val1;

			return val2;
		}

		public static bool IsPowerOfTwo(int value)
		{
			return (value & (value - 1)) == 0;
		}

		public static float ISqrt(float d)
		{
			return 1f / (float)Math.Sqrt(d);
		}

		public static void SinCos(float angle, out float sin, out float cos)
		{
			sin = (float)Math.Sin(angle);
			cos = (float)Math.Cos(angle);
		}

		public static float Square(float value)
		{
			return value * value;
		}
	}
}
