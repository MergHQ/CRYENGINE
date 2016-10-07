using CryEngine.Common;

namespace CryEngine.EntitySystem
{
	public class EntityComponent
	{
		public event EventHandler<EntityComponent> Removed;

		public Entity Entity { get; private set; }

		internal void Initialize(Entity owner)
		{
			Entity = owner;
			OnInitialize();
		}

		/// <summary>
		/// Removes the component from it's owning GameObject.
		/// </summary>
		public void Remove()
		{
			OnRemove();
			Removed?.Invoke(this);
		}

		public virtual void OnInitialize() { }
		public virtual void OnStart() { }
		public virtual void OnEnd() { }
		public virtual void OnReset() { }
		public virtual void OnEvent(SEntityEvent arg) { }
		public virtual void OnUpdate() { }
		public virtual void OnRemove() { }
	}
}
