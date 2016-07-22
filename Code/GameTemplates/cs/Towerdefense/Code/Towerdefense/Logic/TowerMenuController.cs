using System;
using CryEngine.Common;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Controls the logic for the menu that appears when left-clicking on a Unit.
    /// </summary>
    class TowerMenuController
    {
        public UiTowerMenu UI { get; private set; }

        public bool Active { get { return UI.Active; } }

        public TowerMenuController(UiTowerMenu ui)
        {
            UI = ui;
        }

        public void ToggleMenu()
        {
            UI.Active = !UI.Active;
        }

        public virtual void ShowMenu()
        {
            UI.Active = true;
        }

        public virtual void HideMenu()
        {
            UI.Active = false;
        }

        public void Destroy()
        {
            UI = null;
        }
    }

    class TowerUpgradeMenuController : TowerMenuController
    {
        public TowerUpgradeMenuController(UiTowerUpgradeMenu ui) : base(ui)
        {

        }

        public void ToggleMenu(Tower tower)
        {
            ToggleMenu();
            UpdateMenu(tower);
        }

        public void ShowMenu(Tower tower)
        {
            ShowMenu();
            UpdateMenu(tower);
        }

        public void UpdateMenu(Tower tower)
        {
            var ui = UI as UiTowerUpgradeMenu;
            ui.SetPosition(tower.Position);
            ui.NamePlate.DisplayName = tower.Name;
            ui.NamePlate.Level = tower.Level + 1;
            ui.SellValue = tower.CurrentTower.RefundCost;
            ui.UpgradeCost = tower.NextTower != null ? tower.NextTower.Cost : 0;
            ui.UpgradeEnabled = tower.NextTower != null;
        }
    }

    class TowerBuildMenuController : TowerMenuController
    {
        public TowerBuildMenuController(UiTowerBuildMenu ui) : base(ui)
        {

        }

        public void ShowMenu(Vec3 position)
        {
            ShowMenu();
            UI.SetPosition(position);
        }
    }
}

