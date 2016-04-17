// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Sydewinder.Types;
using CryEngine.EntitySystem;

namespace CryEngine.Sydewinder.Framework
{
	public class EntityCollisionCheck : IEntityEventListener
	{
		private static EntityCollisionCheck _entityCollisionCheckInstance = null;

		static EntityCollisionCheck()
		{
			_entityCollisionCheckInstance = new EntityCollisionCheck(); 
		}

		public override void OnEntityEvent(IEntity pEntity, SEntityEvent arg1)
		{
			// As the only attached Entity event is the collision event, get physics of the collision.
			EventPhysCollision collision = arg1.GetEventPhysCollision();

			// Retrieve both entities which are part of the collision.
			IEntity entOne = Env.EntitySystem.GetEntityFromPhysics(collision.GetFirstEntity());
			IEntity entTwo = Env.EntitySystem.GetEntityFromPhysics(collision.GetSecondEntity());

			if (entOne == null || entTwo == null)
				return;
			
			DestroyableBase tOne = GamePool.GetMoveableByEntityId(entOne.GetId());
			DestroyableBase tTwo = GamePool.GetMoveableByEntityId(entTwo.GetId());

			// Check if one of the objects is already dead.
			if ((tOne != null && !tOne.IsAlive) || (tOne != null && !tOne.IsAlive))
				return;

			// Handle player collision with lower tunnel by changing the player location.
			if ((tOne is Player && tTwo is Tunnel) || (tOne is Tunnel && tTwo is Player))
			{
				// As the only colliding tunnel is a lower tunnel, always change player position.
				Player p = (tOne is Player) ? tOne as Player : tTwo as Player;
				Vec3 playerPos = p.Position;
				p.Position = new Vec3(playerPos.x, playerPos.y, 75);

				return;
			}

			// Don't let player collide with own projectiles.
			if ((tOne is Player && tTwo is DefaultAmmo) || (tTwo is Player && tTwo is DefaultAmmo))
			{
				DefaultAmmo ammo = (tOne is DefaultAmmo) ? tOne as DefaultAmmo : tTwo as DefaultAmmo;

				if (!ammo.IsHostile)
					return;
			}

			if (tOne != null && tOne.IsAlive)
				tOne.Collision();

			if (tTwo != null && tTwo.IsAlive)
				tTwo.Collision();

			// Handle Player and Projectile collides with enemies.
			if (tOne is Player || tTwo is Player || tOne is ProjectileBase || tTwo is ProjectileBase)
			{
				if (tOne is Enemy)
					(tOne as Enemy).Destroy();
				
				 if (tTwo is Enemy)
					(tTwo as Enemy).Destroy();
			} 
		}

		public static void SubscribeToEntityCollision(uint entityId)
		{ 
			Env.EntitySystem.AddEntityEventListener(entityId, EEntityEvent.ENTITY_EVENT_COLLISION, _entityCollisionCheckInstance);
		}

		public static void UnsubscribeFromEntityCollision(uint entityId)
		{ 
			Env.EntitySystem.RemoveEntityEventListener(entityId, EEntityEvent.ENTITY_EVENT_COLLISION, _entityCollisionCheckInstance);
		}
	}
}
