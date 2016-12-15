// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Attributes;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;

namespace CryEngine.UI
{
	/// <summary>
	/// Represents location of a SceneObject in space. Holds a SceneObject's hierarchical children.
	/// </summary>
	[DataContract]
	public class Transform : UIComponent, IEnumerator<Transform>
	{
		public event EventHandler ParentChanged; ///< Raised if parent SceneObject was changed.

		Transform _parent = null;
		int _childrenPosition = -1;
		Transform _currentChild = null;

		[HideFromInspector]
		public Transform Parent
		{
			get
			{
				return _parent;
			}
			set
			{
				if (_parent != null)
					_parent.Children.Remove(this);
				_parent = value;
				if (_parent != null)
					_parent.Children.Add(this);
				if (ParentChanged != null)
					ParentChanged();
			}
		} ///< Moves this Transform from one SceneObject to another.

		public List<Transform> Children = new List<Transform>(); ///< Decending SceneObjects.

		[DataMember(Name = "Children")]
		List<SceneObject> _childOwners
		{
			get
			{
				return Children.Select(x => x.Owner).ToList();
			}
			set
			{
				Children = value.Select(x => x.GetComponent<Transform>()).ToList();
				foreach (var ch in value)
					ch.Transform._parent = this;
			}
		}

		/// <summary>
		/// IEnumerator specific.
		/// </summary>
		public IEnumerator<Transform> GetEnumerator()
		{
			Reset();
			return (IEnumerator<Transform>)this;
		}

		/// <summary>
		/// IEnumerator specific.
		/// </summary>
		public bool MoveNext()
		{
			if (++_childrenPosition >= Children.Count)
			{
				return false;
			}
			else
			{
				_currentChild = Children[_childrenPosition];
			}
			return true;
		}

		/// <summary>
		/// IEnumerator specific.
		/// </summary>
		public void Reset()
		{
			_childrenPosition = -1;
		}

		/// <summary>
		/// IEnumerator specific.
		/// </summary>
		void IDisposable.Dispose() { }

		/// <summary>
		/// IEnumerator specific.
		/// </summary>
		public Transform Current { get { return _currentChild; } }
		object IEnumerator.Current { get { return _currentChild; } }

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnDestroy()
		{
			Children.Clear();
		}
	}
}
