using System;
using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine.Framework
{
    public static class EntityEx
    {
        public static void SetMaterial(this Entity entity, string path)
        {
            var material = Global.gEnv.p3DEngine.GetMaterialManager().LoadMaterial(path);
            if (material != null)
                entity.BaseEntity.SetMaterial(material);
        }

        public static void SetMaterial(this IEntity entity, string path)
        {
            var material = Global.gEnv.p3DEngine.GetMaterialManager().LoadMaterial(path);
            if (material != null)
                entity.SetMaterial(material);
        }

        public static void SetSlotMaterial(this IEntity entity, int slot, string path)
        {
            var material = Global.gEnv.p3DEngine.GetMaterialManager().LoadMaterial(path);
            if (material != null)
                entity.SetSlotMaterial(slot, material);
        }

        public static void LookAt(this IEntity entity, Vec3 target)
        {
            var rot = entity.GetRotation();
            rot = QuatEx.LookAt(entity.GetPos(), target);
            entity.SetRotation(rot);
        }


        public static void Physicalize(this IEntity entity)
        {
            var physParams = new SEntityPhysicalizeParams()
            {
                mass = -1,
                density = -1,
                type = (int)EPhysicalizationType.ePT_Rigid,
            };

            entity.Physicalize(physParams);
        }
    }
}

