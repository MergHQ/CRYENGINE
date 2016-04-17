// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.Runtime.Serialization;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Attributes;

/// <summary>
/// All Classes directly derived by CryEngine.Components.Component.
/// </summary>
namespace CryEngine.Components
{
	/// <summary>
	/// Base class for all modifiers and controllers of SceneObjects. Analyzes all standard functions on initialization and stores them for usage by the framework.
	/// </summary>
	[DataContract]
	public class Component : IUpdateReceiver
	{
		[DataMember(Name="ACT")]
		protected bool _isActive = false;
		[DataMember(Name="ABH")]
		protected bool _isActiveByHierarchy = true;
		bool _isFocusable = false;
		MethodInfo _OnAwake;
		MethodInfo _OnUpdate;
		MethodInfo _OnDestroy;
		MethodInfo _OnEnterFocus;
		MethodInfo _OnLeaveFocus;
		MethodInfo _OnLeftMouseDown;
		MethodInfo _OnLeftMouseUp;
		MethodInfo _OnMouseEnter;
		MethodInfo _OnMouseLeave;
		MethodInfo _OnMouseMove;
		MethodInfo _OnKey;

		[HideFromInspector]
		public bool HasFocus { get; private set; } ///< Determines whether this object is Focused (e.g. through processing by Canvas).
		[HideFromInspector]
		public bool Enabled { get; set; } ///< Determines whether object is individually focusable.
		[HideFromInspector]
		public SceneObject Owner { get; private set; } ///< Owning SceneObject.

		public bool IsFocusable { get { return _isFocusable; } } ///< Determines whether object is generally focusable.
		public SceneObject Root { get { return Owner.Root; } } ///< Root object for this scene tree.
		public Transform Transform { get { return Owner.Transform; } } ///< Transform of this Components owner.
		public bool IsUpdateable { get { return _OnUpdate != null; } } ///< Indicates whether this object can be updated.

		[HideFromInspector]
		public bool Active		
		{
			set 
			{
				_isActive = value;
			}
			get 
			{
				return _isActive;
			}
		} ///< Determines whether this object should be updated.

		[HideFromInspector]
		public bool ActiveByHierarchy
		{
			set 
			{
				_isActiveByHierarchy = value;
			}
			get 
			{
				return _isActiveByHierarchy && Active;
			}
		} ///< Determines whether this object should be updated on a basis of its ancestors activity.

		public void InvokeOnEnterFocus () 
		{ 
			HasFocus = true;
			if (_OnEnterFocus != null) 
				_OnEnterFocus.Invoke (this, null); 
		}

		public void InvokeOnLeaveFocus () 
		{ 
			HasFocus = false;
			if (_OnLeaveFocus != null) 
				_OnLeaveFocus.Invoke (this, null); 
		}

		public void InvokeOnLeftMouseDown (int x, int y) 
		{ 
			if (_OnLeftMouseDown != null) 
				_OnLeftMouseDown.Invoke (this, new object[]{ x, y }); 
		}

		public void InvokeOnLeftMouseUp (int x, int y, bool wasInside) 
		{
			if (_OnLeftMouseUp != null) 
				_OnLeftMouseUp.Invoke (this, new object[]{ x, y, wasInside }); 
		}

		public void InvokeOnMouseEnter (int x, int y) 
		{ 
			if (_OnMouseEnter != null) 
				_OnMouseEnter.Invoke (this, new object[]{ x, y }); 
		}

		public void InvokeOnMouseLeave (int x, int y) 
		{ 
			if (_OnMouseLeave != null) 
				_OnMouseLeave.Invoke (this, new object[]{ x, y }); 
		}

		public void InvokeOnMouseMove (int x, int y) 
		{ 
			if (_OnMouseMove != null) 
				_OnMouseMove.Invoke (this, new object[]{ x, y }); 
		}

		public void InvokeOnKey (SInputEvent e) 
		{ 
			if (_OnKey != null) 
				_OnKey.Invoke (this, new object[]{ e }); 
		}

		protected Component()
		{
			Enabled = true;
		}

		/// <summary>
		/// Creates an instance of T and wires it into the scene hierarchy.
		/// </summary>
		/// <param name="owner">SceneObject to own this Component.</param>
		/// <typeparam name="T">Type of Component to be instanciated.</typeparam>
		public static T Instantiate<T>(SceneObject owner) where T : Component
		{
			return (T)Instantiate (owner, typeof(T));
		}

		/// <summary>
		/// Registers this component at SceneManager.
		/// </summary>
		/// <param name="order">Intended update order for this component.</param>
		public void TryRegisterUpdateReceiver(int order)
		{
			if (_OnUpdate != null) 
				SceneManager.RegisterUpdateReceiver(this, order);
		}

		void InitMethods(Type t)
		{
			_OnAwake = t.GetMethod ("OnAwake");
			_OnUpdate = t.GetMethod ("OnUpdate");
			_OnDestroy = t.GetMethod ("OnDestroy");

			_OnEnterFocus = t.GetMethod ("OnEnterFocus");
			_OnLeaveFocus = t.GetMethod ("OnLeaveFocus");
			_OnLeftMouseDown = GetType().GetMethod ("OnLeftMouseDown");
			_OnLeftMouseUp = GetType().GetMethod ("OnLeftMouseUp");
			_OnMouseEnter = GetType().GetMethod ("OnMouseEnter");
			_OnMouseLeave = GetType().GetMethod ("OnMouseLeave");
			_OnMouseMove = GetType().GetMethod ("OnMouseMove");
			_OnKey = GetType().GetMethod ("OnKey");

			_isFocusable = _OnEnterFocus != null || _OnLeaveFocus != null || _OnLeftMouseDown != null || 
				_OnLeftMouseUp != null || _OnMouseEnter != null || _OnMouseLeave != null;
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public virtual void Update()
		{
			if (ActiveByHierarchy)
				_OnUpdate.InvokeSecure (this);
		}

		/// <summary>
		/// Creates an instance of T and wires it into the scene hierarchy.
		/// </summary>
		/// <param name="owner">SceneObject to own this Component.</param>
		/// <param name="t">Type of Component to be instanciated.</param>
		public static Component Instantiate(SceneObject owner, Type t)
		{
			var instance = Activator.CreateInstance (t) as Component;
			instance.Owner = owner;
			instance.InitMethods (t);

			if (instance._OnAwake != null)
				instance._OnAwake.Invoke (instance, null);
			instance.Active = true;
			return instance;
		}

		/// <summary>
		/// Must be implemented by deriving classes in order to make the  component pickable.
		/// </summary>
		/// <returns><c>True</c>, if Component was hit, <c>false</c> otherwise.</returns>
		/// <param name="x">The x coordinate.</param>
		/// <param name="y">The y coordinate.</param>
		public virtual bool HitTest(int x, int y)
		{
			return false;
		}

		/// <summary>
		/// Removes this Component from its parent and unregisters it as UpdateReceiver. Invokes function OnDestroy.
		/// </summary>
		public void Destroy()
		{
			Owner.Components.Remove (this);
			if (_OnUpdate != null) 
				SceneManager.RemoveUpdateReceiver(this);
			_OnUpdate = null;

			if (_OnDestroy != null)
				_OnDestroy.InvokeSecure (this);
			_OnDestroy = null;
		}

		/// <summary>
		/// Serializes this Component to JSON string.
		/// </summary>
		/// <returns>The JSON description.</returns>
		public string ToJSON()
		{
			return Tools.ToJSON (this);
		}
	}
}
