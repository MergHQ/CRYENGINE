// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.Components;
using CryEngine.UI;
using CryEngine.LevelSuite;

namespace CryEngine.SampleApp
{
    public class SampleApp : Application
    {
        private LevelManager _levelManager;
        private Canvas _canvas;
        private const string _defaultLevel = ParticlesLevel;
        private const EKeyId _changeLevelKey = EKeyId.eKI_Tab;

        #region Levels
        private const string EntitiesLevel = "EntitiesExample";
        private const string EarthquakeLevel = "EarthquakeExample";
        private const string ParticlesLevel = "ParticlesExample";
        private const string RaycastLevel = "RaycastExample";
        private const string CollisionLevel = "CollisionExample";
        #endregion

        public void OnAwake()
        {
            _canvas = SceneObject.Instantiate<Canvas>(null);
              
            _levelManager = new LevelManager(_canvas);
            _levelManager.Add<EntitiesLevel>(EntitiesLevel);
            _levelManager.Add<ParticlesLevel>(ParticlesLevel);
            _levelManager.Add<EarthquakeLevel>(EarthquakeLevel);
            _levelManager.Add<RaycastLevel>(RaycastLevel);

            var changeLevelPrompt = SceneObject.Instantiate<TextElement>(_canvas);
            changeLevelPrompt.RectTransform.Size = new Point(Screen.Width, 40f);
            changeLevelPrompt.RectTransform.Padding = new Padding(-40f, -20f);
            changeLevelPrompt.RectTransform.Alignment = Alignment.BottomHStretch;
            changeLevelPrompt.Text.Alignment = Alignment.Right;
            changeLevelPrompt.Text.Height = 20;
            changeLevelPrompt.Text.Content = string.Format("Press {0} to change level.", _changeLevelKey.ToString().Substring(4));

            if (!Env.IsSandbox)
            {
                Input.OnKey += Input_OnKey;
                _levelManager.LoadLevel(_defaultLevel);
            }
        }

        public void OnDestroy()
        {
            _canvas.Destroy();

            if (!Env.IsSandbox)
                Input.OnKey -= Input_OnKey;
        }

        private void Input_OnKey(SInputEvent arg)
        {
            if (arg.KeyPressed(EKeyId.eKI_Tab) && _levelManager.Maps.Count != 0)
                _levelManager.LoadNextLevel();

            if (arg.KeyPressed(EKeyId.eKI_Escape))
                Env.Console.ExecuteString("quit");
        }
    }
}
