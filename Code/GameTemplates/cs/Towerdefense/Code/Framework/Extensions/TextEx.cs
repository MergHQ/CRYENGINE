using CryEngine.UI.Components;

namespace CryEngine.Framework
{
    public static class TextEx
    {
        public static void ApplyStyle(this Text text, TextStyle style)
        {
            text.DropsShadow = style.DropShadows;
            text.FontStyle = style.FontStyle;
            text.FontName = style.FontName;
            text.Color = style.Color;
            text.Height = style.Height;
        }
    }
}

