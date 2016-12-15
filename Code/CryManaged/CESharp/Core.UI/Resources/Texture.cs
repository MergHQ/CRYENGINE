// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Attributes;
using CryEngine.Common;
using CryEngine.UI;
using System;
using System.Runtime.Serialization;

namespace CryEngine.Resources
{
    /// <summary>
    /// Defines how a texture is supposed draw itself. Options are draw in one piece (None), or draw equally sized slices in horizontal and/or vertical directrion.
    /// </summary>
    public enum SliceType
    {
        None,
        ThreeHorizontal,
        ThreeVertical,
        Nine
    }

    /// <summary>
    /// Wrapper for CryEngine ITexture. Holds, creates and updates an ITexture instance.
    /// </summary>
    [DataContract]
    public class UITexture
    {
        public Canvas TargetCanvas { get; set; }

        ITexture _ceTex;
        [DataMember]
        bool _isFiltered;
        int _texId;
        bool _isRendertarget;

        [HideFromInspector, DataMember]
        public bool RoundLocation { get; set; } ///< Indicates whether draw location is rounded up.

        public Color Color { get; set; } ///< Color to multiply the texture with on draw.
		public int Width { get; private set; } ///< Texture Width.
		public int Height { get; private set; } ///< Texture Height.
		public float Angle { get; set; } ///< Rotation angle of the texture.
		public Rect ClampRect = null; ///< Determines which area the texture can be drawn in. Texture will be clamped outside the edges of this area if not null.
		public string Name { get { return _ceTex.GetName(); } } ///< Describing name for the dexture. Set to filename if loaded from file.
		public int ID { get { return _texId; } } ///< ID retrieved from CryEngine on texture creation.

        /// <summary>
        /// Creates a texture with image data and sets some default properties.
        /// </summary>
        /// <param name="width">Intended Texture Width.</param>
        /// <param name="height">Intended Texture Height.</param>
        /// <param name="data">Image source data.</param>
        /// <param name="isFiltered">If set to <c>true</c> texture is filtered.</param>
        /// <param name="roundLocation">If set to <c>true</c> texture start location is rounded up on draw.</param>
        /// <param name="isRendertarget">If set to <c>true</c> Texture can be rendered to.</param>
        public UITexture(int width, int height, Byte[] data, bool isFiltered = true, bool roundLocation = false, bool isRendertarget = false)
        {
            Color = Color.White;
            RoundLocation = roundLocation;
            _isFiltered = isFiltered;
            _isRendertarget = isRendertarget;
            CreateTexture(width, height, data);
        }

        /// <summary>
        /// Pushes new image data into video memory for the allocated texture object. May reinstantiate the texture object, e.g. if size changed.
        /// </summary>
        /// <param name="width">Target Width.</param>
        /// <param name="height">Target Height.</param>
        /// <param name="data">Image data for the texture to be filled with.</param>
        public void UpdateData(int width, int height, Byte[] data)
        {
            /*if (width == Width && height == Height)
			{
				Global.gEnv.pRenderer.UpdateTextureInVideoMemory ((uint)_ceTex.GetTextureID (), data, 0, 0, Width, Height);
			}
			else*/
            {
                Destroy();
                CreateTexture(width, height, data);
            }
        }

        /// <summary>
        /// Creates a new texture object and fills it with given image data.
        /// </summary>
        /// <param name="width">Target Width.</param>
        /// <param name="height">Target Height.</param>
        /// <param name="data">Image data for the texture to be filled with.</param>
        void CreateTexture(int width, int height, byte[] data)
        {
            Width = width;
            Height = height;
            var flags = (int)ETextureFlags.FT_NOMIPS;
            if (_isRendertarget)
                flags = (int)ETextureFlags.FT_DONT_STREAM | (int)ETextureFlags.FT_DONT_RELEASE | (int)ETextureFlags.FT_USAGE_RENDERTARGET;
            _ceTex = Global.gEnv.pRenderer.CreateTexture("MF", Width, Height, 1, data, (byte)ETEX_Format.eTF_R8G8B8A8, flags);
            _ceTex.SetClamp(false);
            _ceTex.SetHighQualityFiltering(_isFiltered);

            _texId = _ceTex.GetTextureID();
        }

        void DrawSection(float x, float y, float w, float h, float u0, float v0, float u1, float v1)
        {
            float crx = x, cry = y, crw = w, crh = h;
            if (ClampRect != null)
            {
                crx = ClampRect.x;
                cry = ClampRect.y;
                crw = ClampRect.w;
                crh = ClampRect.h;
            }

            if (ClampRect == null || (crx <= x && cry <= y && crx + crw >= x + w && cry + crh >= y + h))
            {
                if (RoundLocation) { x = (int)x; y = (int)y; }
                TargetCanvas.PushTexturePart(x, y, w, h, ID, u0, v0, u1, v1, Angle, Color);
            }
            else
            {
                if (crx >= x + w || crx + crw <= x || cry >= y + h || cry + crh <= y || crw <= 0 || crh <= 0)
                    return;

                float cx = x, cy = y, cw = w, ch = h, cu0 = u0, cv0 = v0, cu1 = u1, cv1 = v1;
                if (crx > x)
                {
                    cu0 = u0 + (u1 - u0) * (crx - x) / w;
                    cx = crx;
                    cw = cw - (crx - x);
                }
                if (crx + crw < x + w)
                {
                    cu1 = u1 - (u1 - u0) * (x + w - crx - crw) / w;
                    cw = crx + crw - cx;
                }
                if (cry > y)
                {
                    cv0 = v0 + (v1 - v0) * (cry - y) / h;
                    cy = cry;
                    ch = ch - (cry - y);
                }
                if (cry + crh < y + h)
                {
                    cv1 = v1 - (v1 - v0) * (y + h - cry - crh) / h;
                    ch = cry + crh - cy;
                }
                if (RoundLocation) { cx = (int)cx; cy = (int)cy; }
                TargetCanvas.PushTexturePart(cx, cy, cw, ch, ID, cu0, cv0, cu1, cv1, Angle, Color);
            }
        }

        /// <summary>
        /// Draws the current texture object, in a given rectangular area.
        /// </summary>
        /// <param name="x">The x coordinate.</param>
        /// <param name="y">The y coordinate.</param>
        /// <param name="w">The Width.</param>
        /// <param name="h">The Height.</param>
        /// <param name="sliceType">SliceType to be used.</param>
        public void Draw(float x, float y, float w, float h, SliceType sliceType = SliceType.None)
        {
            float sx = ((float)Width / 3.0f);
            float sy = ((float)Height / 3.0f);

            switch (sliceType)
            {
                case SliceType.None:
                    DrawSection(x, y, w, h, 0, 1, 1, 0);
                    break;

                case SliceType.Nine:
                    DrawSection(x, y, sx, sy, 0, 1, 0.333f, 0.666f);
                    DrawSection(x + sx, y, w - sx * 2, sy, 0.333f, 1, 0.666f, 0.666f);
                    DrawSection(x + w - sx, y, sx, sy, 0.666f, 1, 1, 0.666f);

                    DrawSection(x, y + sy, sx, h - sy * 2, 0, 0.666f, 0.333f, 0.333f);
                    DrawSection(x + sx, y + sy, w - sx * 2, h - sy * 2, 0.333f, 0.666f, 0.666f, 0.333f);
                    DrawSection(x + w - sx, y + sy, sx, h - sy * 2, 0.666f, 0.666f, 1, 0.333f);

                    DrawSection(x, y + h - sy, sx, sy, 0, 0.333f, 0.333f, 0);
                    DrawSection(x + sx, y + h - sy, w - sx * 2, sy, 0.333f, 0.333f, 0.666f, 0);
                    DrawSection(x + w - sx, y + h - sy, sx, sy, 0.666f, 0.333f, 1, 0);
                    break;

                case SliceType.ThreeHorizontal:
                    DrawSection(x, y, sx, h, 0, 1, 0.333f, 0);
                    DrawSection(x + sx, y, w - sx * 2, h, 0.333f, 1, 0.666f, 0);
                    DrawSection(x + w - sx, y, sx, h, 0.666f, 1, 1, 0);
                    break;

                case SliceType.ThreeVertical:
                    DrawSection(x, y, w, sy, 0, 1, 1, 0.666f);
                    DrawSection(x, y + sy, w, h - sy * 2, 0, 0.666f, 1, 0.333f);
                    DrawSection(x, y + h - sy, w, sy, 0, 0.333f, 1, 0);
                    break;
            }
        }

        /// <summary>
        /// Destroy this texture instance.
        /// </summary>
        public void Destroy()
        {
            if (_ceTex != null)
            {
                /*Global.gEnv.pRenderer.RemoveTexture ((uint)_ceTex.GetTextureID ());
				_ceTex.Dispose ();*/
                _ceTex = null;
            }
        }
    }
}
