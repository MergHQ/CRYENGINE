// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.Rendering;

namespace CryEngine
{
	/// <summary>
	/// Static class exposing access to the current view camera
	/// </summary>
	public static class Camera
	{
		public static Vector3 Position
		{
			set
			{
				Engine.System.GetViewCamera().SetPosition(value);
			}
			get
			{
				return Engine.System.GetViewCamera().GetPosition();
			}
		} ///< Sets position of assigned HostEntity.

		public static Vector3 ForwardDirection
		{
			set
			{
				var camera = Engine.System.GetViewCamera();
				var newRotation = new Quaternion(value);

				camera.SetMatrix(new Matrix3x4(Vector3.One, newRotation, camera.GetPosition()));
			}
			get
			{
				return Engine.System.GetViewCamera().GetMatrix().GetColumn1();
			}
		} ///< Sets rotation of assigned HostEntity

		public static Matrix3x4 Transform
		{
			set
			{
				Engine.System.GetViewCamera().SetMatrix(value);
			}
			get
			{
				return Engine.System.GetViewCamera().GetMatrix();
			}
		}

		public static Quaternion Rotation
		{
			set
			{
				var camera = Engine.System.GetViewCamera();

				camera.SetMatrix(new Matrix3x4(Vector3.One, value, camera.GetPosition()));
			}
			get
			{
				return new Quaternion(Engine.System.GetViewCamera().GetMatrix());
			}
		}

		/// <summary>
		/// Gets or sets the field of view by CVar.
		/// </summary>
		/// <value>The field of view.</value>
		public static float FieldOfView
		{
			get
			{
				return MathHelpers.RadiansToDegrees(Engine.System.GetViewCamera().GetFov());
			}
			set
			{
				var camera = Engine.System.GetViewCamera();

				camera.SetFrustum(Global.gEnv.pRenderer.GetWidth(), Global.gEnv.pRenderer.GetHeight(), MathHelpers.DegreesToRadians(value));
			}
		}
		
		public static Vector3 Unproject(int x, int y)
		{
			return Global.gEnv.pRenderer.UnprojectFromScreen(x, Renderer.ScreenHeight - y);
		}

		public static Vector2 ProjectToScreen(Vector3 position)
		{
			return Global.gEnv.pRenderer.ProjectToScreen(position);
		}
	}
}
