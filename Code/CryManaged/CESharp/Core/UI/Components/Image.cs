// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using CryEngine.Resources;
using CryEngine.Common;
using System.Runtime.Serialization;
using CryEngine.Attributes;

namespace CryEngine.UI.Components
{
	/// <summary>
	/// View for one image, aligned reltaive to its owning element.
	/// </summary>
	[DataContract]
	public class Image : UIComponent
	{
		public event EventHandler OnSourceChanged; ///< Invoked if the underlying ImageSource was changed.

		Texture _texture = null;
		[DataMember]
		ImageSource _source = null;
		[DataMember]
		SliceType _sliceType = SliceType.None;
		[DataMember]
		Color _color = Color.White;

		public ImageSource Source
		{
			set
			{
				if (_source != value) 
				{
					_source = value;
					if (OnSourceChanged != null)
						OnSourceChanged ();
				}
			}
			get
			{
				return _source;
			}
		} ///< Source of the image shown by this component.

		[DataMember]
		public bool KeepRatio { get; set; } ///< Adapts image to keep its aspect ratio considering images- and owners size.
		public Color Color { get { return _color; } set { _color = value; if (Source == null) Source = ImageSource.Blank; } } ///< Color multiplier for the image.
		public SliceType SliceType { get { return _sliceType; } set { _sliceType = value; } } ///< SliceType to be used on image drawing.
		[HideFromInspector, DataMember]
		public bool IgnoreClamping { get; set; } ///< Will disregard any clamping options derived by layout system.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public void OnAwake()
		{
			OnSourceChanged += () =>
			{
				if (Source != null)
					_texture = Source.Texture;
			};
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public void OnUpdate()
		{
			if (_source != null)
			{
				if (!IgnoreClamping)
					_texture.ClampRect = ((Owner as UIElement).RectTransform).ClampRect;
				else
					_texture.ClampRect = null;
				if (Color.A > 0.05f) 
				{
					_texture.Angle = (Owner as UIElement).RectTransform.Angle;
					_texture.Color = _color;
					_texture.TargetCanvas = ParentCanvas;

					Rect r = GetAlignedRect ();
					_texture.Draw (r.x, r.y, r.w, r.h, SliceType);
				}
			}
		}

		/// <summary>
		/// Returns the area which the image is supposed to be drawn in, considering this components layout properties.
		/// </summary>
		/// <returns>The aligned Rect.</returns>
		public override Rect GetAlignedRect()
		{
			if (_source == null)
				return new Rect ();
			
			var rt = (Owner as UIElement).RectTransform;
			var tl = rt.TopLeft;
			if (KeepRatio) 
			{
				var imageRatio = (float)_source.Width / (float)_source.Height;
				var elementRatio = rt.Width / rt.Height;
				var ctr = rt.Center;
				if (imageRatio > elementRatio) 
				{
					var imgHeight = rt.Width / imageRatio;
					return new Rect (tl.x, ctr.y - imgHeight / 2.0f, rt.Width, imgHeight);
				} 
				else 
				{
					var imgWidth = rt.Height * imageRatio;
					return new Rect (ctr.x - imgWidth / 2.0f, tl.y, imgWidth, rt.Height);
				}
			}
			else
			{
				return new Rect (tl.x, tl.y, rt.Width, rt.Height);
			}
		}
	}
}
