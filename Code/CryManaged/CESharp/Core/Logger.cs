namespace CryEngine
{
    public class Logger
    {
        private readonly string _prefix;

        public string Prefix { get; set; } = "[";
        public string Postfix { get; set; } = "]";

        public bool Enabled { get; set; } = true;

        public Logger(string prefix = "")
        {
            if (!string.IsNullOrEmpty(prefix))
                this._prefix = Prefix + prefix + Postfix + " ";
        }

        public void LogInfo(string message, params object[] args)
        {
            if (Enabled)
                Log.Info(Format(message), args);
        }

        public void LogWarning(string message, params object[] args)
        {
            if (Enabled)
                Log.Warning(Format(message), args);
        }

        public void LogError(string message, params object[] args)
        {
            if (Enabled)
                Log.Error(Format(message), args);
        }

        string Format(string message)
        {
            return _prefix + message;
        }
    }
}

