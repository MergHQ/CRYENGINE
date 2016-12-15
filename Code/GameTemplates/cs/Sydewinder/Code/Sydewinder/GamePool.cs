// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.EntitySystem;
using System.Collections.Generic;
using System.Linq;

namespace CryEngine.Sydewinder
{
    public static class GamePool
    {
        private static Dictionary<uint, DestroyableBase> _gameObjects = new Dictionary<uint, DestroyableBase>();
        private static List<uint> _flagedForPurge = new List<uint>();
        private static List<DestroyableBase> _deferredInsert = new List<DestroyableBase>();
        private static bool _updatingPool = false;

        public static void AddObjectToPool(DestroyableBase gameObject)
        {
            if (_updatingPool)
                _deferredInsert.Add(gameObject);
            else
            {
				_gameObjects.Add(gameObject.Entity.Id, gameObject);
            }
        }

        /// <summary> 
        /// Flag for destroy to avoid concurrency while.
        /// iterating through game objects
        /// </summary>
        /// <param name="id">Identifier.</param>
        public static void FlagForPurge(uint id)
        {
            _flagedForPurge.Add(id);
        }

        public static void UpdatePool()
        {
            foreach (var item in _deferredInsert)
				_gameObjects.Add(item.Entity.Id, item);
            _deferredInsert.Clear();

            _updatingPool = true;

            // Collect active game objects.
            var removeables = _gameObjects.Values.Where(x => x.IsAlive == false).ToArray();
            for (int i = 0; i < removeables.Length; i++)
				RemoveObjectFromPool(removeables[i].Entity.Id);

            // Move all game objects.
            foreach (DestroyableBase gameObj in _gameObjects.Values)
                gameObj.Move();

			Player.LocalPlayer.Move();

			// Remove not needed objects.
			foreach (uint id in _flagedForPurge)
                RemoveObjectFromPool(id);

            _flagedForPurge.Clear();
            _updatingPool = false;
        }

        /// <summary>
        /// Avoids physicalized position change by setting same position again.
        /// Won't remove or purge objects.
        /// </summary>
        public static void UpdatePoolForPausedGame()
        {
            // Keep position of all game objects
            foreach (DestroyableBase gameObj in _gameObjects.Values)
                gameObj.KeepPosition();

			Player.LocalPlayer.KeepPosition();
        }

        public static void Clear()
        {
            uint[] ids = _gameObjects.Keys.ToArray();
            foreach (var id in ids)
                RemoveObjectFromPool(id);
            _flagedForPurge.Clear();
        }

        private static void RemoveObjectFromPool(uint id)
        {
            _gameObjects.Remove(id);

			Entity.Remove(id);
        }

        public static DestroyableBase GetMoveableByEntityId(uint entityId)
        {
            DestroyableBase movable = null;
            _gameObjects.TryGetValue(entityId, out movable);
            return movable;
        }
    }
}
