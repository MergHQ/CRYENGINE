using CryEngine.Components;
using CryEngine.EntitySystem;

namespace CryEngine.LevelSuite
{
    public class EntityPicker
    {
        public event EventHandler<BaseEntity> Picked;
        public event EventHandler Destroyed;

        public EntityPicker()
        {
            Mouse.ShowCursor();

            // Assign a callback so we know when the left mouse button was pressed.
            Mouse.OnLeftButtonDown += Mouse_OnLeftButtonDown;

            // Create a host for the camera
            Camera.Current = SceneObject.Instantiate(null).AddComponent<Camera>();
        }

        void Mouse_OnLeftButtonDown(int x, int y)
        {
            // Get the instance of the entity and display it's information on the UI.
            var entity = Mouse.HitBaseEntity;

            if (entity != null)
            {
                Log.Info<EntityPicker>(GetEntityInfo(entity));
                Picked?.Invoke(entity);
            }
        }

        public static string GetEntityInfo(BaseEntity entity)
        {
            return string.Format("Mouse selected Entity '{0}' of Type {1} with ID {2}", entity.Name, entity.GetType(), entity.Id);
        }

        public void Destroy()
        {
            Mouse.OnLeftButtonDown -= Mouse_OnLeftButtonDown;
            Destroyed?.Invoke();
        }
    }
}