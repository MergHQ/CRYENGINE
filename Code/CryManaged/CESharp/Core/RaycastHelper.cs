using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine
{
    /// <summary>
    /// Helper class for performing Raycasts.
    /// </summary>
    public static class RaycastHelper
    {
        private static uint _rayFlags = unchecked((uint)((int)geom_flags.geom_colltype_ray << (int)rwi_flags.rwi_colltype_bit) | (int)rwi_flags.rwi_colltype_any | (10 & (int)rwi_flags.rwi_pierceability_mask) | ((int)geom_flags.geom_colltype14 << (int)rwi_flags.rwi_colltype_bit));

        public static RaycastOutput Raycast(Vec3 position, Vec3 direction, float distance, entity_query_flags flags, uint rayFlags, int maxHits)
        {
            var rayHit = new ray_hit();
            var hits = Env.PhysicalWorld.RayWorldIntersection(position, direction * distance, (int)flags, rayFlags, rayHit, maxHits);
            return new RaycastOutput(rayHit, hits);
        }

        public static RaycastOutput Raycast(Vec3 position, Vec3 direction)
        {
            return Raycast(position, direction, float.PositiveInfinity, entity_query_flags.ent_all, _rayFlags, 1);
        }

        public static RaycastOutput Raycast(Vec3 position, Vec3 direction, float distance)
        {
            return Raycast(position, direction, distance, entity_query_flags.ent_all, _rayFlags, 1);
        }
    }

    /// <summary>
    /// Wrapper class for a raycast output.
    /// </summary>
    public class RaycastOutput
    {
        public enum HitType
        {
            Unknown,
            Terrain,
            Entity,
            Static,
        }

        private uint _hitID;

        public ray_hit Ray { get; private set; }
        public int Hits { get; private set; }
        public bool Intersected { get { return Hits != 0; } }
        public Vec3 Point { get { return Ray.pt; } }
        public Vec3 Normal { get { return Ray.n; } }
        public float Distance { get { return Ray.dist; } }

        /// <summary>
        /// The native CRYENGINE entity that was hit. Returns null if nothing was hit, or the hit wasn't an Entity.
        /// </summary>
        public IEntity HitNativeEntity { get { return _hitID != 0 ? Env.EntitySystem.GetEntity(_hitID) : null; } }

        /// <summary>
        /// The CESharp entity that was hit. Returns null if nothing was hit, the entity isn't a CESharp entity, or the hit wasn't an Entity.
        /// </summary>
        public BaseEntity HitBaseEntity { get { return EntityFramework.GetEntity(_hitID); } }

        /// <summary>
        /// The type of surface that was hit, if any. Returns 'Unknown' if nothing was hit.
        /// </summary>
        public HitType Type { get; private set; }

        /// <summary>
        /// The native entity's physical entity, if something was hit.
        /// </summary>
        public IPhysicalEntity Collider { get { return Ray.pCollider; } }

        public RaycastOutput(ray_hit ray, int hits)
        {
            Ray = ray;
            Hits = hits;

            if (ray.bTerrain == 1)
            {
                Type = HitType.Terrain;
            }
            else
            {
                var pEntity = Env.EntitySystem.GetEntityFromPhysics(ray.pCollider);

                if (pEntity != null)
                    pEntity = pEntity.GetAttachedChild(ray.partid);

                if (pEntity != null)
                {
                    // We hit some sort of entity.
                    _hitID = pEntity.GetId();

                    Type = HitType.Entity;
                }
                else
                {
                    // We hit something that isn't registered with the entity system. Probably a designer object or brush.
                    Type = HitType.Static;
                }
            }
        }

        /// <summary>
        /// Returns True if the entity passed in was hit.
        public bool HasHitEntity(BaseEntity entity)
        {
            return HitBaseEntity != null ? HitBaseEntity.Id == entity.Id : false;
        }
    }
}
