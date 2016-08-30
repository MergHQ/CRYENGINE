using System;
using System.Linq;
using CryEngine.EntitySystem;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class ManagedEntityTest
    {
        public ManagedEntityTest()
        {
            // Show message after 5 seconds to avoid initial console log spam.
            new Timer(5f, TestMultipleEntities);
        }

        void TestSingleEntity()
        {
            var managedEntityInstance = EntityFramework.GetEntity<ManagedEntityExample>();

            if (managedEntityInstance != null)
            {
                Log.Info("ManagedEntityExample: Value of 'ExposedInteger' is " + managedEntityInstance.ExposedInteger.ToString());
            }
            else
            {
                Log.Warning("ManagedEntityExample: No entity of type 'ManagedEntityExample' exists");
            }
        }

        void TestMultipleEntities()
        {
            var entities = EntityFramework.GetEntities<ManagedEntityExample>();

            if (entities.Count != 0)
            {
                Log.Info("ManagedEntityExample ({0}): Values are {1}", entities.Count, string.Join(", ", entities.Select(x => x.ExposedInteger).ToArray()));
            }
            else
            {
                Log.Warning("ManagedEntityExample: No entities of type 'ManagedEntityExample' exist.");
            }
        }
    }
}

