using CryEngine.Common;

namespace CryEngine.Animations
{
	//Wraps the native IAttachment class
	
	public class CharacterAttachment
	{
		private readonly IAttachment _nativeAttachment;

		public Vector3 WorldPosition
		{
			get
			{
				var worldTransform = _nativeAttachment.GetAttWorldAbsolute();
				return worldTransform.t;
			}
		}

		public Quaternion WorldRotation
		{
			get
			{
				var worldTransform = _nativeAttachment.GetAttWorldAbsolute();
				return worldTransform.q;
			}
		}

		internal CharacterAttachment(IAttachment nativeAttachment)
		{
			_nativeAttachment = nativeAttachment;
		}
	}
}
