using System;
using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Resources;
using CryEngine.Towerdefense.Components;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class UiHud : SceneObject
    {
        public UiHud.ResourcesHud Resources { get; private set; }
        public UiHud.BaseHud Base { get; private set; }
        public UiHud.EnemiesHud Enemies { get; private set; }
        public UiHud.WaveHud Wave { get; private set; }

        public static Canvas Canvas { get; private set; }

        public void OnAwake()
        {
            Canvas = SceneObject.Instantiate<Canvas>(this);

            var strip = SceneObject.Instantiate<Panel>(Canvas);
            strip.Background.Color = AssetLibrary.Colors.HudStrip;
            strip.RectTransform.Size = new Point(Canvas.RectTransform.Width, 60f);
            strip.RectTransform.Alignment = Alignment.Top;
            strip.RectTransform.Pivot = new Point(0.5f, 1f);

            var container = SceneObject.Instantiate<Panel>(strip);
            container.RectTransform.Size = new Point(860f, 40f);
            container.RectTransform.Alignment = Alignment.Center;

            // Resources
            Resources = SceneObject.Instantiate<ResourcesHud>(container);
            Resources.RectTransform.Alignment = Alignment.Left;
            Resources.IconUrl = AssetLibrary.UI.IconResources;
            Resources.Label = AssetLibrary.Strings.Resources.ToUpper();
            Resources.Value = 0;

            // Base Info
            Base = SceneObject.Instantiate<BaseHud>(container);

            // Enemies
            Enemies = SceneObject.Instantiate<EnemiesHud>(container);
            Enemies.RectTransform.Alignment = Alignment.Right;
            Enemies.IconUrl = AssetLibrary.UI.IconResources;
            Enemies.Label = AssetLibrary.Strings.Enemies.ToUpper();
            Enemies.EnemiesAlive = 0;
            Enemies.EnemiesTotal = 0;

            // Wave
            Wave = SceneObject.Instantiate<WaveHud>(Canvas);
            Wave.Time = 90f;
        }

        public class WaveHud : Panel
        {
            Text label;
            Text timer;
            int currentWave;
            int totalWaves;

            public int CurrentWave
            {
                set
                {
                    currentWave = value;
                    UpdateWaveInfo();
                }
            }

            public int TotalWaves
            {
                set
                {
                    totalWaves = value;
                    UpdateWaveInfo();
                }
            }

            public float Time
            {
                set
                {
                    var timeSpan = TimeSpan.FromSeconds(value);
                    timer.Content = string.Format("{0}:{1}", timeSpan.Minutes, timeSpan.Seconds);
                }
            }

            public override void OnAwake()
            {
                base.OnAwake();

                Background.SliceType = SliceType.Nine;
                Background.Color = AssetLibrary.Colors.HudStrip;
                RectTransform.Size = new Point(120f, 30f);
                RectTransform.Alignment = Alignment.Top;
                RectTransform.Padding = new Padding(0f, 90f, 0f, 0f);

                var inner = SceneObject.Instantiate<UIElement>(this);
                inner.RectTransform.Size = new Point(RectTransform.Width - 10f, RectTransform.Height - 5f);
                inner.RectTransform.Alignment = Alignment.Center;

                label = inner.AddComponent<Text>();
                label.ApplyStyle(AssetLibrary.TextStyles.HudProperty);
                label.Alignment = Alignment.Center;
                label.Color = Color.White;

                timer = inner.AddComponent<Text>();
                timer.ApplyStyle(AssetLibrary.TextStyles.HudTimer);
                timer.Alignment = Alignment.Bottom;
                timer.Color = AssetLibrary.Colors.LightGrey;
                timer.Active = false;
            }

            void UpdateWaveInfo()
            {
                label.Content = string.Format("{0} {1} / {2}", AssetLibrary.Strings.Wave.ToUpper(), currentWave, totalWaves);
            }
        }

        public abstract class SideHud : Panel
        {
            UIElement content;
            UIElement iconContainer;
            Image icon;

            protected UIElement Properties { get; set; }
            protected Text PropertyName { get; set; }
            protected Text PropertyLabel { get; set; }
            public string Label { set { PropertyName.Content = value; } }
            public string IconUrl { set { icon.Source = ResourceManager.ImageFromFile(value); } }

            public override void OnAwake()
            {
                base.OnAwake();

                RectTransform.LayoutChanged += OnLayoutChanged;

                // Create parent container
                RectTransform.Size = new Point(170f, (Parent as UIElement).RectTransform.Height);

                Background.SliceType = SliceType.ThreeHorizontal;
                //                container.Background.Source = ResourceManager.ImageFromFile(alignment == Alignment.Left ? AssetLibrary.UI.HeaderSlantedLeft : AssetLibrary.UI.HeaderSlantedRight);

                // Create inner container
                content = SceneObject.Instantiate<UIElement>(this);
                content.RectTransform.Size = new Point(RectTransform.Width - 60f, RectTransform.Height - 10f);

                // Properties that appear on either the left or right
                Properties = SceneObject.Instantiate<UIElement>(content);
                Properties.RectTransform.Size = new Point(content.RectTransform.Width, content.RectTransform.Height);

                PropertyName = Properties.AddComponent<Text>();
                PropertyName.ApplyStyle(AssetLibrary.TextStyles.HudProperty);

                PropertyLabel = Properties.AddComponent<Text>();
                PropertyLabel.ApplyStyle(AssetLibrary.TextStyles.HudValue);

                // Icon
                iconContainer = SceneObject.Instantiate<UIElement>(content);
                iconContainer.RectTransform.Size = new Point(20f, 20f);

                icon = iconContainer.AddComponent<Image>();

                // Set default values
                PropertyName.Content = "LABEL";
                PropertyLabel.Content = "0000";
            }

            void OnLayoutChanged()
            {
                // The distance from either the right or left side of this UI element
                const float padding = 20f;

                // The gap between the icon and the list of properties
                const float propertySpacer = 2f;

                // Inverse the alignment. For example, if the parent is aligned left, this should be aligned right
                Alignment inverseAlignment = RectTransform.Alignment == Alignment.Left ? Alignment.Right : Alignment.Left;

                RectTransform.Pivot = RectTransform.Alignment == Alignment.Left ? new Point(1f, 0.5f) : new Point(0f, 0.5f);

                content.RectTransform.Alignment = inverseAlignment;
                content.RectTransform.Pivot = RectTransform.Alignment == Alignment.Left ? new Point(0f, 0.5f) : new Point(1f, 0.5f);
                content.RectTransform.Padding = RectTransform.Alignment == Alignment.Left ? new Padding(-padding, 0f) : new Padding(padding, 0f);

                Properties.RectTransform.Alignment = inverseAlignment;
                Properties.RectTransform.Pivot = RectTransform.Alignment == Alignment.Left ? new Point(0f, 0.5f) : new Point(1f, 0.5f);
                Properties.RectTransform.Padding = RectTransform.Alignment == Alignment.Left ? new Padding(-(padding + propertySpacer), 0f) : new Padding(padding + propertySpacer, 0f);

                PropertyName.Alignment = RectTransform.Alignment == Alignment.Left ? Alignment.TopRight : Alignment.TopLeft;

                PropertyLabel.Alignment = RectTransform.Alignment == Alignment.Left ? Alignment.BottomRight : Alignment.BottomLeft;

                iconContainer.RectTransform.Alignment = RectTransform.Alignment == Alignment.Left ? Alignment.Right : Alignment.Left;
                iconContainer.RectTransform.Pivot = RectTransform.Alignment == Alignment.Left ? new Point(0f, 0.5f) : new Point(1f, 0.5f);
            }
        }

        public class ResourcesHud : SideHud
        {
            Func<int, string> formatter;
            int propertyValue;

            public Func<int, string> Formatter { set { formatter = value; } }

            public int Value
            {
                set
                {
                    var sign = value > propertyValue ? 1 : -1;
                    var difference = Math.Abs(propertyValue - value);

                    propertyValue = value;
                    PropertyLabel.Content = formatter(value);

                    var marker = SceneObject.Instantiate<UiMoveableText>(Properties);
                    marker.RectTransform.Size = new Point(50f, 30f);
                    marker.RectTransform.Alignment = Alignment.BottomRight;
                    marker.RectTransform.Pivot = new Point(0f, 0.7f);
                    marker.Label.Alignment = Alignment.Right;
                    marker.Label.Color = difference * sign < 0f ? Color.Red : Color.Green;
                    marker.Label.Content = difference * sign < 0f ? "-" + difference.ToString() : "+" + difference.ToString();
                    marker.TimeToLive = 4f;
                    marker.Move(40f, Vec2.Down, 2f);
                }
            }

            public override void OnAwake()
            {
                base.OnAwake();

                formatter = new Func<int, string>(x => x.ToString());
            }
        }

        public class EnemiesHud : SideHud
        {
            Func<int, int, string> formatter;
            int enemiesAlive;
            int enemiesTotal;

            /// <summary>
            /// Formats currently specific values. Int 1 represents EnemiesAlive. Int 2 represents EnemiesTotal.
            /// </summary>
            /// <value>The formatter.</value>
            public Func<int, int, string> Formatter { set { formatter = value; } }

            public int EnemiesAlive
            {
                set
                {
                    enemiesAlive = value;
                    PropertyLabel.Content = formatter(value, enemiesTotal);
                }
            }

            public int EnemiesTotal
            {
                set
                {
                    enemiesTotal = value;
                    PropertyLabel.Content = formatter(enemiesAlive, value);
                }
            }

            public override void OnAwake()
            {
                base.OnAwake();

                formatter = new Func<int, int, string>((x, y) => string.Format("{0} / {1}", x.ToString("00"), y.ToString("00")));
            }
        }

        public class BaseHud : Panel
        {
            UiImageFader fader;
            Text healthText;
            FillBar fillBar;

            /// <summary>
            /// Controls the threshold at which the healthbar will turn red.
            /// </summary>
            float healthWarningThreshold = 0.25f;

            /// <summary>
            /// Controls if the healthbar should flash when the Health value is changed.
            /// </summary>
            /// <value><c>true</c> if enable health bar flash; otherwise, <c>false</c>.</value>
            public bool EnableHealthBarFlash
            {
                set
                {
                    fader.Active = value;
                }
            }

            public Health Health
            {
                set
                {
                    var health = MathEx.Clamp(value.CurrentHealth, 0f, value.MaxHealth);
                    healthText.Content = string.Format("{0} / {1}", health, value.MaxHealth);
                    fillBar.Fill = (float)value.CurrentHealth / value.MaxHealth;

                    // Set the fillbar to red if health is below the threshold.
                    fillBar.FillImage.Color = health <= value.MaxHealth * healthWarningThreshold ? AssetLibrary.Colors.Red : Color.White;

                    if (fader.Active)
                        fader.FadeInOut(2f);
                }
            }

            public override void OnAwake()
            {
                base.OnAwake();

                RectTransform.Size = new Point(540f, (Parent as UIElement).RectTransform.Height);
                RectTransform.Alignment = Alignment.Center;
                Background.SliceType = SliceType.ThreeHorizontal;
                //                container.Background.Source = ResourceManager.ImageFromFile(AssetLibrary.UI.Header);

                var content = SceneObject.Instantiate<UIElement>(this);
                content.RectTransform.Alignment = Alignment.Center;
                content.RectTransform.Size = new Point(RectTransform.Width - 60f, RectTransform.Height - 10f);

                fillBar = SceneObject.Instantiate<FillBar>(this);
                fillBar.RectTransform.Size = new Point(RectTransform.Width - 60f, 10f);
                fillBar.Background.Color = Color.Black;
                fillBar.FillImage.Color = Color.White;
                fillBar.Height = 6;
                fillBar.RectTransform.Alignment = Alignment.BottomLeft;
                fillBar.RectTransform.Padding = new Padding(30f, -10f);
                fillBar.RectTransform.Pivot = new Point(1f, 0f);

                var label = content.AddComponent<Text>();
                label.ApplyStyle(AssetLibrary.TextStyles.HudProperty);
                label.Alignment = Alignment.TopLeft;
                label.Content = AssetLibrary.Strings.BaseHealth.ToUpper(); // TODO: Localisation?
                label.Offset = new Point(-2f, 0f);

                healthText = content.AddComponent<Text>();
                healthText.ApplyStyle(AssetLibrary.TextStyles.HudValue);
                healthText.Alignment = Alignment.TopRight;
                healthText.Offset = new Point(2f, 0f);

                // Assign default values
                healthText.Content = "N/A";

                fader = AddComponent<UiImageFader>();
                fader.Active = false;
                fader.Add(fillBar.FillImage);
            }
        }

        public static void CreateHitMarker(Vec3 position, Action<UiMoveableText> onCreate = null)
        {
            var wsi = SceneObject.Instantiate<UiWorldSpaceElement>(UiHud.Canvas);
            wsi.SetPosition(position);
            wsi.Offset = new Vec2(0, -50f);
            wsi.RectTransform.Size = new Point(200f, 30f);

            var text = SceneObject.Instantiate<UiMoveableText>(wsi);
            text.RectTransform.Alignment = Alignment.Top;
            text.RectTransform.Size = new Point(wsi.RectTransform.Width, wsi.RectTransform.Height);
            text.Label.Alignment = Alignment.Center;

            onCreate?.Invoke(text);

            text.Move(40f, Vec2.Up, 2f);
        }
    }
}

