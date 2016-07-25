// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;
using System.Globalization;
using CryEngine.EntitySystem;

namespace CryEngine.Components
{
	/// <summary>
	/// Allows for controlling position and orientation of CryEngine's Player entity.
	/// </summary>
	public class Camera : Component
	{
		public event EventHandler<Camera> OnPlayerEntityAssigned; ///< Raiused if a base entity was assigned.

		private Entity _hostEntity;
		private Vec3 _position = Vec3.Up;
		private Vec3 _forwardDir = Vec3.Forward;
		private static ICVar _fovCVar;

		public Entity HostEntity
		{
			get 
			{
				return _hostEntity;
			}
			set
			{
				_hostEntity = value;
				if (value == null)
					return;
				_position = _hostEntity.Position;
				_forwardDir = _hostEntity.Rotation.Forward;
			}
		} ///< Represents IEntity which Camera uses to control CryEngine sided view.

		public static Camera Current = null;

		public Vec3 Position
		{
			set 
			{
				_position = value;
				if (HostEntity != null)
					HostEntity.Position = value;
			}
			get 
			{
				return _position;
			}
		} ///< Sets position of assigned HostEntity.

		public Vec3 ForwardDirection
		{
			set
			{
				_forwardDir = value.Normalized;
				if (HostEntity != null) 
				{
					var q = Quat.CreateRotationVDir (_forwardDir);
					HostEntity.Rotation = q;
				}
			}
			get 
			{
				return _forwardDir;
			}
		} ///< Sets rotation of assigned HostEntity

        public Quat Rotation
        {
            set
            {
                ForwardDirection = value * Vec3.Forward;
            }
            get
            {
                return HostEntity != null ? HostEntity.Rotation : Quat.Identity;
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
				if (_fovCVar == null)
					_fovCVar = Env.Console.GetCVar ("gamezero_cam_fov");
				
				return _fovCVar != null ? _fovCVar.GetFVal() : 60.0f;
			}
			set
			{
				if (_fovCVar == null)
					_fovCVar = Env.Console.GetCVar ("gamezero_cam_fov");

				if (_fovCVar != null)
					_fovCVar.Set (value);
			}
		}

		/// <summary>
		/// Tries to find and assign Player Entity from scene automatically.
		/// </summary>
		public void OnUpdate()
		{
			if (HostEntity == null) 
			{
				if ((HostEntity = Entity.ByName ("Player")) != null) 
				{
                    // Automatically assume the position and rotation of the current native camera.
                    Position = Env.Renderer.GetCamera().GetPosition();
                    Rotation = new Quat(Env.Renderer.GetCamera().GetAngles());
					if (OnPlayerEntityAssigned != null)
						OnPlayerEntityAssigned (this);
				}
			}				
		}

		public static Vec3 Unproject(int x, int y)
		{
			return Env.Renderer.UnprojectFromScreen (x, Screen.Height - y);
		}
	}
}
