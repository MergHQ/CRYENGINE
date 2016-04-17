// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using CryEngine.Common;

namespace CryEngine.EntitySystem
{
	public class Entity
	{
		private static Dictionary<uint, Entity> _entityById = new Dictionary<uint, Entity> ();

		public uint ID { get; private set; }
		public Vec3 Position { get { return BaseEntity.GetPos (); } set { BaseEntity.SetPos (value); } }
		public Quat Rotation { get { return BaseEntity.GetRotation(); } set{ BaseEntity.SetRotation (value);} }
		public Vec3 Scale { get { return BaseEntity.GetScale(); } set{ BaseEntity.SetScale (value);} }
		public string Name { get { return BaseEntity.GetName (); } }
		public IMaterial Material { get { return BaseEntity.GetMaterial(); } set { BaseEntity.SetMaterial (value); } }
		public bool Exists { get { return BaseEntity != null; } }

		public IEntity BaseEntity
		{
			get 
			{
				if (ID == 0)
					return null;
				return Env.EntitySystem.GetEntity(ID);
			}
		}
		public IPhysicalEntity PhysicalEntity
		{
			get 
			{
				return BaseEntity.GetPhysics ();
			}
		}

		public Entity(IEntity baseEntity)
		{
			ID = baseEntity.GetId();
		}

		public static Entity ById(uint id)
		{
			if (id == 0)
				return null;
			Entity ent;
			if (_entityById.TryGetValue (id, out ent))
				return ent;

			var baseEnt = Env.EntitySystem.GetEntity (id);
			if (baseEnt == null)
				return null;
			
			return ent = _entityById [id] = new Entity (baseEnt);
		}

		public static Entity ByName(string name)
		{
			var ent = _entityById.Values.FirstOrDefault (x => x.Exists && x.Name == name);
			if (ent != null)
				return ent;

			var baseEnt = Env.EntitySystem.FindEntityByName (name);
			if (baseEnt == null)
				return null;
			
			return ent = _entityById [baseEnt.GetId()] = new Entity (baseEnt);
		}

		public static Entity Instantiate(Vec3 position, Quat rot, float scale, string model, string material = null)
		{
			SEntitySpawnParams spawnParams = new SEntitySpawnParams() 
			{
				pClass = Env.EntitySystem.GetClassRegistry().GetDefaultClass(),
				vPosition = position,
				vScale = new Vec3(scale)
			};

			var spawnedEntity = Env.EntitySystem.SpawnEntity(spawnParams, true);
			spawnedEntity.LoadGeometry(0, model);
			spawnedEntity.SetRotation(rot);

			if (material != null)
			{
				IMaterial mat = Env.Engine.GetMaterialManager().LoadMaterial(material);
				spawnedEntity.SetMaterial(mat);
			}

			var physics = new SEntityPhysicalizeParams() 
			{
				density = 1,
				mass = 0f,
				type = (int)EPhysicalizationType.ePT_Rigid,
			};
			spawnedEntity.Physicalize(physics);
			return new Entity(spawnedEntity);
		}

		public void Remove()
		{
			Env.EntitySystem.RemoveEntity (ID);
		}

		public void LoadParticleEmitter(int slot, IParticleEffect emitter, float scale = 1.0f)
		{
			var sp = new SpawnParams { fSizeScale = scale };
			BaseEntity.LoadParticleEmitter(slot, emitter, sp, false, false);
		}

		public Vec3 GetHelperPos(int slot, string helperName)
		{
			return BaseEntity.GetStatObj(slot).GetHelperPos(helperName);
		}

		public void SetTM(int slot, Matrix34 mx)
		{
			BaseEntity.SetSlotLocalTM(slot, mx);	
		}

		public void FreeSlot(int slot)
		{
			BaseEntity.FreeSlot (slot);
		}

		public void LoadGeometry(int slot, string url)
		{
			BaseEntity.LoadGeometry (slot, url);
		}

		public void SetSlotFlag(int slot, EEntitySlotFlags flags)
		{
			BaseEntity.SetSlotFlags (slot, (uint)flags);
		}

		public IParticleEmitter GetParticleEmitter(int slot)
		{
			return BaseEntity.GetParticleEmitter (slot);
		}

		public void LoadLight(int slot, CDLight light)
		{
			BaseEntity.LoadLight (slot, light);
		}
	}
}
