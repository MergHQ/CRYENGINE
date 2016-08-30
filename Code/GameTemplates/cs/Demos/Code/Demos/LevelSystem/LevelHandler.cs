using CryEngine.Common;
using CryEngine.UI;
using CryEngine.EntitySystem;

namespace CryEngine.LevelSuite
{
    public class LevelHandler
    {
        public string MapName { get; private set; }

        protected Canvas Canvas { get; private set; }
        protected LevelUI UI { get; private set; }
        protected EntityPicker EntityPicker { get; private set; }

        protected virtual bool ShowCursor { get; }
        protected virtual string Header { get; }

        public LevelHandler()
        {

        }

        public void Initialize(string mapName, Canvas canvas)
        {
            MapName = mapName;
            Canvas = canvas;
        }

        public virtual void Start()
        {
            if (ShowCursor)
            {
                Mouse.ShowCursor();
            }
            else
            {
                Mouse.HideCursor();
            }

            EntityPicker = new EntityPicker();
            EntityPicker.Picked += OnEntitySelected;

            UI = SceneObject.Instantiate<LevelUI>(Canvas);
            UI.Header = Header;
            UI.AddDivider();

            OnStart();
        }

        public virtual void OnLeftMouseButtonDown(int x, int y) { }
        public virtual void OnInputKey(SInputEvent arg) { }
        public virtual void OnUpdate() { }

        public virtual void Cleanup()
        {
            UI.Destroy();
            OnCleanup();
        }

        public void ToggleUI()
        {
            UI.Active = !UI.Active;
        }

        protected virtual void OnStart() { }
        protected virtual void OnEntitySelected(BaseEntity entity) { }

        protected T GetEntity<T>() where T : BaseEntity
        {
            T entity = EntityFramework.GetEntity<T>();
            if (entity == null)
                Log.Error("[{0}] Failed to find entity of type '{1}'", GetType(), typeof(T));
            return entity;
        }

        protected virtual void OnCleanup()
        {
            EntityPicker.Destroy();
        }
    }
}
