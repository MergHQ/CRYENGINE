namespace CryEngine.Towerdefense
{
    public class UiTowerBuildMenu : UiTowerMenu
    {
        public event EventHandler<UnitTowerData> Build;

        public override void OnAwake()
        {
            base.OnAwake();

            NamePlate.DisplayName = "BUILD MENU";
            NamePlate.ShowLevel = false;

            foreach (var tower in AssetLibrary.Data.Towers.Register)
            {
                var instance = AddButton();
                instance.Button.Ctrl.OnPressed += () => OnBuildPressed(tower);
                instance.Label.Content = string.Format("{0} ({1})", tower.Name.ToUpper(), tower.UpgradePath[0].Cost);
                Menu.Add(instance);
            }
        }

        void OnBuildPressed(UnitTowerData towerData)
        {
            Build?.Invoke(towerData);
        }
    }
}
