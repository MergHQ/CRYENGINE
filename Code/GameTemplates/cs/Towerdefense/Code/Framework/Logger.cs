using System;

namespace CryEngine.Framework
{
    public class Logger
    {
        readonly string prefix;

        public string Prefix { get; set; } = "[";
        public string Postfix { get; set; } = "]";

        public bool Enabled { get; set; } = true;

        public Logger(string prefix = "")
        {
            if (!string.IsNullOrEmpty(prefix))
                this.prefix = Prefix + prefix + Postfix + " ";
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
            return prefix + message;
        }
    }
}

