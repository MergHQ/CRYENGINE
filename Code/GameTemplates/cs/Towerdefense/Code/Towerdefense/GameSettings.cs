using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public static class GameSettings
    {
        public static int InitialResources
        {
            get
            {
                var token = GameTokens.GetToken("Settings.InitialResources");
                return token != null ? token.AsInt() : 0;
            }
        }

        public static float CameraFoV
        {
            get
            {
                var token = GameTokens.GetToken("Settings.CameraFoV");
                return token != null ? token.AsFloat() : 0;
            }
        }
    }
}

