using System;
using CryEngine.Common;

namespace CryEngine.RollingBall
{
	public class PlayerView
	{
		private readonly Player _player;

		public PlayerView(Player player)
		{
			if(player == null)
			{
				throw new ArgumentNullException(nameof(player));
			}
			_player = player;
		}

		public void UpdateView(float frameTime)
		{
			Quaternion rotation = Camera.Rotation;
			float yawSpeed = _player.RotationSpeedYaw;
			float pitchSpeed = _player.RotationSpeedPitch;
			float pitchMin = _player.RotationLimitsMinPitch;
			float pitchMax = _player.RotationLimitsMaxPitch;
			Vector2 mouseDelta = new Vector2(Input.GetInputValue(EKeyId.eKI_MouseX), Input.GetInputValue(EKeyId.eKI_MouseY));

			//Invert the mouseDelte to have proper third-person camera-control.
			mouseDelta = -mouseDelta;

			var ypr = Camera.CreateAnglesYPR(new Matrix3x4(rotation));

			ypr.X += mouseDelta.x * yawSpeed * frameTime;

			float pitchDelta = mouseDelta.Y * pitchSpeed * frameTime;
			ypr.Y = MathHelpers.Clamp(ypr.Y + pitchDelta, pitchMin, pitchMax);

			ypr.Z = 0;

			var matrix = Camera.CreateOrientationYPR(ypr);
			rotation = new Quaternion(matrix);

			var position = _player.Entity.Position;
			var forward = rotation.Forward;
			position -= forward * _player.ViewDistance;
			Camera.Position = position;
			Camera.Rotation = rotation;
		}
	}
}

