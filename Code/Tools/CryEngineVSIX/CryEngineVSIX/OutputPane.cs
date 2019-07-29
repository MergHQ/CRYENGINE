using EnvDTE;
using EnvDTE80;
using System.Threading.Tasks;

namespace CryEngineVSIX
{
	public class OutputPane
	{
		private static OutputWindowPane _pane;

		public static void Init(DTE2 appObj)
		{
			Window window = appObj.Windows.Item(EnvDTE.Constants.vsWindowKindOutput);
			var output = (OutputWindow)window.Object;
			try
			{
				_pane = output.OutputWindowPanes.Item("CryEngine");
			}
			catch
			{
			}

			if (_pane == null)
				_pane = output.OutputWindowPanes.Add("CryEngine");
		}

		public static void Shutdown()
		{
			Clear();
			_pane = null;
		}

		public static void Clear()
		{
			_pane?.Clear();
		}

		public static void Focus()
		{
			_pane?.Activate();
		}

		public static void Log(string message)
		{
			_pane?.OutputString($"{message}\n");
		}

		public static void LogInfo(string message)
		{
			Log($"INFO: {message}");
		}

		public static void LogWarning(string message)
		{
			Log($"WARNING: {message}");
		}

		public static void LogError(string message)
		{
			Log($"ERROR: {message}");
		}
	}
}
