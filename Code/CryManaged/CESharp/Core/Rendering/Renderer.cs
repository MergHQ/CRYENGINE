using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using CryEngine.Common;

namespace CryEngine.Rendering
{
	/// <summary>
	/// Provides CryEngine's output resolution and sends event if resolution changed.
	/// </summary>
	public class Renderer : IGameUpdateReceiver
	{
		public static event EventHandler<int, int> ResolutionChanged; ///< Event fired if resolution changed

		public static int ScreenWidth { get { return Global.gEnv.pRenderer.GetWidth(); } } ///< Render Output Width
		public static int ScreenHeight { get { return Global.gEnv.pRenderer.GetHeight(); } } ///< Render Output Height
			
		internal static Renderer Instance { get; set; }

		int _lastWidth;
		int _lastHeight;
		
		internal Renderer()
		{
			_lastWidth = ScreenWidth;
			_lastHeight = ScreenHeight;
			GameFramework.RegisterForUpdate(this);
		}

		~Renderer()
		{
			GameFramework.UnregisterFromUpdate(this);
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public void OnUpdate()
		{
			if (_lastWidth != ScreenWidth || _lastHeight != ScreenHeight)
			{
				_lastWidth = ScreenWidth;
				_lastHeight = ScreenHeight;
				if (ResolutionChanged != null)
					ResolutionChanged(_lastWidth, _lastHeight);
			}
		}
	}
}
