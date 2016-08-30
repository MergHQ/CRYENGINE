using CryEngine.Common;
using CryEngine.UI;
using CryEngine.EntitySystem;
using CryEngine.LevelSuite;

namespace CryEngine.SampleApp
{
    public class ProjectileLevel : LevelHandler
    {
        private ProjectileSpawner _projectileSpawner;

        protected override string Header { get { return "Projectile Example"; } }

        protected override void OnStart()
        {
            UI.AddInfo("Left-click anywhere to fire a projectile.");

            _projectileSpawner = EntityFramework.GetEntity<ProjectileSpawner>();
            if (_projectileSpawner == null)
                Log.Error<ProjectileLevel>("Failed to find a ProjectileSpawner in the level");
        }
    }
}
