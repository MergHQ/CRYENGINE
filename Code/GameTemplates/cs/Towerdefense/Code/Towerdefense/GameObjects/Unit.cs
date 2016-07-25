using System;
using System.Collections.Generic;
using System.Linq;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Framework;

using CryEngine.Towerdefense.Components;
using CryEngine.EntitySystem;
using CryEngine.UI;

namespace CryEngine.Towerdefense
{
    public interface IUpgradeableUnit
    {
        void Upgrade();
    }

    public class Unit : GameObject, IMouseDown, IMouseEnter, IMouseLeave
    {
        public event EventHandler<Unit> OnDeath;

        UiWorldSpaceElement ui;

        public UnitData UnitData { get; private set; }
        public Health Health { get; private set; }
        public UiWorldSpaceElement UI { get { return ui; } }

        public bool ShowUI
        {
            set
            {
                if (ui != null)
                    ui.Active = value;
            }
        }

        public override void OnInitialize()
        {
            base.OnInitialize();

            // Add Health component
            Health = AddComponent<Health>();
            Health.OnDeath += Kill;

            // Create Unit UI
            ui = CreateUI(UiHud.Canvas);

            if (ui != null)
            {
                ui.Offset = new Vec2(0, -50f);

                // Set UI so it renders behind the rest of the HUD
                ui.SetAsFirstSibling();
            }
        }

        public void SetUnitData(UnitData unitData)
        {
            UnitData = unitData;
            Health.SetMaxHealth(unitData.MaxHealth);
            Health.SetHealth(unitData.MaxHealth);
            NativeEntity.LoadGeometry(0, unitData.Mesh);
            NativeEntity.SetMaterial(unitData.Material);
            OnSetUnitData(unitData);
        }

        public virtual void UpdateUnit(LevelManager level)
        {
            OnUpdateUnit(level);
            ui?.SetPosition(Position);
        }

        public override void OnDestroy()
        {
            base.OnDestroy();
            Health.OnDeath -= Kill;
            ui?.Destroy();
        }

        /// <summary>
        /// Kills the unit and invokes the OnDeath event.
        /// </summary>
        protected virtual void Kill()
        {
            OnDeath?.Invoke(this);
        }

        public virtual void OnMouseDown(MouseButton button) { }
        public virtual void OnMouseEnter() { }
        public virtual void OnMouseLeave() { }
        protected virtual void OnUpdateUnit(LevelManager level) { }
        protected virtual void OnSetUnitData(UnitData unitData) { }
        protected virtual UiWorldSpaceElement CreateUI(Canvas canvas) { return null; }
    }
}
