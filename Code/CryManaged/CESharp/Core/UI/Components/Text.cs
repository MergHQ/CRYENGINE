// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Drawing;
using CryEngine.Resources;
using System.Collections.Generic;
using System.Linq;
using CryEngine.Common;
using System.Runtime.Serialization;

namespace CryEngine.UI.Components
{
	/// <summary>
	/// Renders arbitrary text to a Texture and displays it according to its layout properties, relative to owners layout.
	/// </summary>
	[DataContract]
	public class Text : UIComponent
	{
		static Dictionary<int,int> _heightToSize = new Dictionary<int, int>();

		public static byte DefaultHeight = 15; ///< Target Height of the font in pixels.

		[DataMember]
		string _content = "";
		[DataMember]
		byte _height = DefaultHeight;
		[DataMember]
		string _fontName = "Arial";
		[DataMember]
		FontStyle _fontStyle = FontStyle.Regular;
		[DataMember]
		Alignment _alignment = Alignment.Left;
		[DataMember]
		Point _offset = new Point(0,0);
		bool _requiresUpdate = true;
		Texture _texture = null;
		[DataMember]
		Color _color = Color.White;
		Point _alignedOffset = new Point();
		Point _alignedSize = new Point();
		bool _dropsShadow = true;

		public string Content { get{ return _content; } set{ if (_content != value) _requiresUpdate = true; _content = value; }} ///< The text to be displayed.
		public int Width { get{ return _texture.Width; } } ///< Aligned width for the text content.
		public byte Height { get{ return _height; } set{ if (_height != value) _requiresUpdate = true; _height = value; }} ///< Aligned height for the text content.
		public Point Offset { get{ return _offset; } set{ if (_offset != value) _requiresUpdate = true; _offset = value; }} ///< Offset for the output of the text, relative to the layouted position.
		public string FontName { get{ return _fontName; } set{ if (_fontName != value) _requiresUpdate = true; _fontName = value; }} ///< Determines the font to be used for rendering.
		public FontStyle FontStyle { get{ return _fontStyle; } set{ if (_fontStyle != value) _requiresUpdate = true; _fontStyle = value; }} ///< Determines the font style to be used for rendering.
		public Alignment Alignment { get{ return _alignment; } set{ if (_alignment != value) _requiresUpdate = true; _alignment = value; }} ///< Determines the alignment of text, relative to owners layout.
		public Color Color { get{ return _color; } set{ _color = value; }} ///< Color for the rendered texture to be multiplied with.
		public bool DropsShadow { get{ return _dropsShadow; } set{ if (_dropsShadow != value) _requiresUpdate = true;  _dropsShadow = value; }} ///< Determines whether the rendered text is shadowed.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public void OnAwake()
		{
			var rt = (Owner as UIElement).RectTransform;
			rt.LayoutChanged += () => UpdateLayout ();
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public void OnUpdate()
		{
			UpdateContent ();
			var rt = (Owner as UIElement).RectTransform;
			if (_texture != null) 
			{
				_texture.ClampRect = ((Owner as UIElement).RectTransform).ClampRect;
				_texture.Color = Color;
				_texture.TargetCanvas = ParentCanvas;
				_texture.Draw (_alignedOffset.x, _alignedOffset.y, _alignedSize.x, _alignedSize.y);
			}
		}

		float VAlignMultiplier
		{
			get 
			{
				if (Alignment == Alignment.Top || Alignment == Alignment.TopLeft || Alignment == Alignment.TopRight)
					return 0;
				if (Alignment == Alignment.Bottom || Alignment == Alignment.BottomLeft || Alignment == Alignment.BottomRight)
					return 1;
				return 0.5f;
			}
		}

		float HAlignMultiplier
		{
			get 
			{
				if (Alignment == Alignment.Left || Alignment == Alignment.TopLeft || Alignment == Alignment.BottomLeft)
					return 0;
				if (Alignment == Alignment.Right || Alignment == Alignment.TopRight || Alignment == Alignment.BottomRight)
					return 1;
				return 0.5f;
			}
		}

		int GetSizeForHeight(byte height)
		{
			if (_heightToSize.ContainsKey(height))
				return _heightToSize[height];
			Graphics g = Graphics.FromHwnd (IntPtr.Zero);
			var max = _heightToSize.Count > 0 ? _heightToSize.Values.Max () : 6;
			SizeF size;
			do 
			{
				max++;
				var font = new Font (FontName, max, _fontStyle);

				size = g.MeasureString ("Test", font);
				for (int i = (int)size.Height; i >= 0; i--) 
				{
					if (_heightToSize.ContainsKey (i))
						break;
					_heightToSize [i] = max;
				}
			} 
			while((int)size.Height < height);
			return _heightToSize [height];
		}

		/// <summary>
		/// Returns the relative position of a letter by its index.
		/// </summary>
		/// <returns>The relative position of the indexed letter.</returns>
		/// <param name="index">The index value.</param>
		public int GetOffsetAt(int index)
		{
			if (index < 1 || index > _content.Length)
				return 4;
			if (index == _content.Length)
				return (int)_alignedSize.x;

			Graphics g = Graphics.FromHwnd (IntPtr.Zero);
			var font = new Font (FontName, GetSizeForHeight(Height), _fontStyle);
			var nullSize = g.MeasureString ("XX", font);
			var fillSize = g.MeasureString ("X" + _content.Substring (0, index) + "X", font);
			var width = fillSize.Width - nullSize.Width;
			return (int)width + 1;
		}

		/// <summary>
		/// Computes the relative layout alignment for this component.
		/// </summary>
		public void UpdateLayout()
		{
			Graphics g = Graphics.FromHwnd (IntPtr.Zero);
			var font = new Font (FontName, GetSizeForHeight(Height), _fontStyle);
			var nullSize = g.MeasureString ("XX", font);
			var fillSize = g.MeasureString ("X" + _content + "X", font);
			var rt = (Owner as UIElement).RectTransform;
			var tl = rt.TopLeft;
			var width = fillSize.Width - nullSize.Width * 0.8f;
			var height = fillSize.Height;
			_alignedOffset.x = tl.x + HAlignMultiplier * (rt.Width - width) + Offset.x;
			_alignedOffset.y = tl.y + VAlignMultiplier * (rt.Height - height) + Offset.y;
			_alignedSize.x = (int)width + 1;
			_alignedSize.y = (int)height + 1;
		}

		/// <summary>
		/// Renders the text content to an ITexture, in case it was modified.
		/// </summary>
		public void UpdateContent()
		{
			if (!_requiresUpdate)
				return;
			UpdateLayout ();
			int ax = Math.Max(1, (int)_alignedSize.x);
			int ay = Math.Max(1, (int)_alignedSize.y);

			var font = new Font (FontName, GetSizeForHeight(Height), _fontStyle);
			var bmp = new Bitmap (ax, ay, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
			Graphics g = Graphics.FromImage(bmp);
			g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.AntiAlias;
			if (DropsShadow)
				g.DrawString (_content, font, Brushes.Black, 1, 1);
			g.DrawString (_content, font, Brushes.White, 0, 0);
			var data = bmp.GetPixels ();

			if (_texture == null)
				_texture = new Texture (ax, ay, data, false, true);
			else
				_texture.UpdateData (ax, ay, data);
			
			_requiresUpdate = false;
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public void OnDestroy()
		{
			if (_texture != null)
				_texture.Destroy ();
			_texture = null;
		}

		/// <summary>
		/// Returns the layouted relative alignment.
		/// </summary>
		/// <returns>The aligned rect.</returns>
		public override Rect GetAlignedRect()
		{
			return new Rect (_alignedOffset.x, _alignedOffset.y, _alignedSize.x, _alignedSize.y);
		}
	}
}
