// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine;
using CryEngine.UI.Components;
using System;
using System.Runtime.Serialization;

using System.IO;
using CryEngine.Resources;

namespace CryEngine.UI
{
    /// <summary>
    /// Determines a buffer which is applied to make a rect smaller.
    /// </summary>
    [DataContract]
    public class Padding
    {
        [DataMember(Name = "L")]
        public float Left; ///< The left buffer.
		[DataMember(Name = "T")]
        public float Top; ///< The top buffer.
		[DataMember(Name = "R")]
        public float Right; ///< The right buffer.
		[DataMember(Name = "B")]
        public float Bottom; ///< The bottom buffer.

        public Point TopLeft { get { return new Point(Left, Top); } set { Left = value.x; Top = value.y; } } ///< The upper left buffer.

        /// <summary>
        /// Constructs Padding with one default value for all buffers.
        /// </summary>
        /// <param name="def">Default buffer value.</param>
        public Padding(float def = 0)
        {
            Top = Left = Right = Bottom = def;
        }

        /// <summary>
        /// Only initializes upper left buffer. lower right will be 0.
        /// </summary>
        /// <param name="left">Left buffer value.</param>
        /// <param name="top">Top buffer value.</param>
        public Padding(float left, float top)
        {
            Top = top; Left = left;
        }

        /// <summary>
        /// Initializes Padding with specific values for all buffers.
        /// </summary>
        /// <param name="left">Left buffer value.</param>
        /// <param name="top">Top buffer value.</param>
        /// <param name="right">Right buffer value.</param>
        /// <param name="bottom">Bottom buffer value.</param>
        public Padding(float left, float top, float right, float bottom)
        {
            Top = top; Left = left; Right = right; Bottom = bottom;
        }

        /// <summary>
        /// Returns a <see cref="System.String"/> that represents the current <see cref="CryEngine.UI.Padding"/>.
        /// </summary>
        /// <returns>A <see cref="System.String"/> that represents the current <see cref="CryEngine.UI.Padding"/>.</returns>
        public override string ToString()
        {
            return Left.ToString("0") + "," + Top.ToString("0") + "," + Right.ToString("0") + "," + Bottom.ToString("0");
        }
    }

    /// <summary>
    /// Alignment for Layout computation of UIElements. UIElements will be centered around an alignment target.
    /// </summary>
    public enum Alignment
    {
        Center,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left,
        TopLeft,
        TopHStretch,
        //CenterHStretch,
        BottomHStretch,
        //LeftVStretch,
        //CenterVStretch,
        RightVStretch,
        Stretch
    }

    /// <summary>
    /// Base class for all UI elements. Provides RectTransform for Layout computation.
    /// </summary>
    [DataContract]
    public class UIElement : SceneObject
    {
        public static string DataDirectory { get { return Path.Combine(FileSystem.DataDirectory, "libs/ui"); } } ///< Path where UI System textures are stored.

        public RectTransform RectTransform { get; private set; } ///< Defines and computes layout in "d space, for this UIElement.

        /// <summary>
        /// Checks if cursor is inside a UIElement, considering its Bounds.
        /// </summary>
        public bool IsCursorInside { get { return RectTransform.Bounds.Contains(Mouse.CursorPosition); } }

        /// <summary>
        /// Creates this element along with a RectTransform.
        /// </summary>
        public UIElement()
        {
            RectTransform = AddComponent<RectTransform>();
        }

        /// <summary>
        /// Returns ths hierarchically predecessing Canvas object for this element.
        /// </summary>
        /// <returns>The parent Canvas.</returns>
        public Canvas FindParentCanvas()
        {
            if (this is Canvas)
                return this as Canvas;
            return Parent is UIElement ? (Parent as UIElement).FindParentCanvas() : null;
        }
    }
}
