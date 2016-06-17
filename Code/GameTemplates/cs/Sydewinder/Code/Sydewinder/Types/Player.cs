// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;
using CryEngine.Sydewinder.Types;
using CryEngine.Sydewinder.UI;
using System.Collections.Generic;
using CryEngine.Components;
using CryEngine.EntitySystem;

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
		private static ObjectSkin PlayerSkin = new ObjectSkin("objects/ships/player_ship.cgf");

		// Collection of different weapons.
		private List<WeaponBase> _weapons = new List<WeaponBase>();

		/// <summary>
		/// Initializes a new instance of the <see cref="CryEngine.Sydewinder.Player"/> class.
		/// </summary>
		/// <param name="position">Position</param>
		public Player()
		{
			// Fires as fast as spacebar is hit.
			_weapons.Add(Gun.Create(new Vec3(0, 25f, 0)));
			ExplosionTrigger = "player_explosion";

			// Use different explosion as the blue explosion in the base class is only used for enemies
			DestroyParticleEffect = Env.ParticleManager.FindEffect("spaceship.Destruction.explosion");
		}

		/// <summary>
		/// Spawn this instance.
		/// </summary>
		public static Player Create(Vec3 pos)
		{
			var player = Entity.Instantiate<Player>(pos, Quat.Identity, 10, PlayerSkin.Geometry);
			var particleEffect = Env.ParticleManager.FindEffect("spaceship.Trails.fire_trail");
			player.LoadParticleEmitter(1, particleEffect, 0.03f);

			// Get position where to spawn the particle effect.
			Vec3 jetPosition = player.GetHelperPos(0, "particle_01");

			// Rotate particle effect to ensure it's shown at the back of the ship.
			player.SetTM(1, Matrix34.Create(Vec3.One, Quat.CreateRotationX(Utils.Deg2Rad(270f)), jetPosition));

			// Rotate Z-Axis of player ship in degrees.
			player.Rotation = new Quat(Matrix34.CreateRotationZ(Utils.Deg2Rad(180.0f)));

			Hud.CurrentHud.SetEnergy (player.MaxLife);
			GamePool.AddObjectToPool(player);
			return player;
		}

		protected override void OnCollision(DestroyableBase hitEnt)
		{
			// Handle player collision with lower tunnel by changing the player location.
			if (hitEnt is Tunnel)
			{
				// As the only colliding tunnel is a lower tunnel, always change player position.
				Vec3 playerPos = Position;
				Position = new Vec3(playerPos.x, playerPos.y, 75);
				return;
			}

			// Don't let player collide with own projectiles.
			if (hitEnt is DefaultAmmo && !(hitEnt as DefaultAmmo).IsHostile)
				return;

			ProcessHit (hitEnt is Door);
		}

		public void UpdateRotation()
		{
			// Roll - to Right side.
			_rollAngle = _rollAngle * 0.8f + (_maxRollAngle * (_upSpeed / _maxSpeed)) * 0.2f;
			Rotation = Quat.CreateRotationXYZ(new Ang3(Utils.Deg2Rad(_pitchAngle), Utils.Deg2Rad(_rollAngle), _yawDeg));
		}

		public void UpdateSpeed()
		{			
			float acceleration = _acceleration * FrameTime.Delta;

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
				var currentScreenPos = Env.Renderer.ProjectToScreen (Position);
				_upSpeed = 1 - (eyePos - currentScreenPos).Normalized.y * 20;
			}

			// Usded to ensure the player does not move outside of the visible window.
			// ProjectToScreen returns a Vec3. Each value between 0 and 100 means it is visible on screen in this dimension.
			Vec3 nextPos = Position + new Vec3(0, _forwardSpeed, _upSpeed) * FrameTime.Delta;
			var screenPosition = Env.Renderer.ProjectToScreen(nextPos);
			// In case new position on screen is outside of bounds
			if (screenPosition.x <=0.05f || screenPosition.x >= 0.95f)
				_forwardSpeed = 0.001f;

			if (screenPosition.y <= 0.15f || screenPosition.y >= 0.75f)
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
							FreeSlot(2);
						else
							LoadGeometry(2, "objects/ships/Impact.cgf");
					}
				}
				
				_impactCoolDownFrameCount -=1;

				// Clean up cooldown
				if (_impactCoolDownFrameCount == 0)
					FreeSlot(2);
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
				weapon.Fire (Position + new Vec3(0, 1.2f, 0));
				Program.GameApp.MakeScore (1 * Hud.CurrentHud.Stage);
			}
		}

		public void ProcessHit(bool lethal = false)
		{
			if (_impactCoolDownFrameCount > 0)
				return;

			DrainLife(HitDamage);
			DestroyParticleEffect.Spawn (Position);
			if (lethal)
				DrainLife(MaxLife);
			Hud.CurrentHud.SetEnergy(Life);

			if (Life == 0)
			{
				GamePool.FlagForPurge(ID);
				for (int i = 0; i < _weapons.Count; i++)
					GamePool.FlagForPurge (_weapons [i].ID);
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

