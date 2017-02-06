// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.Sydewinder.Types;
using System;
using System.Collections.Generic;

namespace CryEngine.Sydewinder
{
	[PlayerEntity]
	public class Player : DestroyableBase
	{
		// *** Player Translation Speed ***.
		private float _maxSpeed = 17f;
		private float _acceleration = 54f;

		// Prevent Ship from droping down towards floor.
		private float _forwardSpeed = 0.001f;
		private float _upSpeed = 0.001f;

		// *** Player Rotation Speed ***.
		// Base Player rotation.
		private float _rollAngle = 0f;
		private float _pitchAngle = 0f;
		private const float _yawDeg = (float)Math.PI; // Result of DegreesToRadians(180)
		private float _maxRollAngle = 48f;

		// Number of frames until cooldown is over (0 means no cooldown)
		private int _impactCoolDownFrameCount = 0;
		private int[] _impactCoolDownFrames = new int[9];

		private const int HitDamage = 25;

		// Player Model Skin.
		private static ObjectSkin PlayerSkin = new ObjectSkin("objects/ships/player_ship.cgf");

		// Collection of different weapons.
		private List<WeaponBase> _weapons = new List<WeaponBase>();

		/// <summary>
		/// Initializes a new instance of the <see cref="CryEngine.Sydewinder.Player"/> class.
		/// This is called automatically when a new client connects
		/// </summary>
		/// <param name="position">Position</param>
		public Player()
		{
			// Fires as fast as spacebar is hit.
			_weapons.Add(Gun.Create(new Vector3(0, 25f, 0)));
			ExplosionTrigger = "player_explosion";

			// Use different explosion as the blue explosion in the base class is only used for enemies
			DestroyParticleEffect = Engine.ParticleManager.FindEffect("spaceship.Destruction.explosion");
		}

		/// <summary>
		/// Revives the player
		/// </summary>
		public void Revive(Vector3 pos)
		{
			Revive();

			Entity.NativeHandle.Hide(false);

			Entity.Position = pos;
			Entity.Scale = 10;

			Entity.LoadGeometry(0, PlayerSkin.Geometry);

			Entity.Physics.Physicalize(0, 1, EPhysicalizationType.ePT_Rigid);

			var particleEffect = Engine.ParticleManager.FindEffect("spaceship.Trails.fire_trail");
			Entity.LoadParticleEmitter(1, particleEffect, 0.03f);

			// Get position where to spawn the particle effect.
			Vector3 jetPosition = Entity.GetHelperPos(0, "particle_01");

			// Rotate particle effect to ensure it's shown at the back of the ship.
			Entity.SetGeometrySlotLocalTransform(1, new Matrix3x4(Vector3.One, Quaternion.CreateRotationX(MathHelpers.DegreesToRadians(270f)), jetPosition));

			// Rotate Z-Axis of player ship in degrees.
			Entity.Rotation = Quaternion.CreateRotationZ(MathHelpers.DegreesToRadians(180.0f));

			SydewinderApp.Instance.HUD.SetEnergy(MaxLife);
			//GamePool.AddObjectToPool(this);
		}

		public override void OnCollision(CryEngine.EntitySystem.CollisionEvent collisionEvent)
		{
			var hitEntity = collisionEvent.Source.OwnerEntity;
			if (hitEntity == Entity)
			{
				hitEntity = collisionEvent.Target.OwnerEntity;
			}

			if (hitEntity != null)
			{
				// Handle player collision with lower tunnel by changing the player location.
				if (hitEntity.HasComponent<Tunnel>())
				{
					// As the only colliding tunnel is a lower tunnel, always change player position.
					Vector3 playerPos = Entity.Position;
					Entity.Position = new Vector3(playerPos.x, playerPos.y, 75);
					return;
				}

				var defaultAmmo = hitEntity.GetComponent<DefaultAmmo>();

				// Don't let player collide with own projectiles.
				if (defaultAmmo != null && !defaultAmmo.IsHostile)
					return;

				ProcessHit(hitEntity.HasComponent<Door>());
			}
		}

		public void UpdateRotation()
		{
			// Roll - to Right side.
			_rollAngle = _rollAngle * 0.8f + (_maxRollAngle * (_upSpeed / _maxSpeed)) * 0.2f;
			Ang3 newRotation = new Ang3();
			newRotation.x = MathHelpers.DegreesToRadians(_pitchAngle);
			newRotation.y = MathHelpers.DegreesToRadians(_rollAngle);
			newRotation.z = _yawDeg;
			Entity.Rotation = Quaternion.CreateRotationXYZ(newRotation);
		}

		public void UpdateSpeed()
		{
			float acceleration = FrameTime.Delta * _acceleration;

			// xVal = Controls left/right direction with control values between -1 and 1.
			float xVal = Input.GetInputValue(EKeyId.eKI_XI_ThumbLX);

			// yVal = Controls up/down direction with control values between -1 and 1.
			float yVal = Input.GetInputValue(EKeyId.eKI_XI_ThumbLY);

			if (Input.KeyDown(EKeyId.eKI_Left) || Input.KeyDown(EKeyId.eKI_XI_DPadLeft))
				xVal = -1f;
			if (Input.KeyDown(EKeyId.eKI_Right) || Input.KeyDown(EKeyId.eKI_XI_DPadRight))
				xVal = 1f;
			if (Input.KeyDown(EKeyId.eKI_Down) || Input.KeyDown(EKeyId.eKI_XI_DPadDown))
				yVal = -1f;
			if (Input.KeyDown(EKeyId.eKI_Up) || Input.KeyDown(EKeyId.eKI_XI_DPadUp))
				yVal = 1f;

			// Calculate player ship acceleration
			float xAcceleration = xVal != 0 ? ((xVal < 0 ? xVal * -1 : xVal) * acceleration) : acceleration;
			float yAcceleration = yVal != 0 ? ((yVal < 0 ? yVal * -1 : yVal) * acceleration) : acceleration;

			// Continous moving while key is held down.
			if (xVal < 0)
				_forwardSpeed = Math.Max(-_maxSpeed, _forwardSpeed - xAcceleration);
			else if (_forwardSpeed < 0)
				_forwardSpeed = Math.Min(0.001f, _forwardSpeed + xAcceleration);

			if (xVal > 0)
				_forwardSpeed = Math.Min(_maxSpeed, _forwardSpeed + xAcceleration);
			else if (_forwardSpeed > 0)
				_forwardSpeed = Math.Max(0.001f, _forwardSpeed - xAcceleration);

			if (yVal < 0)
				_upSpeed = Math.Max(-_maxSpeed, _upSpeed - yAcceleration);
			else if (_upSpeed < 0)
				_upSpeed = Math.Min(0.001f, _upSpeed + yAcceleration);

			if (yVal > 0)
				_upSpeed = Math.Min(_maxSpeed, _upSpeed + yAcceleration);
			else if (_upSpeed > 0)
				_upSpeed = Math.Max(0.001f, _upSpeed - yAcceleration);

			// In case there is an eyetracker enabled, we use it.
			var eyePos = Input.GetAxis("EyeTracker");
			if (eyePos != null)
			{
				var currentScreenPos = Camera.ProjectToScreen(Entity.Position);
				_upSpeed = 1 - (eyePos - currentScreenPos).Normalized.y * 20;
			}

			// Usded to ensure the player does not move outside of the visible window.
			// ProjectToScreen returns a Vector3. Each value between 0 and 100 means it is visible on screen in this dimension.
			Vector3 nextPos = Entity.Position + new Vector3(0, _forwardSpeed, _upSpeed) * FrameTime.Delta;
			var screenPosition = Camera.ProjectToScreen(nextPos);

			// In case new position on screen is outside of bounds
			if (screenPosition.x <= 0.05f || screenPosition.x >= 0.95f)
				_forwardSpeed = 0.001f;

			if (screenPosition.y <= 0.15f || screenPosition.y >= 0.75f)
				_upSpeed = 0.001f;

			Speed = new Vector3(0, _forwardSpeed, _upSpeed);
		}

		/// <summary>
		/// Used to work with the cooldown timer to simulate blinking effects
		/// </summary>
		public void CheckCoolDown()
		{
			if (_impactCoolDownFrameCount != 0)
			{
				for (int i = 0; i < _impactCoolDownFrames.Length; i++)
				{
					if (_impactCoolDownFrameCount == _impactCoolDownFrames[i])
					{
						// Simulate blinking effect
						if (i % 2 == 0)
							Entity.FreeGeometrySlot(2);
						else
							Entity.LoadGeometry(2, "objects/ships/Impact.cgf");
					}
				}

				_impactCoolDownFrameCount -= 1;

				// Clean up cooldown
				if (_impactCoolDownFrameCount == 0)
					Entity.FreeGeometrySlot(2);
			}
		}

		/// <summary>
		/// Fire this instance.
		/// Acutally just a dummy implementation to spawn more entities.
		/// </summary>
		public void Fire()
		{
			// Fire all weapons.
			foreach (WeaponBase weapon in _weapons)
			{
				// Spawn projectile in front of Ship to avoid collision with self.
				weapon.Fire(Entity.Position + new Vector3(0.0f, 1.6f, 0.0f));
				SydewinderApp.Instance.MakeScore(1 * SydewinderApp.Instance.HUD.Stage);
			}
		}

		public void ProcessHit(bool lethal = false)
		{
			if (_impactCoolDownFrameCount > 0)
				return;

			DrainLife(HitDamage);
			DestroyParticleEffect.Spawn(Entity.Position);
			if (lethal)
				DrainLife(MaxLife);
			SydewinderApp.Instance.HUD.SetEnergy(Life);

			if (Life == 0)
			{
				Entity.NativeHandle.Hide(true);
				Entity.Position = new Vector3(0, 0, 0);
				Destroy();
			}
			else
			{
				// Based on the previous frametime, calculate how many frames might be needed before 2 more seconds are over
				_impactCoolDownFrameCount = (int)(2f / FrameTime.Delta);

				int framesForEachBlink = _impactCoolDownFrameCount / 10;

				// Set individual frames used to start- & stop blinking effects
				for (int i = 0; i < _impactCoolDownFrames.Length; i++)
					_impactCoolDownFrames[i] = _impactCoolDownFrameCount - (framesForEachBlink * (i-1));

			}
		}
	}
}

