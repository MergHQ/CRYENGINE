// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Resources;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Attributes;

namespace CryEngine.Editor.Views
{
	public class ObjectView
	{
		public CollapseCtrl CollapseCtrl;
		public Text Text;
		public SceneObject Object;
		public bool IsCollapsed { get { return CollapseCtrl != null ? CollapseCtrl.Collapsed : false; } }

		public ObjectView(SceneObject parent, SceneObject obj, bool active, bool collapsed)
		{
			Object = obj;
			Text = parent.AddComponent<Text> ();
			Text.Content = obj.Name;
			Text.Alignment = Alignment.TopLeft;
			Text.Height = 16;
			Text.Color = obj.Active ? Color.White : Color.Gray;
			Text.UpdateContent ();

			if (obj.Transform.Children.Exists(x => x.Owner.GetType().GetCustomAttributes(typeof(HideFromInspectorAttribute), true).Length == 0))
			{
				CollapseCtrl = parent.AddComponent<CollapseCtrl>();
				CollapseCtrl.Collapsed = collapsed;
				CollapseCtrl.Context = obj;
			}

			obj.ActiveChanged += TMActiveChanged;
		}

		public void SetPosition(int x, int y)
		{
			Text.Offset = new Point (13 + x, y);
			if (CollapseCtrl != null)
				CollapseCtrl.Icon.RectTransform.Padding = new Padding (8 + x, 7 + y);
		}

		public void SetActive(bool active)
		{
			Text.Active = active;
			if (CollapseCtrl != null)
				CollapseCtrl.Icon.Active = active;
		}

		void TMActiveChanged(SceneObject so)
		{
			Text.Color = so.Active ? Color.White : Color.Gray;
		}

		public void Clear()
		{
			Object.ActiveChanged -= TMActiveChanged;
		}
	}

	[DataContract]
	public class SelectionIndicatorCtrl : Component
	{
		public event EventHandler<int> IndexChanged;
		public ScrollPanel ScrollPanel;

		public void OnLeftMouseDown(int x, int y)
		{
			var ort = (Owner as UIElement).RectTransform;
			if (x < ort.Left.x + 15 || x > ort.Right.x - 15)
				return;
			var fVal = (y - ort.Top.y - 47 - ScrollPanel.ScrollOffset.y) / 16.0f;
			if (IndexChanged != null)
				IndexChanged ((int)Math.Round (fVal));
		}

		public override bool HitTest(int x, int y)
		{
			var ort = (Owner as UIElement).RectTransform;
			return ort.ClampRect != null && ort.ClampRect.Contains (x, y);
		}
	}

	[HideFromInspector, DataContract]
	public class SceneTreeView : Window
	{
		public event EventHandler<SceneObject> SelectionChanged;

		List<SceneObject> _idxToObject = new List<SceneObject> ();
		SceneObject _root = null;
		List<ObjectView> _objectViews = new List<ObjectView>();
		ScrollPanel _scrollPlanel;
		Panel _selectionIndicator;
		SelectionIndicatorCtrl _siCtrl;
		int _selectedIndex = -1;
		float _tvWidth;

		public SceneObject SceneRoot 
		{
			get 
			{
				return _root;
			}
			set 
			{
				_root = value;
				UpdateTreeView (true);
			}
		}

		public override void OnAwake()
		{
			base.OnAwake ();

			_scrollPlanel = SceneObject.Instantiate<ScrollPanel>(this);
			_scrollPlanel.RectTransform.Padding = new Padding (13,13+23,14,14);

			_selectionIndicator = SceneObject.Instantiate<Panel> (_scrollPlanel);
			_selectionIndicator.Name = "IndicatorPanel";
			_selectionIndicator.Background.Color = Color.SkyBlue.WithAlpha(0.75f);
			_selectionIndicator.RectTransform.Alignment = Alignment.TopLeft;
			_selectionIndicator.RectTransform.Size = new Point (16,16);
			_selectionIndicator.RectTransform.ClampMode = ClampMode.Full;
			_selectionIndicator.SetAsFirstSibling ();
			SelectedIndex = -1;

			_scrollPlanel.Panel.RectTransform.LayoutChanged += LayoutSelectionIndicator;
			_siCtrl = AddComponent<SelectionIndicatorCtrl> ();
			_siCtrl.ScrollPanel = _scrollPlanel;
			_siCtrl.IndexChanged += (val) => SelectedIndex = val;
		}

		void LayoutSelectionIndicator()
		{
			var w = _scrollPlanel.RectTransform.Width - (_scrollPlanel.VScrollBarActive ? 9 : 0);
			var spcr = _scrollPlanel.Panel.RectTransform.ClampRect;
			_selectionIndicator.RectTransform.Padding = new Padding(w/2, _scrollPlanel.ScrollOffset.y + 9 + _selectedIndex * 16, 0, 0);
			_selectionIndicator.RectTransform.PerformLayout (true);
			var sib = _selectionIndicator.RectTransform.Bounds;
			var hPad = (sib.y+sib.h) - (spcr.y + spcr.h);
			_selectionIndicator.RectTransform.Spacing = new Rect () { h = hPad > 0 ? hPad : 0 };
			_selectionIndicator.RectTransform.Width = w;
		}

		void ClearView()
		{
			foreach (var ov in _objectViews) 
			{
				ov.Clear ();
				_scrollPlanel.Panel.RemoveComponent (ov.Text);
			}
			_objectViews.Clear ();
		}

		void UpdateTreeView(bool fromScratch=false)
		{
			if (fromScratch)
				ClearView ();
			if (_root == null)
				return;

			_idxToObject.Clear ();
			_tvWidth = 0;
			UpdateTreeView (SceneRoot.Transform, 0);
			_scrollPlanel.SetPanelSize(_tvWidth, 2 + _idxToObject.Count * 16);
		}

		void UpdateTreeView(Transform t, int depth, bool anyParentCollapsed=false)
		{
			var children = new List<Transform> (t.Children);
			foreach (var ct in children) 
			{
				var ov = _objectViews.SingleOrDefault (x => x.Object == ct.Owner);
				if (ov == null) 
				{
					if (ct.Owner.GetType().GetCustomAttributes(typeof(HideFromInspectorAttribute), true).Length > 0)
						continue;

					ov = new ObjectView (_scrollPlanel.Panel, ct.Owner, !anyParentCollapsed, depth > 1);
					_objectViews.Add (ov);
					if (ov.CollapseCtrl != null)
						ov.CollapseCtrl.OnCollapseChanged += (b) => UpdateTreeView ();
				} 
				ov.SetPosition (2 + depth * 10, 2 + _idxToObject.Count * 16);
				ov.SetActive (!anyParentCollapsed);

				if (!anyParentCollapsed)
				{
					_idxToObject.Add (ct.Owner);
					_tvWidth = Math.Max (_tvWidth, ov.Text.Offset.x + ov.Text.Width);
				}
				UpdateTreeView (ct, depth + 1, anyParentCollapsed || ov.IsCollapsed);
			}
		}

		public int SelectedIndex
		{
			set
			{
				bool isInsideTree = (value >= 0 && value < _idxToObject.Count);
				_selectionIndicator.Active = isInsideTree;
				_selectedIndex = isInsideTree ? value : -1;
				LayoutSelectionIndicator ();

				if (SelectionChanged != null)
					SelectionChanged (_selectedIndex >= 0 ? _idxToObject[_selectedIndex] : null);
			}
		}
	}
}
