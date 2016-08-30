namespace CryEngine.Towerdefense
{
    public class UiTowerUpgradeMenu : UiTowerMenu
    {
        public event EventHandler UpgradeFocusEnter;
        public event EventHandler UpgradeFocusLost;
        public event EventHandler Upgrade;
        public event EventHandler Sell;

        UiTowerButton sell;
        UiTowerButton upgrade;

        public int SellValue { set { sell.Label.Content = string.Format("SELL ({0})", value); } }
        public int UpgradeCost { set { upgrade.Label.Content = string.Format("UPGRADE ({0})", value); } }
        public bool SellEnabled { set { sell.Active = value; } }
        public bool UpgradeEnabled { set { upgrade.Active = value; } }

        public override void OnAwake()
        {
            base.OnAwake();

            // Add the Sell button.
            sell = AddButton();
            sell.Button.Ctrl.OnPressed += OnSellPressed;
            sell.Button.BackgroundImageUrl = AssetLibrary.UI.ButtonCancel;

            // Add the Upgrade button.
            upgrade = AddButton();
            upgrade.Button.Ctrl.OnPressed += OnUpgradePressed;
            upgrade.MouseEnter += OnUpgradeFocusEnter;
            upgrade.MouseLeft += OnUpgradeFocusLost;
            upgrade.Button.BackgroundImageUrl = AssetLibrary.UI.ButtonUpgrade;

            // Add both buttons to the vertical menu so that it can layout the buttons.
            // Whenever a new item is added, the layout is automatically refreshed.
            Menu.Add(sell);
            Menu.Add(upgrade);
        }

        void OnUpgradeFocusEnter()
        {
            UpgradeFocusEnter?.Invoke();
        }

        void OnUpgradeFocusLost()
        {
            UpgradeFocusLost?.Invoke();
        }

        void OnUpgradePressed()
        {
            Upgrade?.Invoke();
        }

        void OnSellPressed()
        {
            Sell?.Invoke();
        }

        public void OnDestroy()
        {
            sell.Button.Ctrl.OnPressed -= OnSellPressed;
            upgrade.Button.Ctrl.OnPressed -= OnUpgradePressed;
            upgrade.MouseEnter -= OnUpgradeFocusEnter;
            upgrade.MouseLeft -= OnUpgradeFocusLost;
        }
    }
}
