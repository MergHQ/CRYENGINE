using System;
using System.Linq;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.EntitySystem;


namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Finds entities in the level and creates a cache of objects including the Base, Enemy spawn points, and Construction Points
    /// </summary>
    public class LevelManager : GameComponent
    {
        const string cameraName = "Camera";
        const string playerBaseName = "Base";
        const string constructionPointName = "ConstructionPoint";
        const string spawnPointName = "EnemySpawn";
        const string pathName = "Path";

        List<SpawnPoint> spawnPoints;
        List<ConstructionPointController> constructionPoints;
        List<Unit> units;

        public Base PlayerBase { get; private set; }
        public ReadOnlyCollection<SpawnPoint> SpawnPoints { get { return spawnPoints.AsReadOnly(); } }
        public ReadOnlyCollection<Enemy> Enemies { get { return units.OfType<Enemy>().ToList().AsReadOnly(); } }

        public LevelManager()
        {
            Input.OnKey += (arg) => 
            {
                if (arg.KeyUp(EKeyId.eKI_Space))
                {
                    units.ForEach(x => x.Destroy());   
                        units.Clear();
                }
            };
        }

        /// <summary>
        /// Sets up the level ready for the round to begin.
        /// </summary>
        public void Setup()
        {
            spawnPoints = new List<SpawnPoint>();
            constructionPoints = new List<ConstructionPointController>();
            units = new List<Unit>();

            // Set the camera
            var camera = Env.EntitySystem.FindEntityByName(cameraName);
            if (camera != null)
            {
                var host = SceneObject.Instantiate(null);
                Camera.Current = host.AddComponent<Camera>();
                Camera.Current.OnPlayerEntityAssigned += (arg) =>
                {
                    arg.HostEntity.Position = camera.GetPos();
                    arg.HostEntity.Rotation = camera.GetRotation();
                    Env.Console.ExecuteString("gamezero_cam_fov 45");
                };
            }
            else
            {
                Global.gEnv.pLog.LogError("Level Manager: Camera is missing from the level. Make sure there is a Camera named " + cameraName);
            }

            // Find the Base reference entity
            PlayerBase = EntityFramework.GetEntity<Base>();
            if (PlayerBase != null)
            {
                PlayerBase.Setup();
            }
            else
            {
                Global.gEnv.pLog.LogError("Level Manager: Player Base is missing from the level. Make sure their is an entity named " + playerBaseName);
            }

            // Find construction point reference entities
            EntityFramework.GetEntities<ConstructionPoint>().ForEach(x =>
            {
                // Create construction point
                var cell = new ConstructionPointController(x.NativeEntity.GetPos(), x.NativeEntity.GetRotation(), 4, x);
                constructionPoints.Add(cell);

                // Register callback for a Unit is added or removed. We need a reference so we can update the unit with the position of each enemy.
                cell.OnUnitPlaced += AddUnit;
                cell.OnUnitRemoved += RemoveUnit;
            });

            // Find spawn point reference entities
            var spawnsInLevel = EntityFramework.GetEntities<SpawnPoint>();
            if (spawnsInLevel.Count == 0)
            {
                Log.Warning("Failed to find any spawn points");
            }
            else
            {
                spawnsInLevel.ForEach(x =>
                {
                    // Create spawn point
                    Path path = null;

                    if (PlayerBase != null)
                        path = new Path(x.NativeEntity.GetPos(), PlayerBase.Position);

                    var instance = new SpawnPoint(path);
                    spawnPoints.Add(instance);

                    // Register callback for when an enemy is spawned and despawned
                    instance.OnEnemySpawned += AddUnit;
                    instance.OnEnemyDespawned += RemoveUnit;
                });
            }

            // Create a grid, if the level requires one
            var gridEntity = Env.EntitySystem.FindEntityByName("Grid");
            if (gridEntity != null)
                new Grid(16, 16, 2);
        }

        public override void OnUpdate()
        {
            units.ForEach(x => x.UpdateUnit(this));
        }

        /// <summary>
        /// Remove all instances of level objects.
        /// </summary>
        protected override void OnDestroy()
        {
            PlayerBase?.Destroy();

            constructionPoints.ForEach(x =>
            {
                x.Destroy();
                x.OnUnitPlaced -= AddUnit;
                x.OnUnitRemoved -= RemoveUnit;
            });

            spawnPoints.ForEach(x =>
            {
                x.Cleanup();
                x.OnEnemySpawned -= AddUnit;
                x.OnEnemyDespawned -= RemoveUnit;
            });

            constructionPoints.Clear();
            spawnPoints.Clear();
            units.Clear();
        }

        void AddUnit(Unit unit)
        {
            if (!units.Contains(unit))
                units.Add(unit);
        }

        void RemoveUnit(Unit unit)
        {
            if (units.Contains(unit))
                units.Remove(unit);
        }

        /// <summary>
        /// Hack to loop through all entities in the scene with the specified name and returns them in a list. It searches by looking for entities with a name that matches Name1, Name2, Name3, etc.
        /// </summary>
        /// <returns>The entities by name.</returns>
        /// <param name="name">Name.</param>
        void ReplaceEntitiesWithName(string name, Action<IEntity> onFind)
        {
            int i = 1;
            bool findEntities = true;
            while (findEntities)
            {
                var strName = name + i.ToString();
                var entity = Env.EntitySystem.FindEntityByName(strName);

                if (entity != null)
                {
                    onFind(entity);
                    i++;
                }
                else
                {
                    break;
                }
            }
        }
    }
}