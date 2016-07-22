using System;

namespace CryEngine.Framework
{
    public class GameObjectTest
    {
        readonly Logger logger;

        #region Test Classes
        public class ComponentBase : GameComponent
        {
            public string Value { get; set; }
        }

        public class ComponentA : ComponentBase { }
        public class ComponentB : ComponentBase { }
        #endregion

        public GameObjectTest(int amount = 2)
        {
            logger = new Logger("GameObjectTest");

            for (int i = 0; i < amount; i++)
            {
                Create();
//                new Timer((float)(i + 1) * amount, Create);
            }

            Log("Object Count", string.Format("Expected {0} GameObjects. Actual Gameobjects: {1}", amount, GameObject.Count), () => GameObject.Count != amount);
        }

        void Create()
        {
            var instance = GameObject.Instantiate();

            CreateMultipleComponents<ComponentBase>(instance, 10);
            GetMultipleComponents<ComponentA>(instance, 0);
            GetMultipleComponents<ComponentB>(instance, 0);
            GetMultipleComponents<ComponentBase>(instance, 10);
            CreateAndRemoveComponent<ComponentA>(instance);

            new Timer(5f, () => OnDestroy(instance));
        }

        void OnDestroy(GameObject instance)
        {
            instance.Destroy();
            var components = instance.GetComponents<GameComponent>();
            Log(string.Format("Found {0} components. Expected 0", components.Count), () => components.Count == 0);
        }

        void CreateMultipleComponents<T>(GameObject gameObject, int amount) where T :GameComponent
        {
            for (int i = 0; i < amount; i++)
            {
                gameObject.AddComponent<T>();
            }
        }

        /// <summary>
        /// Gets multipler components of type T. Fails the number of components does not match the amount specified.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="gameObject"></param>
        /// <param name="amount"></param>
        void GetMultipleComponents<T>(GameObject gameObject, int amount) where T : GameComponent
        {
            var components = gameObject.GetComponents<T>();
            Log(string.Format("Found {0} components of type '{1}'. Expected {2}.", components.Count, typeof(T).Name, amount), () => components.Count == amount);
        }

        /// <summary>
        /// Creates and removes a component of type T. Fails if the component still exists.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="gameObject"></param>
        void CreateAndRemoveComponent<T>(GameObject gameObject) where T : GameComponent
        {
            logger.LogInfo("Creating and removing component of type '{0}'...", typeof(T).Name);
            var component = gameObject.AddComponent<T>();
            component.Remove();
            var get = gameObject.GetComponent<T>();
            Log("CreateAndRemoveComponent", string.Format("Found component of type {0} when it shouldn't exist.", typeof(T).Name), () => get != null);
        }

        void Log(string testName, string onFail, Func<bool> failCriteria)
        {
            if (failCriteria())
            {
                logger.LogError(onFail);
            }
            else
            {
                logger.LogInfo("Passed " + testName);
            }
        }

        void Log(string message, Func<bool> comparison)
        {
            if (comparison() == true)
            {
                logger.LogInfo(message);
            }
            else
            {
                logger.LogError(message);
            }
        }
    }
}
