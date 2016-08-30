using System;
using CryEngine.Common;
using CryEngine.Towerdefense.Components;

namespace CryEngine.Towerdefense
{
    public class EffectSlow : Effect
    {
        UnitMovementController.MovementModifier modifier;
        UnitMovementController movementComp;

        public UnitMovementController.MovementModifier Modifier { set { modifier = value; } }

        protected override void OnInitialize()
        {
            modifier = new UnitMovementController.MovementModifier(0f);
        }

        protected override void OnApplyEffect(GameObject target)
        {
            movementComp = target.GetComponent<UnitMovementController>();
            movementComp?.AddModifier(modifier);
        }

        protected override void OnEndEffect(GameObject target)
        {
            movementComp?.RemoveModifier(modifier);
        }

        protected override void OnSetData(EffectData effect)
        {
            var data = effect as EffectSlowData;

            if (data != null)
                Modifier = new UnitMovementController.MovementModifier(data.SpeedModifier, data.OverridePreviousModifiers);
        }

        protected override void OnDestroy()
        {
            movementComp?.RemoveModifier(modifier);
        }
    }
}

