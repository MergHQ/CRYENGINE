using CryEngine.Common;

using System;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;

using CryEngine.EntitySystem;

namespace CryEngine
{
	public abstract class EntityComponent
	{
        internal static Dictionary<Type, string> _componentClassMap = new Dictionary<Type, string>();

		public Entity Entity { get; private set; }

        #region Functions
        internal void Initialize(IntPtr entityHandle, uint id)
        {
            Entity = new Entity(new IEntity(entityHandle, false), id); 
        }
        #endregion

        #region Entity Event Methods
        public virtual void OnTransformChanged() { }

        public virtual void OnUpdate(float frameTime) { }
        public virtual void OnEditorGameModeChange(bool enterGame) { }
        public virtual void OnUnhide() { }

        public virtual void OnCollision(CollisionEvent collisionEvent) { }
        private void OnCollisionInternal(IntPtr sourceEntityPhysics, IntPtr targetEntityPhysics)
        {
            var collisionEvent = new CollisionEvent();
            collisionEvent.Source = new PhysicsObject(new IPhysicalEntity(sourceEntityPhysics, false));
            collisionEvent.Target = new PhysicsObject(new IPhysicalEntity(targetEntityPhysics, false));
            OnCollision(collisionEvent);
        }

        public virtual void OnPrePhysicsUpdate(float frameTime) { }
        #endregion

        #region Statics
        /// <summary>
        /// Register the given entity prototype class. 
        /// </summary>
        /// <param name="entityType">Entity class prototype.</param>
        internal static void TryRegister(Type entityComponentType)
        {
            if (!typeof(EntityComponent).IsAssignableFrom(entityComponentType) || entityComponentType.IsAbstract)
                return;

            var playerAttribute = (PlayerEntityAttribute)entityComponentType.GetCustomAttributes(typeof(PlayerEntityAttribute), true).FirstOrDefault();
            if (playerAttribute != null)
            {
                Global.gEnv.pMonoRuntime.RegisterManagedActor(entityComponentType.Name);
            }

            // Start with registering a component
            
            var guidString = entityComponentType.GUID.ToString("N");

            var hipart = UInt64.Parse(guidString.Substring(0, 16));
            var lopart = UInt64.Parse(guidString.Substring(16, 16));

            CryEngine.NativeInternals.Entity.RegisterComponent(entityComponentType, hipart, lopart);

            // Register all properties
            PropertyInfo[] properties = entityComponentType.GetProperties(BindingFlags.Public | BindingFlags.Instance | BindingFlags.GetProperty | BindingFlags.SetProperty);
            foreach (PropertyInfo propertyInfo in properties)
            {
                var attribute = (EntityPropertyAttribute)propertyInfo.GetCustomAttributes(typeof(EntityPropertyAttribute), true).FirstOrDefault();
                if (attribute == null)
                    continue;

                NativeInternals.Entity.RegisterComponentProperty(entityComponentType, propertyInfo, propertyInfo.Name, propertyInfo.Name, attribute.Description, attribute.Type);
            }

            // Check if we should register an entity class
            var entityClassAttribute = (EntityClassAttribute)entityComponentType.GetCustomAttributes(typeof(EntityClassAttribute), true).FirstOrDefault();
            if (entityClassAttribute != null)
            {
                var className = entityClassAttribute.Name.Length > 0 ? entityClassAttribute.Name : entityComponentType.Name;
                _componentClassMap[entityComponentType] = className;

                NativeInternals.Entity.RegisterEntityWithDefaultComponent(className, entityClassAttribute.EditorPath, entityClassAttribute.Helper, entityClassAttribute.Icon, entityClassAttribute.Hide, entityComponentType);
            }
        }
        #endregion
    }
}
