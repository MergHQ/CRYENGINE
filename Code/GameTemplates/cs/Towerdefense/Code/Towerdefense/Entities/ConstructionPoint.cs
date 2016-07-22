using System;
using CryEngine.EntitySystem;
using CryEngine.Common;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// An entity that represents a Construction Point where a tower can be placed. The logic is controlled by ConstructionPointController.
    /// </summary>
    public class ConstructionPoint : BaseEntity
    {
        ConstructionPointController controller;

        const int slotInteractionPlate = 0;
        const int slotBasePlate = 1;

        public Vec3 Position { get { return NativeEntity.GetPos(); } }

        public ConstructionPointController Controller
        {
            get
            {
                return controller;
            }
            set
            {
                controller = value;
                controller.OnOccupiedStateChanged += Controller_OnOccupiedStateChanged;
                controller.OnUnitRemoved += Controller_OnUnitRemoved;
                NativeEntity.SetPos(controller.Position);
                NativeEntity.SetRotation(controller.Rotation);
            }
        }

        public override void OnInitialize()
        {
            var physParams = new SEntityPhysicalizeParams()
            {
                mass = -1,
                density = -1,
                type = (int)EPhysicalizationType.ePT_Rigid,
            };

            // Create the mesh that responds to mouse interaction
            NativeEntity.LoadGeometry(slotInteractionPlate, AssetLibrary.Meshes.Hexagon);
            NativeEntity.SetSlotMaterial(slotInteractionPlate, AssetLibrary.Materials.GreyMin);

            // Create the BasePlate that represents the Construction Point.
            NativeEntity.LoadGeometry(slotBasePlate, AssetLibrary.Meshes.BasePlate);
            NativeEntity.SetSlotMaterial(slotBasePlate, AssetLibrary.Materials.BasePlate);

            NativeEntity.Physicalize(physParams);
        }

        public override void OnRemove()
        {
            if (controller != null)
            {
                controller.OnOccupiedStateChanged -= Controller_OnOccupiedStateChanged;
                controller.OnUnitRemoved -= Controller_OnUnitRemoved;   
            }
        }

        public void Highlight()
        {
            if (!controller.Occupied)
                NativeEntity.SetSlotMaterial(slotInteractionPlate, AssetLibrary.Materials.Yellow);
        }

        public void RemoveHighlight()
        {
            if (!controller.Occupied)
                NativeEntity.SetSlotMaterial(slotInteractionPlate, AssetLibrary.Materials.GreyMin);
        }

        void Controller_OnOccupiedStateChanged ()
        {
            var material = controller.Occupied ? AssetLibrary.Materials.Green : AssetLibrary.Materials.GreyMin;
            NativeEntity.SetSlotMaterial(slotInteractionPlate, material);
        }

        void Controller_OnUnitRemoved (Unit unit)
        {
            NativeEntity.SetSlotMaterial(slotInteractionPlate, AssetLibrary.Materials.GreyMin);
        }
    }
}

