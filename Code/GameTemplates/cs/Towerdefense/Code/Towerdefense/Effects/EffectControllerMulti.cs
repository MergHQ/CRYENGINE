using System;
using System.Collections.Generic;
using CryEngine.Common;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Controls and applies a CombatEffect to multiple targets. Useful for AoE type effects.
    /// </summary>
    public class EffectControllerMulti : EffectController
    {
        List<EffectController> controllers;

        public EffectControllerMulti()
        {
            controllers = new List<EffectController>();
        }

        public static EffectControllerMulti Create<T>(List<GameObject> targets, Action<T> onAddEffect = null) where T : Effect
        {
            var instance = new EffectControllerMulti();
            instance.AddEffect(targets, onAddEffect);
            return instance;
        }

        public void AddEffect<T>(GameObject target, Action<T> onAddEffect) where T : Effect
        {
            controllers.Add(Create(target, onAddEffect));
        }

        public void AddEffect<T>(List<GameObject> targets, Action<T> onAddEffect) where T : Effect
        {
            targets.ForEach(x => controllers.Add(Create(x, onAddEffect)));
        }

        public override void SetEffectData(EffectData effectData)
        {
            controllers.ForEach(x => x.SetEffectData(effectData));
        }

        public override void SetParams(Action<IEffectControllerParams> onAdd)
        {
            controllers.ForEach(x => x.SetParams(onAdd));
        }

        public override void SetParams(IEffectControllerParams parameters)
        {
            controllers.ForEach(x => x.SetParams(parameters));
        }

        public override void SetSource(Vec3 source)
        {
            controllers.ForEach(x => x.SetSource(source));
        }

        public override void ApplyEffect()
        {
            foreach (var controller in controllers)
                controller.ApplyEffect();

            controllers.Clear();
            controllers = null;
        }
    }
}