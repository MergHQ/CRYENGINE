using System;
using CryEngine.Common;
using CryEngine.Framework;

using CryEngine.UI;

namespace CryEngine.Towerdefense
{
    public class Inventory : GameComponent
    {
        UiInventory ui;
        Resources resources;
        Unit ghost;
        MouseController mouseController;
        InputController inputController;
        TowerUpgradeMenuController upgradeMenuController;
        TowerBuildMenuController buildMenuController;
        ConstructionPointController selectedConstructionPoint;
        UnitTowerData ghostUnitData;

        const float ghostHeight = 2f;

        TowerMenuController ActiveMenu
        {
            get
            {
                if (upgradeMenuController.Active)
                {
                    return upgradeMenuController;
                }
                else if (buildMenuController.Active)
                {
                    return buildMenuController;
                }
                return null;
            }
        }

        public void Setup(Resources resources)
        {
            this.resources = resources;

            // Create UI
            ui = SceneObject.Instantiate<UiInventory>(UiHud.Canvas) as UiInventory;
            ui.UpgradeMenu.Upgrade += Upgrade;
            ui.UpgradeMenu.Sell += Sell;
            ui.BuildMenu.Build += Build;

            // Set UI so it renders last
            ui.SetAsFirstSibling();

            // Create the 'ghost' version of the Unit to be placed. It is the same as the Unit but with a transparent material.
            ghostUnitData = AssetLibrary.Data.Towers.TowerBasic;
            ghost = Factory.CreateGhost(ghostUnitData);

            // Create the controllers that handle menu logic.
            upgradeMenuController = new TowerUpgradeMenuController(ui.UpgradeMenu);
            buildMenuController = new TowerBuildMenuController(ui.BuildMenu);

            // Create the controller that handles mouse interaction.
            mouseController = new MouseController();
            mouseController.NoCell += MouseController_OnNoCell;
            mouseController.EnterOccupiedCell += MouseController_OnEnterOccupiedCell;
            mouseController.EnterUnoccupiedCell += Mouse_OnEnterUnoccupiedCell;

            // Prevent cells from being highlighted if the mouse is inside a piece of UI.
            mouseController.CanSelect = new Func<bool>(() =>
            {
                if (ui.UpgradeMenu.Active)
                {
                    return !Mouse.CursorInside(ui.UpgradeMenu);
                }
                else if (ui.BuildMenu.Active)
                {
                    return !Mouse.CursorInside(ui.BuildMenu);
                }
                return true;
            });

            // Create the controller that handles mouse input
            inputController = new InputController(mouseController);
            inputController.Select += InputController_Select;
            inputController.SelectionCleared += InputController_SelectionCleared;
        }

        void MouseController_OnNoCell()
        {
            // Hide the ghost object when the mouse is hovering over nothing.
            if (mouseController.Previous != null)
            {
                ghost.NativeEntity.Invisible(true);
                mouseController.Previous.RemoveHighlight();
            }
            ghost.Hide();
        }

        void Mouse_OnEnterUnoccupiedCell(ConstructionPoint cell)
        {
            // If a cell is under the mouse, highlight it and move the ghost object to the construction point's position.
            cell.Highlight();
            ghost.Position = cell.Position + Vec3.Up * ghostHeight;
            ghost.Show();
        }

        void MouseController_OnEnterOccupiedCell(ConstructionPoint cell)
        {
            // If the mouse is over a construction point occupied by a tower, remove the highlight and hide the ghost object.
            cell.RemoveHighlight();
            ghost.Hide();
        }

        void InputController_Select(InputController.InputEvent inputEvent)
        {
            selectedConstructionPoint = inputEvent.Selection.Controller;

            if (!inputEvent.SelectionChanged && ActiveMenu != null)
            {
                // Hide the menu of the currently selected object if it is currently visible.
                ActiveMenu.HideMenu();
            }
            else
            {
                if (selectedConstructionPoint.Occupied)
                {
                    buildMenuController.HideMenu();
                    upgradeMenuController.ShowMenu(selectedConstructionPoint.Tower);
                }
                else
                {
                    upgradeMenuController.HideMenu();
                    buildMenuController.ShowMenu(selectedConstructionPoint.Position);
                }
            }
        }

        void InputController_SelectionCleared()
        {
            if (ActiveMenu != null)
                ActiveMenu.HideMenu();
        }

        void Build(UnitTowerData towerData)
        {
            if (towerData.UpgradePath.Count == 0)
            {
                Log.Error("Cannot build Tower '{0}' as it has no data inside its upgrade path. It needs at least one upgrade.", towerData.Name);
                return;
            }

            if (resources.Value < towerData.UpgradePath[0].Cost)
            {
                ui.DisplayWarning = AssetLibrary.Strings.BuildNotEnoughResources;
            }
            else
            {
                ui.DisplayMessage = AssetLibrary.Strings.BuildSuccessful;

                // Tell the construction point to place a tower. This is where the tower is instantiated and initialized.
                selectedConstructionPoint.PlaceTower(towerData);

                ghost.Hide();

                resources.Remove(towerData.UpgradePath[0].Cost);

                buildMenuController.HideMenu();

                // Display a hitmarker UI showing how many resources have been deducted for building this tower.
                UiHud.CreateHitMarker(selectedConstructionPoint.Position, (text) =>
                {
                    text.Label.ApplyStyle(AssetLibrary.TextStyles.HudWorldSpaceText);
                    text.Label.Color = Color.Red;
                    text.Label.Content = "-" + towerData.UpgradePath[0].Cost.ToString();
                });
            }
        }

        void Upgrade()
        {
            if (selectedConstructionPoint.Tower.NextTower != null)
            {
                if (resources.Value < selectedConstructionPoint.Tower.NextTower.Cost)
                {
                    ui.DisplayWarning = AssetLibrary.Strings.BuildNotEnoughResourcesToUpgrade;
                }
                else
                {
                    ui.DisplayMessage = AssetLibrary.Strings.BuildUpgradeSuccesful;

                    resources.Remove(selectedConstructionPoint.Tower.NextTower.Cost);

                    // Upgrade the selected construction point's associated tower.
                    selectedConstructionPoint.Tower?.Upgrade();

                    UiHud.CreateHitMarker(selectedConstructionPoint.Tower.Position, (text) =>
                    {
                        text.Label.ApplyStyle(AssetLibrary.TextStyles.HudWorldSpaceText);
                        text.Label.Color = Color.White;
                        text.Label.Content = "UPGRADE!";
                    });
                }

                // Update the menu to reflect the values of the recently upgrade tower.
                upgradeMenuController.UpdateMenu(selectedConstructionPoint.Tower);
            }
        }

        void Sell()
        {
            if (selectedConstructionPoint == null)
                return;

            // Tell the Construction Point to remove its associated tower.
            // The 'onRemove' callback allows access to the tower before it is removed.
            selectedConstructionPoint.RemoveTower((tower) =>
            {
                ui.DisplayMessage = string.Format("Removed object '{0}' with id {1}", tower.Name, tower.Entity.ID);

                // Refund cost
                resources.Add(tower.CurrentTower.RefundCost);

                UiHud.CreateHitMarker(tower.Position, (text) =>
                {
                    text.Label.Alignment = Alignment.Center;
                    text.Label.ApplyStyle(AssetLibrary.TextStyles.HudWorldSpaceText);
                    text.Label.Color = Color.Green;
                    text.Label.Content = "+" + tower.CurrentTower.RefundCost;
                });
            });

            upgradeMenuController.HideMenu();

            selectedConstructionPoint = null;
        }

        protected override void OnDestroy()
        {
            ui.UpgradeMenu.Upgrade -= Upgrade;
            ui.UpgradeMenu.Sell -= Sell;
            ui.BuildMenu.Build -= Build;
            ui.Destroy();
            ghost.Destroy();

            mouseController.NoCell -= MouseController_OnNoCell;
            mouseController.EnterOccupiedCell -= MouseController_OnEnterOccupiedCell;
            mouseController.EnterUnoccupiedCell -= Mouse_OnEnterUnoccupiedCell;
            mouseController.CanSelect = null;
            mouseController.Destroy();

            inputController.Destroy();

            upgradeMenuController.Destroy();
        }
    }
}

