// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;

namespace CryEngine
{
	/// <summary>
	/// Wraps System.Drawing.Color object for easier usage.
	/// </summary>
	public struct Color
	{
		public float R;
		public float G;
		public float B;
		public float A;

		public static Color Red { get { return new Color(1, 0, 0); } }
		public static Color Yellow { get { return new Color(1, 1, 0); } }
		public static Color Green { get { return new Color(0, 1, 0); } }
		public static Color Cyan { get { return new Color(0, 1, 1); } }
		public static Color SkyBlue { get { return new Color(0.25f, 0.5f, 1); } }
		public static Color Blue { get { return new Color(0, 0, 1); } }
		public static Color Pink { get { return new Color(1, 0, 1); } }
		public static Color White { get { return new Color(1, 1, 1); } }
		public static Color LiteGray { get { return new Color(0.75f, 0.75f, 0.75f); } }
		public static Color Gray { get { return new Color(0.5f, 0.5f, 0.5f); } }
		public static Color DarkGray { get { return new Color(0.25f, 0.25f, 0.25f); } }
		public static Color Black { get { return new Color(0, 0, 0); } }
		public static Color Transparent { get { return White.WithAlpha(0); } }

		public Color(float r, float g, float b, float a = 1)
		{
			R = r;
			G = g;
			B = b;
			A = a;
		}

		public static implicit operator ColorB(Color value)
		{
			return new ColorB
			{
				r = (byte)MathHelpers.Clamp(255.0f * value.R, 0.0f, 255.0f),
				g = (byte)MathHelpers.Clamp(255.0f * value.G, 0.0f, 255.0f),
				b = (byte)MathHelpers.Clamp(255.0f * value.B, 0.0f, 255.0f),
				a = (byte)MathHelpers.Clamp(255.0f * value.A, 0.0f, 255.0f),
			};
		}

		public static implicit operator Color(ColorB value)
		{
			if(value == null)
			{
				return new Color();
			}

			return new Color
			{
				R = 255.0f / value.r,
				G = 255.0f / value.g,
				B = 255.0f / value.b,
				A = 255.0f / value.a
			};
		}

		/// <summary>
		/// Implicitly converts the <see cref="Color"/> to a <see cref="ColorF"/>.
		/// </summary>
		/// <returns>The implicit.</returns>
		/// <param name="value">Value.</param>
		public static implicit operator ColorF(Color value)
		{
			return new ColorF(value.R, value.G, value.B, value.A);
		}

		/// <summary>
		/// Implicitly converts the <see cref="ColorF"/> to a <see cref="Color"/>.
		/// </summary>
		/// <returns>The implicit.</returns>
		/// <param name="value">Value.</param>
		public static implicit operator Color(ColorF value)
		{
			if(value == null)
			{
				return new Color();
			}

			return new Color(value.r, value.g, value.b, value.a);
		}

		/// <summary>
		/// Creates a Color from given components.
		/// </summary>
		/// <returns>The RG.</returns>
		/// <param name="r">The red component.</param>
		/// <param name="g">The green component.</param>
		/// <param name="b">The blue component.</param>
		public static Color FromRGB(float r, float g, float b)
		{
			return new Color(r, g, b);
		}

		/// <summary>
		/// Creates new Color from current Color, with alpha applied to it.
		/// </summary>
		/// <returns>The alpha.</returns>
		/// <param name="a">The alpha component.</param>
		public Color WithAlpha(float a)
		{
			return new Color(R, G, B, a);
		}

		/// <summary>
		/// Creates a <see cref="System.Drawing.Color"/> from this Color.
		/// </summary>
		/// <returns>The created color.</returns>
		public System.Drawing.Color ToSystemColor()
		{
			return System.Drawing.Color.FromArgb((int)(A * 255), (int)(R * 255), (int)(G * 255), (int)(B * 255));
		}

		/// <summary>
		/// Returns a <see cref="string"/> that represents the current <see cref="Color"/>.
		/// </summary>
		/// <returns>A <see cref="string"/> that represents the current <see cref="Color"/>.</returns>
		public override string ToString()
		{
			return R.ToString("0.0") + "," + G.ToString("0.0") + "," + B.ToString("0.0");
		}
	}
}
