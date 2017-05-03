using System;
using CryEngine.Common;

namespace CryEngine.Animations
{
	//Wraps the native ICharacterInstace class

	/// <summary>
	/// Interface to character animation.
	/// This interface contains methods for manipulating and querying an animated character
	/// instance.
	/// </summary>
	public class Character
	{
		private readonly ICharacterInstance _nativeCharacter;

		private IAttachmentManager _attachmentManager;

		internal ICharacterInstance NativeHandle
		{
			get
			{
				return _nativeCharacter;
			}
		}

		/// <summary>
		/// Gets a value indicating whether this <see cref="T:CryEngine.Animations.Character"/> has vertex animation.
		/// </summary>
		/// <value><c>true</c> if has vertex animation; otherwise, <c>false</c>.</value>
		public bool HasVertexAnimation
		{
			get
			{
				return _nativeCharacter.HasVertexAnimation();
			}
		}

		/// <summary>
		/// Get the filename of the character. This is either the model path or the CDF path.
		/// </summary>
		/// <value>The file path.</value>
		public string FilePath
		{
			get
			{
				return _nativeCharacter.GetFilePath();
			}
		}

		//TODO Wrap ISkeletonAnim in a managed class.
		/// <summary>
		/// Get the animation-skeleton for this instance.
		/// Returns the instance of an ISkeletonAnim derived class applicable for the model.
		/// </summary>
		/// <value>The animation skeleton.</value>
		public ISkeletonAnim AnimationSkeleton
		{
			get
			{
				return _nativeCharacter.GetISkeletonAnim();
			}
		}

		//TODO Wrap ISkeletonPose in a managed class.
		/// <summary>
		/// Get the pose-skeleton for this instance.
		/// Returns the instance of an ISkeletonPose derived class applicable for the model.
		/// </summary>
		/// <value>The pose skeleton.</value>
		public ISkeletonPose PoseSkeleton
		{
			get
			{
				return _nativeCharacter.GetISkeletonPose();
			}
		}

		//TODO Wrap IFacialInstance in a managed class.
		/// <summary>
		/// Get the Facial interface of this Character.
		/// </summary>
		/// <value>The character face.</value>
		public IFacialInstance CharacterFace
		{
			get
			{
				return _nativeCharacter.GetFacialInstance();
			}
		}

		internal Character(ICharacterInstance nativeCharacter)
		{
			if(nativeCharacter == null)
			{
				throw new ArgumentNullException(nameof(nativeCharacter));
			}
			_nativeCharacter = nativeCharacter;
		}

		public CharacterAttachment GetAttachment(string name)
		{
			if(_attachmentManager == null)
			{
				_attachmentManager = _nativeCharacter.GetIAttachmentManager();
			}

			var nativeAttachment = _attachmentManager.GetInterfaceByName(name);
			if(nativeAttachment == null)
			{
				return null;
			}
			return new CharacterAttachment(nativeAttachment);
		}
	}
}
