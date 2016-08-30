// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using EnvDTE;
using EnvDTE80;
using System.Windows.Forms;

namespace CESharpProjectType
{
	/// <summary>
	/// Manage output to VS Ouput Pane.
	/// </summary>
	public class VsOutputPane
	{
		private static OutputWindowPane _outputWindowPane = null;

		public void Init(DTE2 applicationObject)
		{
			// Create output window, make it active and clear old content.
			Window window = applicationObject.Windows.Item(EnvDTE.Constants.vsWindowKindOutput);
			OutputWindow outputWindow = (OutputWindow)window.Object;

			try
			{
				_outputWindowPane = outputWindow.OutputWindowPanes.Item("CESharp");
			}
			catch { }

			if (_outputWindowPane == null)
				_outputWindowPane = outputWindow.OutputWindowPanes.Add("CESharp");
		}

		public void Shutdown()
		{
			_outputWindowPane = null;
		}

		public static void ClearLog()
		{
			if (_outputWindowPane != null)
				_outputWindowPane.Clear();
		}

		public static void FocusOnLog()
		{
			if (_outputWindowPane != null)
				_outputWindowPane.Activate();
		}

		public static void LogMessage(string message)
		{
			try
			{
				if (_outputWindowPane != null)
					_outputWindowPane.OutputString(string.Format("{0}\n", message));
				//FocusOnLog();
			}
			catch (System.Exception e)
			{
				System.Windows.Forms.MessageBox.Show(e.ToString());
			}
		}

		public static void LogInfo(string message)
		{
			try
			{
				if (_outputWindowPane != null)
					_outputWindowPane.OutputString(string.Format("Info: {0}\n", message));
				//FocusOnLog();
			}
			catch (System.Exception e)
			{
				System.Windows.Forms.MessageBox.Show(e.ToString());
			}
		}

		public static void LogWarning(string message)
		{
			try
			{
				if (_outputWindowPane != null)
					_outputWindowPane.OutputString(string.Format("Warning: {0}\n", message));
			}
			catch (System.Exception e)
			{
				System.Windows.Forms.MessageBox.Show(e.ToString());
			}
		}

		public static void LogError(string message)
		{
			try
			{
				if (_outputWindowPane != null)
					_outputWindowPane.OutputString(string.Format("Error: {0}\n", message));
				FocusOnLog();
			}
			catch (System.Exception e)
			{
				System.Windows.Forms.MessageBox.Show(e.ToString());
			}
		}
	}
}
