// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;

namespace CryEngine
{
	/// <summary>
	/// Managed wrapper of the unmanaged IParticleEmitter, a particle effect emitter.
	/// Can be spawned using <see cref="ParticleEffect.Spawn(Vector3)"/>.
	/// </summary>
	public sealed class ParticleEmitter
	{
		/// <summary>
		/// Returns whether the emitter is still alive in the engine.
		/// </summary>
		public bool Alive
		{
			get
			{
				return NativeHandle.IsAlive();
			}
		}

		/// <summary>
		/// Get the Entity Slot that this particle emitter is attached to.
		/// </summary>
		public int AttachedEntitySlot
		{
			get
			{
				return NativeHandle.GetAttachedEntitySlot();
			}
		}

		/// <summary>
		/// Get the Entity ID that this particle emitter is attached to.
		/// </summary>
		public EntityId AttachedEntityId
		{
			get
			{
				return NativeHandle.GetAttachedEntityId();
			}
		}

		/// <summary>
		/// Get the <see cref="ParticleAttributes"/> of this <see cref="ParticleEmitter"/>.
		/// </summary>
		public ParticleAttributes Attributes
		{
			get
			{
				var nativeAttr = NativeHandle.GetAttributes();
				return nativeAttr == null ? null : new ParticleAttributes(nativeAttr);
			}
		}

		/// <summary>
		/// Get or set the effects used by the particle emitter.
		/// Will define the effect used to spawn the particles from the emitter.
		/// </summary>
		public ParticleEffect Effect
		{
			get
			{
				var nativeEffect = NativeHandle.GetEffect();
				return nativeEffect == null ? null : new ParticleEffect(nativeEffect);
			}
			set
			{
				if(value == null)
				{
					throw new ArgumentNullException(nameof(value));
				}
				NativeHandle.SetEffect(value.NativeHandle);
			}
		}

		/// <summary>
		/// Get or set the position of the <see cref="ParticleEmitter"/>.
		/// </summary>
		public Vector3 Position
		{
			get
			{
				var transform = NativeHandle.GetLocation();
				return transform.t;
			}
			set
			{
				var transform = NativeHandle.GetLocation();
				transform.t = value;
				NativeHandle.SetLocation(transform);
			}
		}

		/// <summary>
		/// Get or set the rotation of the <see cref="ParticleEmitter"/>.
		/// </summary>
		public Quaternion Rotation
		{
			get
			{
				var transform = NativeHandle.GetLocation();
				return transform.q;
			}
			set
			{
				var transform = NativeHandle.GetLocation();
				transform.q = value;
				NativeHandle.SetLocation(transform);
			}
		}

		/// <summary>
		/// Get or set the scale of the <see cref="ParticleEmitter"/>.
		/// </summary>
		public float Scale
		{
			get
			{
				var transform = NativeHandle.GetLocation();
				return transform.s;
			}
			set
			{
				var transform = NativeHandle.GetLocation();
				transform.s = value;
				NativeHandle.SetLocation(transform);
			}
		}

		/// <summary>
		/// Get or set the current <see cref="CryEngine.SpawnParameters"/>.
		/// </summary>
		public SpawnParameters SpawnParameters
		{
			get
			{
				var nativeParams = NativeHandle.GetSpawnParams();
				return nativeParams == null ? null : new SpawnParameters(nativeParams);
			}
			set
			{
				if(value == null)
				{
					throw new ArgumentNullException(nameof(value));
				}
				NativeHandle.SetSpawnParams(value.NativeHandle);
			}
		}

		/// <summary>
		/// Get the version of the <see cref="ParticleEmitter"/>. Can be EMF+, EMF, or WMF.
		/// </summary>
		public int Version
		{
			get
			{
				return NativeHandle.GetVersion();
			}
		}

		[SerializeValue]
		internal IParticleEmitter NativeHandle { get; private set; }

		internal ParticleEmitter(IParticleEmitter nativeHandle)
		{
			NativeHandle = nativeHandle;
		}

		/// <summary>
		/// Sets emitter state to active or inactive. Emitters are initially active.
		/// </summary>
		/// <remarks>
		/// If <paramref name="active"/> is <c>true</c>, the emitter updates and emits as normal, and deletes itself if the lifetime is limited.
		/// If <paramref name="active"/> is <c>false</c>, stops all emitter updating and particle emission. The existing particles continue to 
		/// update and render. The emitter is not deleted.
		/// </remarks>
		/// <param name="active"></param>
		public void Activate(bool active)
		{
			NativeHandle.Activate(active);
		}

		/// <summary>
		/// Programmatically adds particle to the emitter for rendering.
		/// The particles a spawned according to emitter settings.
		/// </summary>
		public void EmitParticle()
		{
			NativeHandle.EmitParticle();
		}

		/// <summary>
		/// Invalidates the cached data.
		/// </summary>
		public void InvalidateCachedEntityData()
		{
			NativeHandle.InvalidateCachedEntityData();
		}

		/// <summary>
		/// Removes emitter and all particles instantly.
		/// </summary>
		public void Kill()
		{
			NativeHandle.Kill();
		}

		/// <summary>
		/// Restarts the emitter from the start (if active).
		/// Any existing particles are re-used oldest first.
		/// </summary>
		public void Restart()
		{
			NativeHandle.Restart();
		}
	}
}
