using System;
using CryEngine.Common;

namespace CryEngine
{
    public class PhysicsComponent : GameComponent
    {
        /// <summary>
        /// Returns the Native Entity's physical entity.
        /// </summary>
        public IPhysicalEntity PhysicalEntity { get { return GameObject.NativeEntity.GetPhysics(); } }

        protected override void OnInitialize()
        {
            Physicalize();
        }

        public void Physicalize(int mass, EPhysicalizationType type)
        {
            Physicalize(mass, -1, type);
        }

        public void Physicalize(int mass, int density, EPhysicalizationType type)
        {
            var physParams = new SEntityPhysicalizeParams();
            physParams.mass = mass;
            physParams.density = density;
            physParams.type = (int)type;

            Physicalize(physParams);
        }

        public void Physicalize(SEntityPhysicalizeParams phys)
        {
            GameObject.NativeEntity.Physicalize(phys);
        }

        /// <summary>
        /// Physicalize the Entity using default values.
        /// </summary>
        public void Physicalize()
        {
            Physicalize(-1, -1, EPhysicalizationType.ePT_Rigid);
        }

        /// <summary>
        /// Adds and executes a physics action of type T.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="setParams">Sets the parameters of the action to be performed.</param>
        public void Action<T>(Action<T> setParams) where T : pe_action
        {
            var actionParams = Activator.CreateInstance<T>();
            setParams(actionParams);
            PhysicalEntity.Action(actionParams);
        }
    }
}
