// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Resources;
using CryEngine.Common;
using System.Linq;
using System.Collections.Generic;
using CryEngine.FlowSystem;
using CryEngine.Components;

namespace CryEngine.Editor
{
	public class Editor : Application
	{
		private Canvas _canvas;
		private Text _log;
		private float _prevVSplitterPos;

		public void OnAwake ()
		{
			Log.Info ("Editor Awaken");

			_canvas = SceneObject.Instantiate<Canvas>(Root);
			const int RES = 768;
			byte[] data = new byte[RES * RES * 4];
			_canvas.TargetTexture = new Texture (RES, RES , data, true, false, true);

			var vSpl = SceneObject.Instantiate<Splitter> (_canvas);
			vSpl.RectTransform.Alignment = Alignment.Stretch;
			vSpl.SplitterPos = 30;
			vSpl.CanCollapse = true;

			var hSpl = SceneObject.Instantiate<Splitter> (vSpl.PaneA);
			hSpl.RectTransform.Alignment = Alignment.Stretch;
			hSpl.SplitterPos = 40;
			hSpl.SplitterSpanMin = 15;
			hSpl.SplitterSpanMax = 85;
			hSpl.IsVertical = false;
			hSpl.CanCollapse = true;

			// SceneTreeView
			var stv = SceneObject.Instantiate<Views.SceneTreeView>(hSpl.PaneA);
			stv.Caption = "Scene Tree";
			stv.RectTransform.Alignment = Alignment.Stretch;

			// ComponentView
			var iv = SceneObject.Instantiate<Views.InspectorView>(hSpl.PaneB);
			iv.RectTransform.Alignment = Alignment.Stretch;

			// Window
			var wndPanel = SceneObject.Instantiate<Window>(_canvas);
			wndPanel.Caption = "Settings";
			wndPanel.RectTransform.Alignment = Alignment.BottomRight;
			wndPanel.RectTransform.Padding = new Padding (-90, -60);
			wndPanel.RectTransform.Size = new Point(160, 100);

			// Checkbox "Show Side Bar"
			var cbt = wndPanel.AddComponent<Text> ();
			cbt.Height = 16;
			cbt.Offset = new Point (20, 0);
			cbt.Content = "Side Panel";

			var cb1 = SceneObject.Instantiate<CheckBox>(wndPanel);
			cb1.RectTransform.Padding = new Padding (130, 50);
			cb1.Ctrl.CheckedChanged += (isChecked) => 
			{
				if (!isChecked)
					_prevVSplitterPos = Math.Max(20, vSpl.SplitterPos);
				vSpl.SplitterPos = isChecked ? _prevVSplitterPos : 0;
			};
			vSpl.CollapsedChanged += (collapsed) => cb1.Ctrl.IsChecked = !collapsed;

			// Image
			var imgPanel = SceneObject.Instantiate<Panel>(vSpl.PaneB);
			imgPanel.Name = "TestImage";
			imgPanel.Background.Source = ResourceManager.ImageFromFile (UIPath + "TestImage.png");
			imgPanel.Background.KeepRatio = true;
			imgPanel.Background.Color = Color.White.WithAlpha(0.8f);
			imgPanel.RectTransform.Padding = new Padding (50, 50);
			imgPanel.RectTransform.Size = new Point(120, 100);

			// Checkbox "Show Image"
			cbt = wndPanel.AddComponent<Text> ();
			cbt.Height = 16;
			cbt.Offset = new Point (20, 20);
			cbt.Content = "Show Image";
			var cb2 = SceneObject.Instantiate<CheckBox>(wndPanel);
			cb2.RectTransform.Padding = new Padding (130, 70);
			cb2.Ctrl.CheckedChanged += (b) => imgPanel.Active = b;
			imgPanel.ActiveChanged += (so) => cb2.Ctrl.IsChecked = so.Active;
			cb2.Ctrl.IsChecked = false;

			// Visualize Root
			stv.SelectionChanged += obj => { iv.Context = obj; };
			stv.SceneRoot = Root;

			// Some Log output
			_log = _canvas.AddComponent<Text> ();
			_log.Height = 15;
			_log.Alignment = Alignment.Bottom;

			SystemHandler.EditorGameStart += TryInitTargetTexture;
			SystemHandler.PrecacheEnded += TryInitTargetTexture;
			TryInitTargetTexture ();
		}

		void TryInitTargetTexture()
		{
			var thePlane = EntitySystem.Entity.ByName ("ThePlane");
			if (thePlane == null)
				return;
			thePlane.Material.SetTexture (_canvas.TargetTexture.ID);
			_canvas.TargetEntity = thePlane;
		}

		public void OnUpdate()
		{
			if (Mouse.HitEntityId >= 0) 
			{
				_log.Content = "Hit ID: " + Mouse.HitEntityId + " (" + Mouse.HitEntityUV.ToString ("0.0") + ")";
			}
			else
				_log.Content = "No Hit";
		}

		public void OnDestroy ()
		{
			Log.Info ("Editor Destroyed");
		}
	}
}
