using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using CryEngine.Common;
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

			OnInitialize();
        }
        #endregion

        #region Entity Event Methods

		protected virtual void OnTransformChanged() { }

		protected virtual void OnInitialize() { }

		protected virtual void OnUpdate(float frameTime) { }

		protected virtual void OnEditorUpdate(float frameTime) { }

		protected virtual void OnEditorGameModeChange(bool enterGame) { }

		protected virtual void OnRemove() { }

		protected virtual void OnHide() { }

		protected virtual void OnUnhide() { }

		protected virtual void OnGameplayStart() { }

		protected virtual void OnCollision(CollisionEvent collisionEvent) { }

		private void OnCollisionInternal(IntPtr sourceEntityPhysics, IntPtr targetEntityPhysics)
		{
			var collisionEvent = new CollisionEvent();
            collisionEvent.Source = new PhysicsObject(new IPhysicalEntity(sourceEntityPhysics, false));
            collisionEvent.Target = new PhysicsObject(new IPhysicalEntity(targetEntityPhysics, false));
            OnCollision(collisionEvent);
        }

		protected virtual void OnPrePhysicsUpdate(float frameTime) { }
        #endregion

        #region Statics
		private static string TypeToHash(Type type)
		{
			string result = string.Empty;
			string input = type.FullName;
			using(SHA384 hashGenerator = SHA384.Create())
			{
				var hash = hashGenerator.ComputeHash(Encoding.Default.GetBytes(input));
				var shortHash = new byte[16];
				for(int i = 0, j = 0; i < hash.Length; ++i, ++j)
				{
					if(j >= shortHash.Length)
					{
						j = 0;
					}
					unchecked
					{
						shortHash[j] += hash[i];
					}
				}
				result = BitConverter.ToString(shortHash);
				result = result.Replace("-", string.Empty);
			}
			return result;
		}

        /// <summary>
        /// Register the given entity prototype class. 
        /// </summary>
        /// <param name="entityComponentType">Entity class prototype.</param>
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
			var guidString = TypeToHash(entityComponentType);
			var half = (int)(guidString.Length / 2.0f);
			ulong hipart = 0;
			ulong lopart = 0;
			if(!ulong.TryParse(guidString.Substring(0, half), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out hipart))
			{
				Log.Error("Failed to parse {0} to UInt64", guidString.Substring(0, half));
			}
			if(!ulong.TryParse(guidString.Substring(half, guidString.Length - half), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out lopart))
			{
				Log.Error("Failed to parse {0} to UInt64", guidString.Substring(half, guidString.Length - half));
			}
            
			NativeInternals.Entity.RegisterComponent(entityComponentType, hipart, lopart);

            // Register all properties
            var properties = entityComponentType.GetProperties(BindingFlags.Public | BindingFlags.Instance | BindingFlags.GetProperty | BindingFlags.SetProperty);
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
