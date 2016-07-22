using CryEngine.EntitySystem;
using CryEngine.LevelSuite;

namespace CryEngine.SampleApp
{
    public class EntitiesLevel : LevelHandler
    {
        protected override string Header { get { return "Entities Example"; } }

        protected override void OnStart()
        {
            base.OnStart();
            UI.AddInfo("Left-click to destroy an entity.");
        }

        protected override void OnEntitySelected(BaseEntity entity)
        {
            base.OnEntitySelected(entity);
            entity.Remove();
        }
    }
}
