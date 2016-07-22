using System;
using System.Collections.Generic;
using System.IO;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public static class AssetLibrary
    {
        public static class UI
        {
            static string folder = "textures/ui/";

            public static string Header { get { return GetFullAssetPath("ui_header.png"); } }
            public static string HeaderOutlined { get { return GetFullAssetPath("ui_header_outlined.png"); } }
            public static string HeaderSquare { get { return GetFullAssetPath("ui_header_square.png"); } }
            public static string HeaderSlantedLeft { get { return GetFullAssetPath("ui_header_slanted_left.png"); } }
            public static string HeaderSlantedRight { get { return GetFullAssetPath("ui_header_slanted_right.png"); } }
            public static string FillBarBackground { get { return GetFullAssetPath("ui_fillbar_bg.png"); } }
            public static string FillBar { get { return GetFullAssetPath("ui_fillbar.png"); } }
            public static string FillBarRed { get { return GetFullAssetPath("ui_fillbar_red.png"); } }
            public static string InventoryButtonUp { get { return GetFullAssetPath("ui_inventory_button_up.png"); } }
            public static string InventoryButtonDown { get { return GetFullAssetPath("ui_inventory_button_down.png"); } }
            public static string InventoryButtonOver { get { return GetFullAssetPath("ui_inventory_button_over.png"); } }
            public static string PanelBackground { get { return GetFullAssetPath("ui_panel_bg.png"); } }

            public static string IconResources { get { return GetFullAssetPath("ui_icon_resources.png"); } }
            public static string IconWarning { get { return GetFullAssetPath("ui_icon_warning.png"); } }
            public static string NamePlateBackground { get { return GetFullAssetPath("ui_nameplate_bg.png"); } }
            public static string NamePlateArrow { get { return GetFullAssetPath("ui_nameplate_arrow.png"); } }

            public static string ButtonRound { get { return GetFullAssetPath("ui_button_round.png"); } }
            public static string ButtonSquare { get { return GetFullAssetPath("ui_button_square.png"); } }
            public static string ButtonSquareBlue { get { return GetFullAssetPath("ui_button_square_blue.png"); } }
            public static string ButtonCancel { get { return GetFullAssetPath("ui_button_cancel.png"); } }
            public static string ButtonUpgrade { get { return GetFullAssetPath("ui_button_upgrade.png"); } }

            public static string GetFullAssetPath(string name)
            {
                return string.Concat(Application.DataPath, folder, name);
            }
        }

        public static class Colors
        {
            public static Color HudStrip { get; } = new Color(0.2f, 0.2f, 0.2f, 0.9f);
            public static Color LightGrey { get; } = new Color(0.886f, 0.843f, 0.843f);
            public static Color MidGrey { get; } = new Color(0.745f, 0.745f, 0.745f);
            public static Color DarkGrey { get; } = new Color(0.122f, 0.122f, 0.122f);
            public static Color Red { get; } = new Color(0.976f, 0.275f, 0.329f);
            public static Color Green { get; } = new Color(0.286f, 0.761f, 0.518f);
            public static Color Blue { get; } = new Color(0.192f, 0.612f, 0.851f);
        }

        public static class TextStyles
        {
            /// <summary>
            /// Dropshadows Off, Fontstyle Bold, Height 14
            /// </summary>
            public static readonly TextStyle Default = new TextStyle()
            {
                DropShadows = false,
                FontStyle = System.Drawing.FontStyle.Bold,
                Height = 14,
            };

            /// <summary>
            /// Dropshadows Off, Fontstyle Bold, Height 16
            /// </summary>
            public static readonly TextStyle HudProperty = new TextStyle()
            {
                DropShadows = false,
                FontStyle = System.Drawing.FontStyle.Bold,
                Height = 16,
                Color = Color.White,
            };

            public static readonly TextStyle HudValue = new TextStyle()
            {
                DropShadows = false,
                FontStyle = System.Drawing.FontStyle.Bold,
                Height = 16,
                Color = Colors.LightGrey,
            };

            public static readonly TextStyle HudTimer = new TextStyle()
            {
                DropShadows = false,
                FontStyle = System.Drawing.FontStyle.Bold,
                Height = 18,
                Color = Colors.LightGrey,
            };

            public static readonly TextStyle HudMessageInfo = new TextStyle()
            {
                DropShadows = false,
                FontStyle = System.Drawing.FontStyle.Bold,
                Height = 14,
                Color = Colors.LightGrey,
            };

            public static readonly TextStyle HudMessageWarning = new TextStyle()
            {
                DropShadows = false,
                FontStyle = System.Drawing.FontStyle.Bold,
                Height = 14,
                Color = Colors.Red,
            };

            public static readonly TextStyle HudWorldSpaceText = new TextStyle()
            {
                DropShadows = true,
                FontStyle = System.Drawing.FontStyle.Bold,
                Height = 20,
            };
        }

        public static class Strings
        {
            public static string StartGame { get; } = "Start Game";
            public static string Quit { get; } = "Quit";
            public static string Resources { get; } = "Resources";
            public static string BaseHealth { get; } = "Base Health";
            public static string Enemies { get; } = "Enemies";
            public static string Wave { get; } = "Wave";
            public static string BuildNotEnoughResourcesToUpgrade { get; } = "You do not have enough resources to upgrade that unit...";
            public static string BuildNotEnoughResources { get; } = "You do not have enough resources to build that unit...";
            public static string BuildSuccessful { get; } = "Successfully placed unit!";
            public static string BuildUpgradeSuccesful { get; } = "Successfully upgraded unit!";
        }

        public static class Levels
        {
            public static string Main { get; } = "Main";
            public static string GridTest { get; } = "GridTest";
            public static string GridTestGenerated { get; } = "GridTestGenerated";
            public static string Empty { get; } = "Empty";
        }

        public static class Meshes
        {
            public static string Hexagon { get; } = "objects/geometry/geom_hex.cgf";
            public static string BasePlate { get; } = "objects/geometry/geom_base_plate.cgf";
            public static string UnitPlate { get; } = "objects/geometry/geom_unit_plate.cgf";
            public static string Diamond { get; } = "objects/geometry/geom_base_core.cgf";
            public static string Enemy { get; } = "objects/geometry/geom_enemy.cgf";
            public static string Tower { get; } = "objects/geometry/geom_tower.cgf";
        }

        public static class Materials
        {
            static string rootPath = "materials/";
            public static string GreyMin { get { return GetMaterialPath("matref_grey_min.mtl"); } }
            public static string Yellow { get { return GetMaterialPath("yellow.mtl"); } }
            public static string Red { get { return GetMaterialPath("red.mtl"); } }
            public static string Green { get { return GetMaterialPath("green.mtl"); } }
            public static string Ghost { get { return GetMaterialPath("ghost.mtl"); } }
            public static string White { get { return GetMaterialPath("white.mtl"); } }
            public static string Purple { get { return GetMaterialPath("purple.mtl"); } }
            public static string BasePlate { get { return "objects/geometry/geom_base_plate.mtl"; } }
            public static string BaseCore { get { return "objects/geometry/geom_base_core.mtl"; } }

            static string GetMaterialPath(string material)
            {
                return string.Concat(rootPath, material);
            }

            public static IMaterial GetMaterial(string path)
            {
                return Global.gEnv.p3DEngine.GetMaterialManager().LoadMaterial(path);
            }
        }

        static DataWrapper dataWrapper;
        public static DataWrapper Data
        {
            get
            {
                if (dataWrapper == null)
                    dataWrapper = new DataWrapper();
                return dataWrapper;
            }
        }

        public class DataWrapper
        {
            public readonly EnemiesWrapper Enemies = new EnemiesWrapper();
            public readonly TowersWrapper Towers = new TowersWrapper();

            public WaveData DefaultWaveData
            {
                get
                {
                    var path = Application.DataPath + "waves.json";
                    if (File.Exists(path))
                    {
                        var file = File.OpenText(Application.DataPath + "waves.json");
                        var data = CryEngine.Tools.FromJSON<WaveData>(file.ReadToEnd());
                        file.Close();
                        return data;
                    }

                    Log.Warning("Failed to find default Wave JSON...");
                    return new WaveData();
                }
            }

            public class EnemiesWrapper
            {
                public readonly UnitDataEnemyEasy Easy = new UnitDataEnemyEasy();
                public readonly UnitDataEnemyMedium Medium = new UnitDataEnemyMedium();
                public readonly UnitDataEnemyHard Hard = new UnitDataEnemyHard();
            }

            public class TowersWrapper
            {
                public List<UnitTowerData> Register
                {
                    get
                    {
                        var register = new List<UnitTowerData>();
                        register.Add(TowerBasic);
                        register.Add(TowerPoison);
                        return register;
                    }
                }

                public readonly UnitTowerBasic TowerBasic = new UnitTowerBasic();
                public readonly UnitTowerPoison TowerPoison = new UnitTowerPoison();
            }
        }
    }
}