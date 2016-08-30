using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class UiInventory : UIElement
    {
        UiTowerBuildMenu buildMenu;
        UiTowerUpgradeMenu upgradeMenu;
        UiTextMessage infoText;
        UiTextMessage warningText;

        public UiTowerBuildMenu BuildMenu { get { return buildMenu; } }
        public UiTowerUpgradeMenu UpgradeMenu { get { return upgradeMenu; } }
        public string DisplayMessage { set { infoText.Show(value, 4f); } }
        public string DisplayWarning { set { warningText.Show(value, 4f); } }

        public void OnAwake()
        {
            upgradeMenu = Instantiate<UiTowerUpgradeMenu>(this);
            upgradeMenu.SetAsLastSibling();
            upgradeMenu.Active = false;
            upgradeMenu.Offset = new Vec2(0, -50f);

            buildMenu = Instantiate<UiTowerBuildMenu>(this);
            buildMenu.SetAsLastSibling();
            buildMenu.Active = false;
            buildMenu.Offset = new Vec2(0, -50f);

            RectTransform.Size = new Point(Screen.Width, Screen.Height);
            RectTransform.Alignment = Alignment.Center;

            infoText = Instantiate<UiTextMessage>(this);
            infoText.Text.ApplyStyle(AssetLibrary.TextStyles.HudMessageInfo);
            infoText.RectTransform.Alignment = Alignment.Bottom;
            infoText.RectTransform.Size = new Point(400f, 30f);
            infoText.RectTransform.Padding = new Padding(10f, -85f);
            infoText.RectTransform.Pivot = new Point(0.5f, 1f);
            infoText.Text.Alignment = Alignment.Center;
            infoText.Text.Offset = new Point(5f, 0f);

            warningText = Instantiate<UiTextMessage>(this);
            warningText.Text.ApplyStyle(AssetLibrary.TextStyles.HudMessageWarning);
            warningText.RectTransform.Alignment = Alignment.Bottom;
            warningText.RectTransform.Size = new Point(400f, 30f);
            warningText.RectTransform.Padding = new Padding(10f, -45f);
            warningText.RectTransform.Pivot = new Point(0.5f, 1f);
            warningText.Text.Alignment = Alignment.Center;
            warningText.Text.Offset = new Point(5f, 0f);
        }
    }
}