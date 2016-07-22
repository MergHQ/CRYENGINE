using System;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Framework;
using CryEngine.Towerdefense;

namespace CryEngine.Towerdefense
{
    public class GridTest : SceneObject
    {
        public WorldMouse WorldMouse { get; private set; }
        public UiHud Hud { get; private set; }

        public void OnAwake()
        {
            LevelSystem.LoadingComplete += OnLevelLoadingComplete;

            var camera = Root.AddComponent<Camera>();
            Camera.Current = camera;

            Mouse.ShowCursor();

            Env.Console.ExecuteString("map gridtest");
        }

        public void OnUpdate()
        {
            Camera.Current.Position = new Vec3(8f, -8f, 5f);
        }

        void OnLevelLoadingComplete(EventArgs<ILevelInfo> arg)
        {
//            Debug.Log("Level load complete");

            // Test HUD
            Hud = SceneObject.Instantiate<UiHud>(Root);

            var resources = new Resources();
            resources.OnResourcesChanged += OnResourcesChanged;

            // Create object placer
            var host = GameObject.Instantiate<GameObject>();
            host.AddComponent<Inventory>().Setup(resources);

            // Display gameobject counter
            SceneObject.Instantiate<UiDebug>(Parent);

            // Player mouse interaction
            WorldMouse = new WorldMouse();

            // Create grid
            new Grid(16, 16, 2);

            LevelSystem.LoadingComplete -= OnLevelLoadingComplete;
        }

        void OnResourcesChanged(int value)
        {
            Hud.Resources.Value = value;
        }
    }
}

