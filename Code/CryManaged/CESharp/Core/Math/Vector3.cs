using CryEngine.Common;

using System;
using System.Globalization;

namespace CryEngine
{
	public struct Vector3
	{
		public static readonly Vector3 Zero = new Vector3(0, 0, 0);
		public static readonly Vector3 One = new Vector3(1, 1, 1);

		public static readonly Vector3 Up = new Vector3(0, 0, 1);
		public static readonly Vector3 Down = new Vector3(0, 0, -1);
		public static readonly Vector3 Forward = new Vector3(0, 1, 0);
		public static readonly Vector3 Back = new Vector3(0, 1, 0);
		public static readonly Vector3 Left = new Vector3(-1, 0, 0);
		public static readonly Vector3 Right = new Vector3(1, 0, 0);

		private float _x;
		private float _y;
		private float _z;

		public float x { get { return _x; } set { _x = value; } }
		public float y { get { return _y; } set { _y = value; } }
		public float z { get { return _z; } set { _z = value; } }

		public float X { get { return _x; } set { _x = value; } }
		public float Y { get { return _y; } set { _y = value; } }
		public float Z { get { return _z; } set { _z = value; } }

		public Vector3(float xCoord, float yCoord, float zCoord)
		{
			_x = xCoord;
			_y = yCoord;
			_z = zCoord;
		}

		#region Overrides
		public override int GetHashCode()
		{
			unchecked // Overflow is fine, just wrap
			{
				int hash = 17;

				hash = hash * 23 + X.GetHashCode();
				hash = hash * 23 + Y.GetHashCode();
				hash = hash * 23 + Z.GetHashCode();

				return hash;
			}
		}

		public override bool Equals(object obj)
		{
			if (obj == null)
				return false;

			if (obj is Vector3 || obj is Vec3)
				return this == (Vector3)obj;

			return false;
		}

		public override string ToString()
		{
			return string.Format(CultureInfo.CurrentCulture, "{0},{1},{2}", X, Y, Z);
		}
		#endregion

		#region Conversions
		public static implicit operator Vec3(Vector3 managedVector)
		{
			return new Vec3(managedVector.X, managedVector.Y, managedVector.Z);
		}

		public static implicit operator Vector3(Vec3 nativeVector)
		{
			if(nativeVector == null)
			{
				return Vector3.Zero;
			}

			return new Vector3(nativeVector.x, nativeVector.y, nativeVector.z);
		}
		#endregion

		#region Operators
		public static Vector3 operator *(Vector3 v, float scale)
		{
			return new Vector3(v.X * scale, v.Y * scale, v.Z * scale);
		}

		public static Vector3 operator *(float scale, Vector3 v)
		{
			return v * scale;
		}

		public static Vector3 operator /(Vector3 v, float scale)
		{
			scale = 1.0f / scale;

			return new Vector3(v.X * scale, v.Y * scale, v.Z * scale);
		}

		public static Vector3 operator +(Vector3 v0, Vector3 v1)
		{
			return new Vector3(v0.X + v1.X, v0.Y + v1.Y, v0.Z + v1.Z);
		}
		
		public static Vector3 operator -(Vector3 v0, Vector3 v1)
		{
			return new Vector3(v0.X - v1.X, v0.Y - v1.Y, v0.Z - v1.Z);
		}
		
		public static Vector3 operator -(Vector3 v)
		{
			return v.Flipped;
		}

		public static Vector3 operator !(Vector3 v)
		{
			return v.Flipped;
		}

		public static bool operator ==(Vector3 left, Vector3 right)
		{
			if ((object)right == null)
				return (object)left == null;

			return ((left.X == right.X) && (left.Y == right.Y) && (left.Z == right.Z));
		}

		public static bool operator !=(Vector3 left, Vector3 right)
		{
			return !(left == right);
		}
		#endregion

		#region Functions
		public float Dot(Vector3 v)
		{
			return X * v.X + Y * v.Y + Z * v.Z;
		}

		public Vector3 Cross(Vector3 v)
		{
			return new Vector3(Y * v.Z - Z * v.Y, Z * v.X - X * v.Z, X * v.Y - Y * v.X);
		}
		#endregion

		#region Properties
		public float Length
		{
			get
			{
				return (float)Math.Sqrt(X * X + Y * Y + Z * Z);
			}
			set
			{
				float lengthSquared = LengthSquared;
				if (lengthSquared < 0.00001f * 0.00001f)
					return;

				lengthSquared = value * MathHelpers.ISqrt(lengthSquared);
				X *= lengthSquared;
				Y *= lengthSquared;
				Z *= lengthSquared;
			}
		}

		public float Magnitude {  get { return Length; } }

		public float LengthSquared
		{
			get
			{
				return X * X + Y * Y + Z * Z;
			}
		}

		public float Length2D
		{
			get
			{
				return (float)Math.Sqrt(X * X + Y * Y);
			}
		}

		public float Magnitude2D {  get { return Length2D; } }

		public float Length2DSquared
		{
			get
			{
				return X * X + Y * Y;
			}
		}

		public Vector3 Normalized
		{
			get
			{
				float fInvLen = MathHelpers.ISqrt(X * X + Y * Y + Z * Z);
				return this * fInvLen;
			}
		}

		public float Volume
		{
			get
			{
				return X * Y * Z;
			}
		}

		public Vector3 Absolute
		{
			get
			{
				return new Vector3(Math.Abs(X), Math.Abs(Y), Math.Abs(Z));
			}
		}

		public Vector3 Flipped
		{
			get
			{
				return new Vector3(-X, -Y, -Z);
			}
		}

		/// <summary>
		/// Returns a vector that is orthogonal to this one
		/// </summary>
		public Vector3 Orthogonal
		{
			get
			{
				return MathHelpers.Square(0.9f) * (x * x + y * y + z * z) - x * x < 0 ? new Vector3(-z, 0, x) : new Vector3(0, z, -y);
			}
		}

		/// <summary>
		/// Gets individual axes by index
		/// </summary>
		/// <param name="index">Index, 0 - 2 where 0 is X and 2 is Z</param>
		/// <returns>The axis value</returns>
		public float this[int index]
		{
			get
			{
				switch(index)
				{
					case 0:
						return x;
					case 1:
						return y;
					case 2:
						return z;

					default:
						throw new ArgumentOutOfRangeException("index", "Indices must run from 0 to 2!");
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
					case 2:
						z = value;
						break;

					default:
						throw new ArgumentOutOfRangeException("index", "Indices must run from 0 to 2!");
				}
			}
		}
		#endregion

	}
}
