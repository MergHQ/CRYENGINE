// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Resources;
using CryEngine.UI.Components;
using System;
using System.IO;
using System.Runtime.Serialization;

namespace CryEngine.UI
{
	/// <summary>
	/// Used in Splitter to allow for modifying the size of two split areae.
	/// </summary>
	[DataContract]
	public class SplitBar : Panel
	{
		[DataMember]
		float _currentAlpha;
		Image _knobImg;

		public SplitBarCtrl SBCtrl { get; private set; } ///< The input controller for this element. 

		public bool IsVertical
		{
			set
			{
				if (value)
				{
					Background.Source = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "vsplit_bar.png"), false);
					_knobImg.Source = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "vsplit_bar_knob.png"), false);
				}
				else
				{
					Background.Source = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "hsplit_bar.png"), false);
					_knobImg.Source = ResourceManager.ImageFromFile(Path.Combine(UIElement.DataDirectory, "hsplit_bar_knob.png"), false);
				}
			}
		} ///< Sets the orientation of the SplitBar. Modified by owning Splitter.

		public float CurrentAlpha
		{
			set
			{
				_currentAlpha = value;
				Background.Color = Color.White.WithAlpha(_currentAlpha / 2);
				_knobImg.Color = Color.White.WithAlpha((0.5f + _currentAlpha) / 1.5f);
			}
		} ///< Sets the transparency of the SplitBar.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnAwake()
		{
			base.OnAwake();
			Background.SliceType = SliceType.ThreeVertical;
			_knobImg = AddComponent<Image>();
			_knobImg.KeepRatio = true;
			SBCtrl = AddComponent<SplitBarCtrl>();
			CurrentAlpha = SBCtrl.TargetAlpha;
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnUpdate()
		{
			CurrentAlpha = _currentAlpha * 0.8f + SBCtrl.TargetAlpha * 0.2f;
		}
	}

	/// <summary>
	/// Divides assigned area into either two horizontally or two vertically split areae. The splitter in between can be moved. Areae may be collapsed.
	/// </summary>
	[DataContract]
	public class Splitter : UIElement
	{
		public event EventHandler<bool> CollapsedChanged; ///< Invoked if one of the areae was collapsed or expanded. Parameter indicates the corresponding state change.

		SplitBar _splittBar;
		Panel _paneA;
		Panel _paneB;
		[DataMember]
		bool _isVertical = true;
		int _splitterWidth = 10;
		[DataMember]
		float _splitterPos = 50.0f;
		[DataMember]
		float _splitterMin = 10.0f;
		[DataMember]
		float _splitterMax = 90.0f;
		float _modSplitterPos;

		public Panel PaneA { get { return _paneA; } } ///< The first panel for content.
		public Panel PaneB { get { return _paneB; } } ///< The second panel for content.

		public bool IsVertical
		{
			set
			{
				_isVertical = value;
				_splittBar.IsVertical = value;
			}
			get
			{
				return _isVertical;
			}
		} ///< Determines the orientation of the Splitter.

		public float SplitterSpanMin { set { _splitterMin = value; SplitterPos = _splitterPos; } } ///< Sets the min. size in % of the total Splitter size, which the first plane can be reduced to.
		public float SplitterSpanMax { set { _splitterMax = value; SplitterPos = _splitterPos; } } ///< Sets the max. size in % of the total Splitter size, which the first plane can be expanded to.
		[DataMember]
		public bool CanCollapse { get; set; } ///< Determines whether content panels can be collapsed.

		public float SplitterPos
		{
			set
			{
				_splitterPos = value;
				LayoutChanged();
			}
			get
			{
				return _splitterPos;
			}
		} ///< Indicates the position of the SplitBar in %, relative to the total SplitBar size.

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void OnAwake()
		{
			_paneA = SceneObject.Instantiate<Panel>(this);
			_paneA.Name = "PaneA";
			_paneA.GetComponent<Image>().Color = Color.Transparent;
			_paneA.RectTransform.Alignment = Alignment.TopLeft;

			_paneB = SceneObject.Instantiate<Panel>(this);
			_paneB.Name = "PaneB";
			_paneB.GetComponent<Image>().Color = Color.Transparent;
			_paneB.RectTransform.Alignment = Alignment.TopLeft;

			_splittBar = SceneObject.Instantiate<SplitBar>(this);
			_splittBar.RectTransform.Alignment = Alignment.TopLeft;
			_splittBar.RectTransform.Size = new Point(_splitterWidth, _splitterWidth);
			_splittBar.IsVertical = IsVertical;
			_splittBar.SBCtrl.Pulling += (x, y) =>
			{
				if (_isVertical)
					SplitterPos = (RectTransform.Width * _splitterPos / 100.0f + x) * 100.0f / RectTransform.Width;
				else
					SplitterPos = (RectTransform.Height * _splitterPos / 100.0f + y) * 100.0f / RectTransform.Height;
			};

			RectTransform.LayoutChanged += LayoutChanged;
		}

		void LayoutChanged()
		{
			var w = RectTransform.Width;
			var h = RectTransform.Height;
			var prevModSplitterPos = _modSplitterPos;
			_modSplitterPos = Math.Min(_splitterMax, Math.Max(_splitterMin, _splitterPos));
			var splitterMaxCollapseThreshold = 100 - (100 - _splitterMax) / 2;
			if (CanCollapse)
			{
				if (_splitterPos < _splitterMin / 2)
					_modSplitterPos = 0;
				if (_splitterPos > splitterMaxCollapseThreshold)
					_modSplitterPos = 100;
			}
			_paneA.Active = _modSplitterPos >= _splitterMin;
			_paneB.Active = _modSplitterPos <= _splitterMax;

			bool modSplitterCollapsed = _modSplitterPos < _splitterMin / 2 || _modSplitterPos > splitterMaxCollapseThreshold;
			bool prevModSplitterCollapsed = prevModSplitterPos < _splitterMin / 2 || prevModSplitterPos > splitterMaxCollapseThreshold;
			if (prevModSplitterCollapsed != modSplitterCollapsed && CollapsedChanged != null)
				CollapsedChanged(modSplitterCollapsed);

			if (_isVertical)
			{
				var splitterOfs = _splitterWidth / 2 + _modSplitterPos * (w - _splitterWidth) / 100.0f;
				_paneA.RectTransform.Size = new Point(splitterOfs - _splitterWidth / 2, h);
				_paneA.RectTransform.Padding.TopLeft = new Point(splitterOfs / 2 - _splitterWidth / 4, h / 2);

				_paneB.RectTransform.Size = new Point(w - splitterOfs - _splitterWidth / 2, h);
				_paneB.RectTransform.Padding.TopLeft = new Point(splitterOfs + _splitterWidth / 2 + (w - splitterOfs - _splitterWidth / 2) / 2, h / 2);

				_splittBar.RectTransform.Size = new Point(_splitterWidth, h - 10);
				_splittBar.RectTransform.Padding.Top = h / 2;
				_splittBar.RectTransform.Padding.Left = splitterOfs;
			}
			else
			{
				var splitterOfs = _splitterWidth / 2 + _modSplitterPos * (h - _splitterWidth) / 100.0f;
				_paneA.RectTransform.Size = new Point(w, splitterOfs - _splitterWidth / 2);
				_paneA.RectTransform.Padding.TopLeft = new Point(w / 2, splitterOfs / 2 - _splitterWidth / 4);

				_paneB.RectTransform.Size = new Point(w, h - splitterOfs - _splitterWidth / 2);
				_paneB.RectTransform.Padding.TopLeft = new Point(w / 2, splitterOfs + _splitterWidth / 2 + (h - splitterOfs - _splitterWidth / 2) / 2);

				_splittBar.RectTransform.Size = new Point(w - 10, _splitterWidth);
				_splittBar.RectTransform.Padding.Left = w / 2;
				_splittBar.RectTransform.Padding.Top = splitterOfs;
			}
		}
	}
}
