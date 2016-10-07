using CryEngine.Common;

using System;
using System.Globalization;

namespace CryEngine
{
	public struct Vector2
	{
		public static readonly Vector2 Zero = new Vector2(0, 0);

		public static readonly Vector2 Up = new Vector2(0, 1);
		public static readonly Vector2 Down = new Vector2(0, -1);
		public static readonly Vector2 Left = new Vector2(-1, 0);
		public static readonly Vector2 Right = new Vector2(1, 0);

		private float _x;
		private float _y;

		public float x { get { return _x; } set { _x = value; } }
		public float y { get { return _y; } set { _y = value; } }
		public float X { get { return _x; } set { _x = value; } }
		public float Y { get { return _y; } set { _y = value; } }

		public Vector2(float xCoord, float yCoord)
		{
			_x = xCoord;
			_y = yCoord;
		}

		#region Overrides
		public override int GetHashCode()
		{
			unchecked // Overflow is fine, just wrap
			{
				int hash = 17;

				hash = hash * 23 + X.GetHashCode();
				hash = hash * 23 + Y.GetHashCode();

				return hash;
			}
		}

		public override bool Equals(object obj)
		{
			if (obj == null)
				return false;

			if (obj is Vector2 || obj is Vec2)
				return this == (Vector2)obj;

			return false;
		}

		public override string ToString()
		{
			return string.Format(CultureInfo.CurrentCulture, "{0},{1}", X, Y);
		}
		#endregion

		#region Conversions
		public static implicit operator Vec2(Vector2 managedVector)
		{
			return new Vec2(managedVector.X, managedVector.Y);
		}

		public static implicit operator Vector2(Vec2 nativeVector)
		{
			if(nativeVector == null)
			{
				return Vector2.Zero;
			}

			return new Vector2(nativeVector.x, nativeVector.y);
		}
		#endregion

		#region Operators
		public static Vector2 operator *(Vector2 v, float scale)
		{
			return new Vector2(v.X * scale, v.Y * scale);
		}

		public static Vector2 operator *(float scale, Vector2 v)
		{
			return v * scale;
		}

		public static Vector2 operator /(Vector2 v, float scale)
		{
			scale = 1.0f / scale;

			return new Vector2(v.X * scale, v.Y * scale);
		}

		public static Vector2 operator +(Vector2 v0, Vector2 v1)
		{
			return new Vector2(v0.X + v1.X, v0.Y + v1.Y);
		}

		public static Vector2 operator -(Vector2 v0, Vector2 v1)
		{
			return new Vector2(v0.X - v1.X, v0.Y - v1.Y);
		}
		
		public static Vector2 operator -(Vector2 v)
		{
			return v.Flipped;
		}

		public static Vector2 operator !(Vector2 v)
		{
			return v.Flipped;
		}

		public static bool operator ==(Vector2 left, Vector2 right)
		{
			if ((object)right == null)
				return (object)left == null;

			return (left.X == right.X) && (left.Y == right.Y);
		}

		public static bool operator !=(Vector2 left, Vector2 right)
		{
			return !(left == right);
		}
		#endregion

		#region Functions
		public float Dot(Vector2 v)
		{
			return X * v.X + Y * v.Y;
		}
		#endregion

		#region Properties
		public float Length
		{
			get
			{
				return (float)Math.Sqrt(X * X + Y * Y);
			}
			set
			{
				float lengthSquared = LengthSquared;
				if (lengthSquared < 0.00001f * 0.00001f)
					return;

				lengthSquared = value * MathHelpers.ISqrt(lengthSquared);
				X *= lengthSquared;
				Y *= lengthSquared;
			}
		}

		public float Magnitude { get { return Length; } }

		public float LengthSquared
		{
			get
			{
				return X * X + Y * Y;
			}
		}
		
		public Vector2 Normalized
		{
			get
			{
				float fInvLen = MathHelpers.ISqrt(X * X + Y * Y);
				return this * fInvLen;
			}
		}
		
		public Vector2 Absolute
		{
			get
			{
				return new Vector2(Math.Abs(X), Math.Abs(Y));
			}
		}

		public Vector2 Flipped
		{
			get
			{
				return new Vector2(-X, -Y);
			}
		}

		/// <summary>
		/// Gets individual axes by index
		/// </summary>
		/// <param name="index">Index, 0 - 1 where 0 is X and 1 is Y</param>
		/// <returns>The axis value</returns>
		public float this[int index]
		{
			get
			{
				switch (index)
				{
					case 0:
						return x;
					case 1:
						return y;

					default:
						throw new ArgumentOutOfRangeException("index", "Indices must run from 0 to 1!");
				}
			}
			set
			{
				switch (index)
				{
					case 0:
						x = value;
						break;
					case 1:
						y = value;
						break;

					default:
						throw new ArgumentOutOfRangeException("index", "Indices must run from 0 to 1!");
				}
			}
		}
		#endregion

	}
}
