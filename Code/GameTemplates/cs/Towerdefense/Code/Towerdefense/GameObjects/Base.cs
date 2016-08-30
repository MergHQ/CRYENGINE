using CryEngine.Common;
using CryEngine.Framework;
using CryEngine.Towerdefense.Components;
using CryEngine.EntitySystem;

namespace CryEngine.Towerdefense
{
    public class Base : GameObject
    {
        [EntityProperty]
        public int InitialHealth { get; set; }

        public Health Health { get; private set; }

//        GameObject core;

        public override void OnInitialize()
        {
            base.OnInitialize();

            Health = new Health();
            NativeEntity.LoadGeometry(0, AssetLibrary.Meshes.BasePlate);
            NativeEntity.SetMaterial(AssetLibrary.Materials.BasePlate);

            NativeEntity.LoadGeometry(1, AssetLibrary.Meshes.Diamond);
            NativeEntity.SetSlotMaterial(1, AssetLibrary.Materials.BaseCore);
            NativeEntity.SetSlotLocalTM(1, Matrix34.Create(Vec3.One, Quat.Identity, new Vec3(0, 0, 2f)));
        }

        public void Setup()
        {
            Health.SetMaxHealth(InitialHealth);
            Health.SetHealth(InitialHealth);

            //core = GameObjectNew.Instantiate<GameObjectNew>();
            //core.NativeEntity.LoadGeometry(0, AssetLibrary.Meshes.Diamond);
            //core.NativeEntity.SetMaterial(AssetLibrary.Materials.BaseCore);
            //core.Position = Position + new Vec3(0, 0, 2f);

            //var sineWaveComp = core.AddComponent<SineWaveMovement>();
            //sineWaveComp.Period = 0.005f;

            //var rotator = core.AddComponent<Rotator>();
            //rotator.Speed = 1f;
        }
    }
}

