// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;

namespace CryEngine.Sydewinder
{
	static class FrameTime
	{
		private static float _frameTime;

		/// <summary>
		/// Gets the last calculated frametime in seconds.
		/// </summary>
		/// <returns>The last.</returns>
		public static float Current { get { return _frameTime; } }

		/// <summary>
		/// Retrieve the last frametime from engine.
		/// </summary>
		public static void Update()
		{			
			_frameTime = Env.Timer.GetFrameTime();
		}

		/// <summary>
		/// Normalizes the speed.
		/// </summary>
		/// <returns>Returning argument multiplicated with frame time</returns>
		/// <param name="absoluteValue">Absolute value.</param>
		public static float Normalize(float value)
		{
			return value * _frameTime;
		}

		public static Vec3 Normalize(Vec3 value)
		{
			return value * _frameTime;
		}
	}
}
