// Copyright 2001-2016 Crytek GmbH / CrytekGroup. All rights reserved.

using System;
using System.Collections.Generic;
using CryEngine.Common;

namespace CryEngine
{
	public class ActionHandler : IActionListener
	{
		private Dictionary<string, Action<string, int, float>> _handlers = new Dictionary<string, Action<string, int, float>>();
		private string _actionMap;

		public ActionHandler(string actionMap)
		{
			_actionMap = actionMap;
			Global.gEnv.pGameFramework.GetIActionMapManager().AddExtraActionListener(this, _actionMap);
		}

		public override void AfterAction() { }

		public override void OnAction(CCryName action, int activationMode, float value)
		{
			string actionName = action.c_str();
			if(_handlers.ContainsKey(actionName))
			{
				_handlers[actionName](actionName, activationMode, value);
			}
		}

		public override void Dispose()
		{
			Global.gEnv.pGameFramework.GetIActionMapManager().RemoveExtraActionListener(this, _actionMap);
			base.Dispose();
		}

		public void AddHandler(string action, Action<string, int, float> handler)
		{
			_handlers.Add(action, handler);
		}

		public void RemoveHandler(string action)
		{
			_handlers.Remove(action);
		}
	}
}