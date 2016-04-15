// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;

namespace CryEngine.Sydewinder
{
	public static class Utils
	{
		// This is a constant factor which is not changing.
		private const float _deg2RadMultiplicator = (float)Math.PI / 180f;

		public static float Deg2Rad(float degrees)
		{
			return degrees * _deg2RadMultiplicator; 
		}

		public static float Rad2Deg(float rad)
		{
			return rad / _deg2RadMultiplicator;
		}
	}
}

