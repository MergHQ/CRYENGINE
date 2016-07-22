using CryEngine.Common;
using CryEngine.UI;
using System;
using System.Collections.Generic;
using System.Linq;

namespace CryEngine.LevelSuite
{
    /// <summary>
    /// Creates and controls a LevelHandler for each level that is added.
    /// </summary>
    public class LevelManager : IGameUpdateReceiver
    {
        private Canvas _canvas;
        private LevelHandler _currentHandler;
        private Dictionary<string, LevelHandler> _levels;
        private const EKeyId _uiToggleKey = EKeyId.eKI_Home;

        public bool Loading { get; private set; }

        public List<string> Maps
        {
            get
            {
                return _levels.Select(x => x.Key).ToList();
            }
        }

        public LevelManager(Canvas canvas)
        {
            _canvas = canvas;
            _levels = new Dictionary<string, LevelHandler>();

            LevelSystem.LoadingComplete += LevelSystem_LoadingComplete;
        }

        public void Add<THandler>(string mapName) where THandler : LevelHandler
        {
            var instance = Activator.CreateInstance<THandler>();
            instance.Initialize(mapName, _canvas);
            _levels.Add(mapName, instance);
        }

        public void LoadLevel(string mapName)
        {
            if (!Env.IsSandbox)
            {
                LoadLevelInternally(mapName);
            }
        }

        public void LoadLevel<THandler>()
        {
            if (!Env.IsSandbox)
            {
                var newLevel = _levels.FirstOrDefault(x => x.Value.GetType() == typeof(THandler));

                if (newLevel.Value == null)
                {
                    Log.Error<LevelManager>("Cannot load level as that level does not exist.");
                }
                else
                {
                    LoadLevelInternally(newLevel.Key);
                }
            }
        }

        public void LoadPreviousLevel()
        {
            var maps = Maps;

            var indexOf = maps.IndexOf(_currentHandler.MapName);

            int i = 0;
            if (indexOf != 0)
            {
                i = indexOf - 1;
            }
            else
            {
                i = maps.Count - 1;
            }

            LoadLevelInternally(maps[i]);
        }

        public void LoadNextLevel()
        {
            var maps = Maps;

            var indexOf = maps.IndexOf(_currentHandler.MapName);

            int i = 0;
            if (indexOf != maps.Count - 1)
            {
                i = indexOf + 1;
            }
            else
            {
                i = 0;
            }

            LoadLevelInternally(maps[i]);
        }

        public void Destroy()
        {
            _currentHandler?.Cleanup();
            GameFramework.UnregisterFromUpdate(this);
            Input.OnKey -= Input_OnKey;
            Mouse.OnLeftButtonDown -= Mouse_OnLeftButtonDown;
        }

        private void LoadLevelInternally(string mapName)
        {
            if (Loading)
                return;

            Loading = true;

            if (_currentHandler != null)
            {
                _currentHandler.Cleanup();

                Input.OnKey -= Input_OnKey;
                Mouse.OnLeftButtonDown -= Mouse_OnLeftButtonDown;
            }

            // Trigger the map change which will then cause the new level's handler to be started inside the LoadingComplete handler.
            Env.Console.ExecuteString("map " + mapName);
        }

        private void LevelSystem_LoadingComplete(EventArgs<ILevelInfo> arg)
        {
            if (arg.Item1 == null)
                return;
            
            var newMapName = arg.Item1.GetName();

            if (_levels.ContainsKey(newMapName))
            {
                // Hack to fix mesh leaks when switching level. (IE invisible designer objects)
                if (!Env.IsSandbox)
                    Env.Console.ExecuteString("e_render 0");

                new Timer(0.1f, () =>
                {
                    if (!Env.IsSandbox)
                        Env.Console.ExecuteString("e_render 1");

                    Input.OnKey += Input_OnKey;
                    Mouse.OnLeftButtonDown += Mouse_OnLeftButtonDown;

                    _currentHandler = _levels[newMapName];

                    Log.Info<LevelManager>("Changing level to '{0}'", _currentHandler.MapName);

                    _currentHandler.Start();

                    Loading = false;
                });
            }
        }

        private void Mouse_OnLeftButtonDown(int x, int y)
        {
            _currentHandler.OnLeftMouseButtonDown(x, y);
        }

        private void Input_OnKey(SInputEvent arg)
        {
            _currentHandler?.OnInputKey(arg);

            if (arg.KeyPressed(_uiToggleKey))
                _currentHandler.ToggleUI();
        }

        void IGameUpdateReceiver.OnUpdate()
        {
            _currentHandler?.OnUpdate();
        }
    }
}
