using System;
using System.Linq;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Framework;


namespace CryEngine.Towerdefense
{
    public class TowerSensor : GameComponent
    {
        public event EventHandler<GameObject> OnTargetChanged;
        public event EventHandler OnNoTargetsFound;

        List<GameObject> potentialTargets = new List<GameObject>();
        bool wereNoTargetsFound;
        GameObject currentTarget;
        float range;

        public float Range { set { range = value; } }
        public List<GameObject> PotentialTargets { set { potentialTargets = value; } }

        public override void OnUpdate()
        {
            // Begin search for a target if none is currently assigned
            if (currentTarget == null)
            {
                // Filter targets down to those that within range
                var targetsInRange = potentialTargets
                    .Where(x => x.Exists)
                    .Where(x => GameObject.Position.GetDistance(x.Position) <= range)
                    .Where(x => Vec3Ex.VisibleOnScreen(x.Position))
                    .ToList();

                if (targetsInRange.Count != 0)
                {
                    // Find the target is closest
                    currentTarget = targetsInRange.OrderBy(x => GameObject.Position.GetDistance(x.Position)).First();

                    OnTargetChanged?.Invoke(currentTarget);
                }
                else
                {
                    if (!wereNoTargetsFound)
                    {
                        OnNoTargetsFound?.Invoke();
                        wereNoTargetsFound = true;
                    }
                }
            }
            else
            {
                // Make sure the currentTarget entity exists before doing anything.
                // Otherwise, start searching for a new target.
                if (currentTarget.Exists)
                {
                    var distance = GameObject.Position.GetDistance(currentTarget.Position);
                    if (distance > range)
                    {
                        Global.gEnv.pLog.LogToConsole("Target fell out of range");
                        currentTarget = null;
                    }
                }
                else
                {
                    currentTarget = null;
                }

                wereNoTargetsFound = false;
            }
        }
    }
}

