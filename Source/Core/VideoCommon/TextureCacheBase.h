// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include <map>

#include "Common/CommonTypes.h"
#include "Common/Thread.h"

#include "VideoCommon/BPMemory.h"
#include "VideoCommon/TextureDecoder.h"
#include "VideoCommon/VideoCommon.h"

struct VideoConfig;

class TextureCache
{
public:
	enum TexCacheEntryType
	{
		TCET_NORMAL,
		TCET_EC_VRAM,    // EFB copy which sits in VRAM and is ready to be used
		TCET_EC_DYNAMIC, // EFB copy which sits in RAM and needs to be decoded before being used
	};

	struct TCacheEntryBase
	{
		TCacheEntryBase(u32 width, u32 height, u32 _maxlevel, bool _efbcopy)
		: virtual_width(width), virtual_height(height), maxlevel(_maxlevel), efbcopy(_efbcopy)
		{}

		// common members
		u32 addr;
		u32 size_in_bytes;
		u32 tlut_addr;
		u32 tlut_size;
		u64 hash;
		u32 format;

		enum TexCacheEntryType type;

		u32 native_width, native_height; // Texture dimensions from the GameCube's point of view
		u32 native_maxlevel;

		const u32 virtual_width, virtual_height; // Texture dimensions from OUR point of view - for hires textures or scaled EFB copies
		const u32 maxlevel;
		const bool efbcopy;

		// used to delete textures which haven't been used for TEXTURE_KILL_THRESHOLD frames
		int frameCount;

		void SetGeneralParameters(u32 _addr, u32 _size, u32 _tlut_addr, u32 _tlut_size, u32 _format)
		{
			addr = _addr;
			size_in_bytes = _size;
			tlut_addr = _tlut_addr;
			tlut_size = _tlut_size;
			format = _format;
		}

		void SetDimensions(u32 _native_width, u32 _native_height, u32 _native_maxlevel)
		{
			native_width = _native_width;
			native_height = _native_height;
			native_maxlevel = _native_maxlevel;
		}

		void SetHashes(u64 _hash)
		{
			hash = _hash;
		}


		virtual ~TCacheEntryBase();

		virtual void Bind(unsigned int stage) = 0;
		virtual bool Save(const std::string& filename, unsigned int level) = 0;

		virtual void Load(unsigned int width, unsigned int height,
			unsigned int expanded_width, unsigned int level) = 0;

		bool OverlapsMemoryRange(u32 range_address, u32 range_size) const;

		bool IsEfbCopy() { return (type == TCET_EC_VRAM || type == TCET_EC_DYNAMIC) && efbcopy; }
	};

	virtual ~TextureCache(); // needs virtual for DX11 dtor

	static void OnConfigChanged(VideoConfig& config);

	// Removes textures which aren't used for more than TEXTURE_KILL_THRESHOLD frames,
	// frameCount is the current frame number.
	static void Cleanup(int frameCount);

	static void Invalidate();
	static void MakeRangeDynamic(u32 start_address, u32 size);
	static bool Find(u32 start_address, u64 hash);

	virtual TCacheEntryBase* CreateTexture(unsigned int width, unsigned int height,
		unsigned int expanded_width, unsigned int maxlevel, PC_TexFormat pcfmt) = 0;
	virtual TCacheEntryBase* CreateRenderTargetTexture(unsigned int scaled_tex_w, unsigned int scaled_tex_h) = 0;

	virtual void FromRenderTargetToTexture(TCacheEntryBase* entry, PEControl::PixelFormat srcFormat,
		const EFBRectangle& srcRect, bool scaleByHalf, unsigned int cbufid, const float *colmat) = 0;
	virtual size_t FromRenderTargetToRam(u8* dst, unsigned int dstFormat, PEControl::PixelFormat srcFormat,
		const EFBRectangle& srcRect, bool isIntensity, bool scaleByHalf) = 0;

	static TCacheEntryBase* Load(unsigned int stage, u32 address, unsigned int width, unsigned int height,
		int format, unsigned int tlutaddr, int tlutfmt, bool use_mipmaps, unsigned int maxlevel, bool from_tmem);
	static void CopyRenderTargetToTexture(u32 dstAddr, unsigned int dstFormat, PEControl::PixelFormat srcFormat,
		const EFBRectangle& srcRect, bool isIntensity, bool scaleByHalf);

	static void RequestInvalidateTextureCache();

protected:
	TextureCache();

	static  GC_ALIGNED16(u8 *temp);
	static unsigned int temp_size;

private:
	static void DumpTexture(TCacheEntryBase* entry, unsigned int level);

	typedef std::map<u32, TCacheEntryBase*> TexCache;

	static TexCache textures;

	// Backup configuration values
	static struct BackupConfig
	{
		int s_colorsamples;
		bool s_texfmt_overlay;
		bool s_texfmt_overlay_center;
		bool s_hires_textures;
	} backup_config;
};

extern TextureCache *g_texture_cache;
