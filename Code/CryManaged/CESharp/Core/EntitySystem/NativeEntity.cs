
using CryEngine.Common;

namespace CryEngine.EntitySystem
{
	/// <summary>
	/// Represents an entity that does not have a managed representation
	/// </summary>
	internal class NativeEntity : Entity
	{
		public NativeEntity(IEntity handle, EntityId id)
		{
			Id = id;
			NativeHandle = handle;
		}
	}
}
