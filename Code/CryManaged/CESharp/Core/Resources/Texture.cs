// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

using System.Runtime.Serialization;
using CryEngine.Common;

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
	/// Defines a region in pixels.
	/// </summary>
	public struct Region
	{
		public int X { get; set; }
		public int Y { get; set; }
		public int Width { get; set; }
		public int Height { get; set; }

		/// <summary>
		/// Initializes a new instance of the <see cref="T:CryEngine.Resources.Region"/> struct.
		/// </summary>
		/// <param name="x">The x coordinate.</param>
		/// <param name="y">The y coordinate.</param>
		/// <param name="width">Width.</param>
		/// <param name="height">Height.</param>
		public Region(int x, int y, int width, int height)
		{
			X = x;
			Y = y;
			Width = width;
			Height = height;
		}

		internal RectI ToNativeRect()
		{
			return new RectI(X, Y, Width, Height);
		}

		/// <summary>
		/// Returns a <see cref="T:System.String"/> that represents the current <see cref="T:CryEngine.Resources.Region"/>.
		/// </summary>
		/// <returns>A <see cref="T:System.String"/> that represents the current <see cref="T:CryEngine.Resources.Region"/>.</returns>
		public override string ToString()
		{
			return string.Format("[Region: X={0}, Y={1}, Width={2}, Height={3}]", X, Y, Width, Height);
		}
	}

	/// <summary>
	/// Wrapper for CryEngine ITexture. Holds, creates and updates an ITexture instance.
	/// </summary>
	public class Texture
	{
		private ITexture _nativeTexture;

		private int _texId;
		private bool _isRenderTarget;
		private bool _isClamped = false;
		private string _originalName = "MF";

		/// <summary>
		/// Texture Width.
		/// </summary>
		/// <value>The width.</value>
		//public int Width { get; private set; }
		public int Width { get { return _nativeTexture.GetWidth(); } }
		/// <summary>
		/// Texture Height.
		/// </summary>
		/// <value>The height.</value>
		//public int Height { get; private set; }
		public int Height { get { return _nativeTexture.GetHeight(); } }

		/// <summary>
		/// Describing name for the texture. Automatically set to the filename if loaded from file.
		/// </summary>
		/// <value>The name.</value>
		public string Name { get { return _nativeTexture.GetName(); } }

		/// <summary>
		/// ID retrieved from CryEngine on texture creation.
		/// </summary>
		/// <value>The identifier.</value>
		public int ID { get { return _texId; } }

		/// <summary>
		/// Get a texture by name.
		/// </summary>
		/// <returns>The by name.</returns>
		/// <param name="name">Name.</param>
		public static Texture GetByName(string name)
		{
			Texture texture = null;
			var nativeTexture = Global.gEnv.pRenderer.EF_GetTextureByName(name);
			if(nativeTexture != null)
			{
				texture = new Texture(nativeTexture);
			}
			return texture;
		}

		/// <summary>
		/// Get a texture by ID.
		/// </summary>
		/// <returns>The by identifier.</returns>
		/// <param name="Id">Identifier.</param>
		public static Texture GetById(int Id)
		{
			Texture texture = null;
			var nativeTexture = Global.gEnv.pRenderer.EF_GetTextureByID(Id);
			if(nativeTexture != null)
			{
				texture = new Texture(nativeTexture);
			}
			return texture;
		}

		private Texture(ITexture nativeTexture)
		{
			_isRenderTarget = ((ETextureFlags)nativeTexture.GetFlags()).HasFlag(ETextureFlags.FT_USAGE_RENDERTARGET);
			_texId = nativeTexture.GetTextureID();
			nativeTexture.AddRef();
		}

		/// <summary>
		/// Creates a texture with image data and sets some default properties.
		/// </summary>
		/// <param name="width">Intended Texture Width.</param>
		/// <param name="height">Intended Texture Height.</param>
		/// <param name="data">Image source data.</param>
		/// <param name="isRendertarget">If set to <c>true</c> Texture can be rendered to.</param>
		/// <param name="clamp">If set to <c>true</c> the Texture will be clamped at the edges.</param>
		/// <param name="name">The name the texture will receive internally. If the name is not unique a number will be appended to it.</param>
		public Texture(int width, int height, byte[] data, bool isRendertarget = false, bool clamp = false, string name = "MF")
		{
			_originalName = name;
			_isRenderTarget = isRendertarget;
			CreateTexture(width, height, data, clamp);
		}

		/// <summary>
		/// Releases unmanaged resources and performs other cleanup operations before the
		/// <see cref="T:CryEngine.Resources.Texture"/> is reclaimed by garbage collection.
		/// </summary>
		~Texture()
		{
			if(_nativeTexture != null)
			{
				_nativeTexture.Release();
			}
		}

		/// <summary>
		/// Pushes new image data into video memory for the allocated texture object. May reinstantiate the texture object, e.g. if size changed.
		/// </summary>
		/// <param name="width">Target Width.</param>
		/// <param name="height">Target Height.</param>
		/// <param name="data">Image data for the texture to be filled with.</param>
		public void UpdateData(int width, int height, byte[] data)
		{
			if (width == Width && height == Height && _texId != 0)
			{
				Global.gEnv.pRenderer.UpdateTextureInVideoMemory((uint)_texId, data, 0, 0, Width, Height, ETEX_Format.eTF_R8G8B8A8);
			}
			else
			{
				Destroy();
				CreateTexture(width, height, data, _isClamped);
			}
		}

		/// <summary>
		/// Creates a new texture object and fills it with given image data.
		/// </summary>
		/// <param name="width">Target Width.</param>
		/// <param name="height">Target Height.</param>
		/// <param name="data">Image data for the texture to be filled with.</param>
		/// <param name="clamp">If set to <c>true</c> the texture will be clamped at the edges.</param>
		private void CreateTexture(int width, int height, byte[] data, bool clamp)
		{
			var flags = (int)ETextureFlags.FT_NOMIPS;
			if(_isRenderTarget)
			{
				flags |= (int)ETextureFlags.FT_DONT_STREAM | (int)ETextureFlags.FT_DONT_RELEASE | (int)ETextureFlags.FT_USAGE_RENDERTARGET;
			}
			_nativeTexture = Global.gEnv.pRenderer.CreateTexture(_originalName, width, height, 1, data, (byte)ETEX_Format.eTF_R8G8B8A8, flags);
			_nativeTexture.SetClamp(clamp);
			_isClamped = clamp;
			_nativeTexture.AddRef();

			_texId = _nativeTexture.GetTextureID();
		}

		/// <summary>
		/// Copy part of <paramref name="sourceTexture"/> to <paramref name="destinationTexture"/>, 
		/// overwriting all pixels in the region.
		/// The regions to copy from and to are specified with  <paramref name="sourceRegion"/> and  <paramref name="destinationRegion"/>.
		/// To copy and blend transparency use <see cref="BlendTextureRegion"/> instead.
		/// </summary>
		/// <param name="sourceTexture">Source texture.</param>
		/// <param name="sourceRegion">Source region.</param>
		/// <param name="destinationTexture">Destination texture.</param>
		/// <param name="destinationRegion">Destination region.</param>
		/// <param name="color">Tint color.</param>
		public static void CopyTextureRegion(Texture sourceTexture, Region sourceRegion, Texture destinationTexture, Region destinationRegion, Color color)
		{
			if(sourceTexture == null)
			{
				throw new System.ArgumentNullException(nameof(sourceTexture));
			}

			if(destinationTexture == null)
			{
				throw new System.ArgumentNullException(nameof(destinationTexture));
			}

			Global.gEnv.pRenderer?.CopyTextureRegion(sourceTexture._nativeTexture, 
			                                        sourceRegion.ToNativeRect(), 
			                                        destinationTexture._nativeTexture,
			                                        destinationRegion.ToNativeRect(),
			                                        color,
			                                        0);
		}

		/// <summary>
		/// Copy part of <paramref name="sourceTexture"/> to <paramref name="destinationTexture"/> 
		/// and blend the transparency while copying.
		/// The regions to copy from and to are specified with  <paramref name="sourceRegion"/> and  <paramref name="destinationRegion"/>.
		/// To copy without blending transparency use <see cref="CopyTextureRegion"/> instead.
		/// </summary>
		/// <param name="sourceTexture">Source texture.</param>
		/// <param name="sourceRegion">Source region.</param>
		/// <param name="destinationTexture">Destination texture.</param>
		/// <param name="destinationRegion">Destination region.</param>
		/// <param name="color">Tint color.</param>
		public static void BlendTextureRegion(Texture sourceTexture, Region sourceRegion, Texture destinationTexture, Region destinationRegion, Color color)
		{
			if(sourceTexture == null)
			{
				throw new System.ArgumentNullException(nameof(sourceTexture));
			}

			if(destinationTexture == null)
			{
				throw new System.ArgumentNullException(nameof(destinationTexture));
			}

			Global.gEnv.pRenderer?.CopyTextureRegion(sourceTexture._nativeTexture, 
			                                         sourceRegion.ToNativeRect(), 
			                                         destinationTexture._nativeTexture,
			                                         destinationRegion.ToNativeRect(), 
			                                         color, 
			                                         Global.OS_ALPHA_BLEND | Global.GS_BLDST_ONEMINUSSRCALPHA | Global.GS_BLSRC_SRCALPHA);
		}

		/// <summary>
		/// Enable clamping the Texture at the edges instead of wrapping.
		/// </summary>
		/// <param name="isClamped">If set to <c>true</c> is clamped.</param>
		public void SetClamped(bool isClamped)
		{
			if(_nativeTexture == null)
			{
				throw new System.NullReferenceException(string.Format("The value of {0} is null!", nameof(Texture)));
			}

			_isClamped = isClamped;
			_nativeTexture.SetClamp(isClamped);
		}

		/// <summary>
		/// Clears the texture and sets all pixels to the default clear color.
		/// </summary>
		public void Clear()
		{
			_nativeTexture.Clear();
		}

		/// <summary>
		/// Clears the texture and sets all pixels to the <paramref name="clearColor"/>
		/// </summary>
		/// <param name="clearColor">The color the texture will be set to.</param>
		public void Clear(Color clearColor)
		{
			_nativeTexture.Clear(clearColor);
		}

		/// <summary>
		/// Destroy this texture instance.
		/// </summary>
		public void Destroy()
		{
			if(_nativeTexture != null && _texId != 0)
			{
				var id = _texId;
				var isRenderTarget = _isRenderTarget;
				GameFramework.AddDestroyAction(() => DestroyNativeTexture(id, isRenderTarget));
				_nativeTexture.Dispose();
				_nativeTexture = null;
				_texId = 0;
			}
		}

		private void DestroyNativeTexture(int textureId, bool isRenderTarget)
		{
			var texture = Global.gEnv.pRenderer.EF_GetTextureByID(textureId);
			if(texture == null)
			{
				return;
			}
			if(!isRenderTarget)
			{
				texture.Release();
			}
			else
			{
				texture.ReleaseForce();
			}
			Global.gEnv.pRenderer.RemoveTexture((uint)textureId);

			texture.Dispose();
		}

		protected static bool IsNull(Texture wrapper)
		{
			var isNull = ReferenceEquals(wrapper, null);
			return isNull ? isNull : (wrapper._nativeTexture == null);
		}

		public static bool operator ==(Texture wrapperA, Texture wrapperB)
		{
			var aIsNull = IsNull(wrapperA);
			var bIsNull = IsNull(wrapperB);

			if (aIsNull && bIsNull)
			{
				return true;
			}

			if (aIsNull != bIsNull)
			{
				return false;
			}

			return ReferenceEquals(wrapperA, wrapperB);
		}

		public static bool operator !=(Texture wrapperA, Texture wrapperB)
		{
			return !(wrapperA == wrapperB);
		}

		public override bool Equals(object obj)
		{
			if (obj == null && IsNull(this))
			{
				return true;
			}
			return ReferenceEquals(this, obj);
		}

		public bool Equals(Texture wrapper)
		{
			if (IsNull(this) && IsNull(wrapper))
			{
				return true;
			}

			return ReferenceEquals(wrapper, this);
		}

		public override int GetHashCode()
		{
			if (IsNull(this))
			{
				throw new System.NullReferenceException();
			}
			return base.GetHashCode();
		}
	}
}
