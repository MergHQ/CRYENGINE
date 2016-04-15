// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;
using CryEngine.Sydewinder.Types;
using CryEngine.Sydewinder.UI;
using System.Collections.Generic;
using CryEngine.Sydewinder.Framework;
using CryEngine.Components;

namespace CryEngine.Sydewinder
{	
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
		private const float _yawDeg = (float)Math.PI; // Result of Deg2Rad(180)
		private float _maxRollAngle = 48f;

		// Number of frames until cooldown is over (0 means no cooldown)
		private int _impactCoolDownFrameCount = 0;
		private int[] _impactCoolDownFrames = new int[9];

		private const int HitDamage = 25;

		// Player Model Skin.
		private ObjectSkin _playerSkin = new ObjectSkin("objects/ships/player_ship.cgf");

		// Collection of different weapons.
		private List<WeaponBase> _weapons = new List<WeaponBase>();

		/// <summary>
		/// Initializes a new instance of the <see cref="CryEngine.Sydewinder.Player"/> class.
		/// </summary>
		/// <param name="position">Position</param>
		public Player()
		{
			// Fires as fast as spacebar is hit.
			_weapons.Add(new Gun(new Vec3(0, 25f, 0)));

			string explosionSoundPath = System.IO.Path.Combine(Application.DataPath, "sounds/player_explosion.wav");
			ExplosionSound = new MultiSoundPlayer(explosionSoundPath);

			// Use different explosion as the blue explosion in the base class is only used for enemies
			DestroyParticleEffect = Env.ParticleManager.FindEffect("spaceship.Destruction.explosion");
		}

		/// <summary>
		/// Spawn this instance.
		/// </summary>
		public Vec3 Spawn(Vec3 position)
		{
			if (Entity != null && Entity.Exists)
				throw new Exception("Player already spawned");
			
			Entity = EntitySystem.Entity.Instantiate(position, Quat.Identity, 10, _playerSkin.Geometry);
			var particleEffect = Env.ParticleManager.FindEffect("spaceship.Trails.fire_trail");
			Entity.LoadParticleEmitter(1, particleEffect, 0.03f);

			// Get position where to spawn the particle effect.
			Vec3 jetPosition = Entity.GetHelperPos(0, "particle_01");

			// Rotate particle effect to ensure it's shown at the back of the ship.
			Entity.SetTM(1, Matrix34.Create(Vec3.One, Quat.CreateRotationX(Utils.Deg2Rad(270f)), jetPosition));

			// Rotate Z-Axis of player ship in degrees.
			Entity.Rotation = new Quat(Matrix34.CreateRotationZ(Utils.Deg2Rad(180.0f)));

			// Attach to entity collision event.
			EntityCollisionCheck.SubscribeToEntityCollision(Entity.ID);

			GamePool.AddObjectToPool(this);
			return Entity.Position;
		}

		public void UpdateRotation()
		{
			if (Entity == null)
				return;

			// Rotation direction for our Player Model.
			// Y = Roll
			// X = Pitch
			// Z = Yaw

			// Roll - to Right side.
			_rollAngle = _rollAngle * 0.8f + (_maxRollAngle * (_upSpeed / _maxSpeed)) * 0.2f;

			Entity.Rotation = Quat.CreateRotationXYZ(new Ang3(Utils.Deg2Rad(_pitchAngle), Utils.Deg2Rad(_rollAngle), _yawDeg));
		}

		public void UpdateSpeed()
		{			
			float acceleration = FrameTime.Normalize(_acceleration);

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


			// Usded to ensure the player does not move outside of the visible window.
			// ProjectToScreen returns a Vec3. Each value between 0 and 100 means it is visible on screen in this dimension.
			Vec3 nextPos = Entity.Position + FrameTime.Normalize(new Vec3(0, _forwardSpeed, _upSpeed));
			Vec3 screenPosition = Env.Renderer.ProjectToScreen(nextPos.x, nextPos.y, nextPos.z);

			// In case new position on screen is outside of bounds
			if (screenPosition.x <=5 || screenPosition.x >= 95)
				_forwardSpeed = 0.001f;

			if (screenPosition.y <= 15 || screenPosition.y >= 75)
				_upSpeed = 0.001f;

			Speed = new Vec3(0, _forwardSpeed, _upSpeed);
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
							Entity.FreeSlot(2);
						else
							Entity.LoadGeometry(2, "objects/ships/Impact.cgf");
					}
				}
				
				_impactCoolDownFrameCount -=1;

				// Clean up cooldown
				if (_impactCoolDownFrameCount == 0)
					Entity.FreeSlot(2);
			}
		}

		/// <summary>
		/// Respawn this instance.
		/// </summary>
		public Vec3 Respawn(Vec3 position)
		{
			if (Entity != null)
				GamePool.FlagForPurge(Entity.ID);

			// Gain maximum life to be fully healthy again.
			GainLife(MaxLife);
			Hud.CurrentHud.SetEnergy(Life);

			return Spawn(position);
		}

		/// <summary>
		/// Fire this instance.
		/// Acutally just a dummy implementation to spawn more entities.
		/// </summary>
		public void Fire()
		{
			if (Entity == null)
				return;

			// Fire all weapons.
			foreach (WeaponBase weapon in _weapons) 
			{
				// Spawn projectile in front of Ship to avoid collision with self.
				weapon.Fire (Entity.Position + new Vec3(0, 1.2f, 0));
				Program.GameApp.MakeScore (1 * Hud.CurrentHud.Stage);
			}
		}

		public override void Collision()
		{
			if (_impactCoolDownFrameCount > 0)
				return;

			DrainLife(HitDamage);
			Hud.CurrentHud.SetEnergy(Life);

			if (Life == 0)
			{
				GamePool.FlagForPurge(Entity.ID);
				for (int i = 0; i < _weapons.Count; i++)
					GamePool.FlagForPurge (_weapons [i].Entity.ID);
				Destroy();
			}
			else
			{
				// Based on the previous frametime, calculate how many frames might be needed before 2 more seconds are over
				_impactCoolDownFrameCount = (int)(2f / FrameTime.Current);

				int framesForEachBlink = _impactCoolDownFrameCount / 10;

				// Set individual frames used to start- & stop blinking effects
				for (int i = 0; i < _impactCoolDownFrames.Length; i++)
					_impactCoolDownFrames[i] = _impactCoolDownFrameCount - (framesForEachBlink * (i-1));

			}
		}
	}
}

