// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Linq;
using System.Runtime.Serialization;

namespace CryEngine.UI.Components
{
	/// <summary>
	/// Defines how the area, in which this elements components will react or be visible, is determined by layouting.
	/// For each element, the determined area is defined by property ClampRect. ClampRect is modified by ClampMode options.
	/// </summary>
	public enum ClampMode
	{
		Full, ///< Defines ClampRect as Bounds property padded by Spacing property, intersected by Parent ClampRect.
		Self, ///< Defines ClampRect as Bounds property padded by Spacing property, ignoring Parent ClampRect.
		Parent, ///< Defines ClampRect by Parent ClampRect.
		None ///< No clamping applied.
	}

	/// <summary>
	/// Defines and performs layout on UIElements.
	/// </summary>
	[DataContract]
	public class RectTransform : UIComponent
	{
		public event EventHandler LayoutChanging; ///< Called before layout is changing.
		public event EventHandler LayoutChanged; ///< Called after layout has been changed.

		[DataMember]
		Alignment _alignment = Alignment.TopLeft;
		[DataMember]
		float _width;
		[DataMember]
		float _height;
		[DataMember]
		float _angle;
		[DataMember]
		Padding _padding = new Padding();
		Rect _bounds;
		Rect _clampRect;
		[DataMember]
		Rect _spacing;
		Point _center;
		Point _pivot = new Point(0.5f, 0.5f);
		bool _needsRefresh = true;
		[DataMember]
		ClampMode _clampMode = ClampMode.Self;
		float _passedCenterX = float.MinValue;
		float _passedCenterY = float.MinValue;
		float _passedWidth = float.MinValue;
		float _passedHeight = float.MinValue;

		public Alignment Alignment { set { _alignment = value; _needsRefresh = true; } get { return _alignment; } } ///< Owning element will be centered around the alignment target. Alignment target is computed relative to its parent's Bounds.
		public float Width { set { _width = value; _needsRefresh = true; } get { return _width; } } ///< The owner elements width.
		public float Height { set { _height = value; _needsRefresh = true; } get { return _height; } } ///< The owner elements height.
		public float Angle { set { _angle = value; } get { return _angle; } } ///< The owner elements rotation.
		public Padding Padding { set { _padding = value; _needsRefresh = true; } get { return _padding; } } ///< Layouted raw Bounds will be padded by this.
		public Point Size { set { _width = value.x; _height = value.y; _needsRefresh = true; } } ///< Sets the owner elements size.
		public Rect Spacing { set { _spacing = value; _needsRefresh = true; } get { return _spacing; } } ///< Modifies the potentially visible area, defines by ClampMode.
		public ClampMode ClampMode { set { _clampMode = value; _clampRect = null; } get { return _clampMode; } } ///< Defines how clamping should be computed in ClampRect.
		public Point Pivot { get { return _pivot; } set { _pivot = value; _needsRefresh = true; } } ///< Defines the center of the element, relative to its size.

		public bool NeedsRefresh { get { return _needsRefresh; } } ///< Indicates whether this RectTransform needs a layout refresh.
		public RectTransform PRT { get { return (Transform.Parent != null && Transform.Parent.Owner is UIElement) ? (Transform.Parent.Owner as UIElement).RectTransform : null; } } ///< Returns Parent elements RectTransform, if available.
		public Point Center { get { PerformLayout(); return _center; } } ///< Returns center of owning element.
		public Point Top { get { PerformLayout(); return new Point(Center.x, Center.y - Height / 2.0f); } } ///< Returns top of owning element.
		public Point TopRight { get { PerformLayout(); return new Point(Center.x + Width / 2.0f, Center.y - Height / 2.0f); } } ///< Returns top right of owning element.
		public Point Right { get { PerformLayout(); return new Point(Center.x + Width / 2.0f, Center.y); } } ///< Returns right of owning element.
		public Point BottomRight { get { PerformLayout(); return new Point(Center.x + Width / 2.0f, Center.y + Height / 2.0f); } } ///< Returns bottom right of owning element.
		public Point Bottom { get { PerformLayout(); return new Point(Center.x, Center.y + Height / 2.0f); } } ///< Returns bottom of owning element.
		public Point BottomLeft { get { PerformLayout(); return new Point(Center.x - Width / 2.0f, Center.y + Height / 2.0f); } } ///< Returns bottom left of owning element.
		public Point Left { get { PerformLayout(); return new Point(Center.x - Width / 2.0f, Center.y); } } ///< Returns left of owning element.
		public Point TopLeft { get { PerformLayout(); return new Point(Center.x - Width / 2.0f, Center.y - Height / 2.0f); } } ///< Returns top left of owning element.
		public Rect Bounds { get { PerformLayout(); return _bounds; } } ///< Resulting area for owning element after layouting.
		public Rect ClampRect { get { PerformLayout(); return _clampRect; } } ///< Resulting are for clamping after layouting.

		void ComputeClampRect()
		{
			var prt = PRT;
			switch (ClampMode)
			{
				case ClampMode.Full:
					_clampRect = Spacing != null ? _bounds.Pad(Spacing) : _bounds;

					// Use intersection if parent and own rect exist
					if (prt != null && prt.ClampRect != null && _clampRect != null)
						_clampRect = prt._clampRect & _clampRect;

					// Take over Parent CR if self null
					if (_clampRect == null && prt != null)
						_clampRect = prt._clampRect;
					break;

				case ClampMode.Self:
					_clampRect = Spacing != null ? Bounds.Pad(Spacing) : _bounds;
					break;

				case ClampMode.Parent:
					_clampRect = prt != null ? prt._clampRect : null;
					break;

				case ClampMode.None:
					_clampRect = null;
					break;
			}
		}

		RectTransform GetOutdatedAncestor()
		{
			if (Transform.Parent == null || !(Transform.Parent.Owner is UIElement))
				return null;
			var prt = (Transform.Parent.Owner as UIElement).RectTransform;
			if (prt.NeedsRefresh)
				return prt;
			return prt.GetOutdatedAncestor();
		}

		bool DeltaChanged
		{
			get
			{
				if (Math.Abs(_passedCenterX - _center.x) > 1 || Math.Abs(_passedCenterY - _center.y) > 1 || Math.Abs(_passedWidth - _width) > 1 || Math.Abs(_passedHeight - _height) > 1)
				{
					_passedCenterX = _center.x;
					_passedCenterY = _center.y;
					_passedWidth = _width;
					_passedHeight = _height;
					return true;
				}
				return false;
			}
		}

		/// <summary>
		/// Performs layout of this element, if any layout property changed locally. Will perform layout of any direct Parent before, if required.
		/// If any valiable result has changed after layout, element's children will be layouted too.
		/// </summary>
		/// <param name="forceRefresh">If set to <c>true</c> local refresh is forced.</param>
		public void PerformLayout(bool forceRefresh = false)
		{
			if (!forceRefresh && !NeedsRefresh)
				return;

			if (!forceRefresh)
			{
				var art = GetOutdatedAncestor();
				if (art != null)
				{
					art.PerformLayout();
					if (!NeedsRefresh)
						return;
				}
			}

			if (LayoutChanging != null)
				LayoutChanging();

			var prt = PRT;
			if (prt == null)
			{
				_center = new Point(_width / 2, _height / 2);
			}
			else
			{
				var pivot = _padding.TopLeft + new Point((Pivot.x - 0.5f) * Width, (Pivot.y - 0.5f) * Height);
				switch (_alignment)
				{
					case Alignment.Center:
						_center = prt.Center + pivot; break;
					case Alignment.Top:
						_center = prt.Top + pivot; break;
					case Alignment.TopRight:
						_center = prt.TopRight + pivot; break;
					case Alignment.Right:
						_center = prt.Right + pivot; break;
					case Alignment.BottomRight:
						_center = prt.BottomRight + pivot; break;
					case Alignment.Bottom:
						_center = prt.Bottom + pivot; break;
					case Alignment.BottomLeft:
						_center = prt.BottomLeft + pivot; break;
					case Alignment.Left:
						_center = prt.Left + pivot; break;
					case Alignment.TopLeft:
						_center = prt.TopLeft + pivot; break;

					case Alignment.TopHStretch:
						_width = prt.Width - _padding.Right - _padding.Left;
						_center = prt.TopLeft + pivot + new Point(_width / 2, -_padding.Bottom); break;
					case Alignment.BottomHStretch:
						_width = prt.Width - _padding.Right - _padding.Left;
						_center = prt.BottomLeft + pivot + new Point(_width / 2, -_padding.Bottom); break;
					case Alignment.RightVStretch:
						_height = prt.Height - _padding.Top - _padding.Bottom;
						_center = prt.TopRight + pivot + new Point(-_padding.Right, _height / 2); break;

					case Alignment.Stretch:
						_width = prt.Width - _padding.Right - _padding.Left;
						_height = prt.Height - _padding.Bottom - _padding.Top;
						_center = prt.TopLeft + pivot + new Point(_width / 2, _height / 2); break;
					default:
						break;
				}
			}

			_needsRefresh = false;
			var tl = TopLeft;
			_bounds = new Rect(tl.x, tl.y, _width, _height);
			ComputeClampRect();

			if (DeltaChanged)
				Transform.Children.Where(t => t.Owner is UIElement).ToList().ForEach(t => (t.Owner as UIElement).RectTransform.PerformLayout(true));

			if (LayoutChanged != null)
				LayoutChanged();
		}
	}
}
