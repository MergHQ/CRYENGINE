start_sources()

set(SDL_MIXER_SDK_DIR "${SDK_DIR}/SDL_mixer")
set(SDL_MIXER_NATIVE_MIDI_DIR "${SDL_MIXER_SDK_DIR}/native_midi")
set(SDL_MIXER_TIMIDITY_DIR "${SDL_MIXER_SDK_DIR}/timidity")
set(SDL_MIXER_EXTERNALS_DIR "${SDL_MIXER_SDK_DIR}/external")
set(SDL_MIXER_FLAC_DIR "${SDL_MIXER_EXTERNALS_DIR}/flac-1.3.2")
set(SDL_MIXER_LIBMODPLUG_DIR "${SDL_MIXER_EXTERNALS_DIR}/libmodplug-0.8.9.0")
set(SDL_MIXER_LIBOGG_DIR "${SDL_MIXER_EXTERNALS_DIR}/libogg-1.3.2")
set(SDL_MIXER_LIBVORBIS_DIR "${SDL_MIXER_EXTERNALS_DIR}/libvorbis-1.3.5")
set(SDL_MIXER_LIBVORBISIDEC_DIR "${SDL_MIXER_EXTERNALS_DIR}/libvorbisidec-1.2.1")
set(SDL_MIXER_MPG123_DIR "${SDL_MIXER_EXTERNALS_DIR}/mpg123-1.25.6")
set(SDL_MIXER_OPUS_DIR "${SDL_MIXER_EXTERNALS_DIR}/opus-1.0.3")
set(SDL_MIXER_OPUSFILE_DIR "${SDL_MIXER_EXTERNALS_DIR}/opusfile-0.10")

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS flac
	SOURCE_GROUP "Root"
		"${SDL_MIXER_FLAC_DIR}/src/flac/analyze.c"
		"${SDL_MIXER_FLAC_DIR}/src/flac/decode.c"
		"${SDL_MIXER_FLAC_DIR}/src/flac/encode.c"
		"${SDL_MIXER_FLAC_DIR}/src/flac/foreign_metadata.c"
		"${SDL_MIXER_FLAC_DIR}/src/flac/iffscan.c"
		"${SDL_MIXER_FLAC_DIR}/src/flac/local_string_utils.c"
		"${SDL_MIXER_FLAC_DIR}/src/flac/main.c"
		"${SDL_MIXER_FLAC_DIR}/src/flac/utils.c"
		"${SDL_MIXER_FLAC_DIR}/src/flac/vorbiscomment.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/bitmath.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/bitreader.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/bitwriter.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/cpu.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/crc.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/fixed.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/fixed_intrin_sse2.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/fixed_intrin_ssse3.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/float.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/format.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/lpc.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/lpc_intrin_avx2.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/lpc_intrin_sse.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/lpc_intrin_sse2.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/lpc_intrin_sse41.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/md5.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/memory.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/metadata_iterators.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/metadata_object.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/ogg_decoder_aspect.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/ogg_encoder_aspect.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/ogg_helper.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/ogg_mapping.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/stream_decoder.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/stream_encoder.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/stream_encoder_framing.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/stream_encoder_intrin_avx2.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/stream_encoder_intrin_sse2.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/stream_encoder_intrin_ssse3.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/window.c"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC++/metadata.cpp"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC++/stream_decoder.cpp"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC++/stream_encoder.cpp"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/main.c"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/operations.c"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/operations_shorthand_cuesheet.c"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/operations_shorthand_picture.c"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/operations_shorthand_seektable.c"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/operations_shorthand_streaminfo.c"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/operations_shorthand_vorbiscomment.c"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/options.c"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/usage.c"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/utils.c"
		"${SDL_MIXER_FLAC_DIR}/src/flac/analyze.h"
		"${SDL_MIXER_FLAC_DIR}/src/flac/decode.h"
		"${SDL_MIXER_FLAC_DIR}/src/flac/encode.h"
		"${SDL_MIXER_FLAC_DIR}/src/flac/foreign_metadata.h"
		"${SDL_MIXER_FLAC_DIR}/src/flac/local_string_utils.h"
		"${SDL_MIXER_FLAC_DIR}/src/flac/utils.h"
		"${SDL_MIXER_FLAC_DIR}/src/flac/vorbiscomment.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/ia32/nasm.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/all.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/bitmath.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/bitreader.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/bitwriter.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/cpu.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/crc.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/fixed.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/float.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/format.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/lpc.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/macros.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/md5.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/memory.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/metadata.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/ogg_decoder_aspect.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/ogg_encoder_aspect.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/ogg_helper.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/ogg_mapping.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/stream_encoder.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/stream_encoder_framing.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/private/window.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/protected/all.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/protected/stream_decoder.h"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include/protected/stream_encoder.h"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/operations.h"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/operations_shorthand.h"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/options.h"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/usage.h"
		"${SDL_MIXER_FLAC_DIR}/src/metaflac/utils.h"
)

sources_platform(WINDOWS)
add_sources("NoUberFile"
	PROJECTS flac
	SOURCE_GROUP "Root"
		"${SDL_MIXER_FLAC_DIR}/src/libFLAC/windows_unicode_filenames.c"
		"${SDL_MIXER_FLAC_DIR}/src/share/win_utf8_io/win_utf8_io.c"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS libmodplug
	SOURCE_GROUP "Root"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/fastmix.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_669.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_abc.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_amf.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_ams.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_dbm.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_dmf.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_dsm.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_far.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_it.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_j2b.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_mdl.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_med.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_mid.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_mod.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_mt2.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_mtm.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_okt.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_pat.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_pat.h"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_psm.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_ptm.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_s3m.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_stm.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_ult.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_umx.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_wav.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/load_xm.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/mmcmp.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/modplug.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/modplug.h"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/snd_dsp.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/sndfile.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/snd_flt.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/snd_fx.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/sndmix.cpp"
		"${SDL_MIXER_LIBMODPLUG_DIR}/src/tables.h"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS libogg
	SOURCE_GROUP "Root"
		"${SDL_MIXER_LIBOGG_DIR}/src/bitwise.c"
		"${SDL_MIXER_LIBOGG_DIR}/src/framing.c"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS libvorbisfile
	SOURCE_GROUP "Root"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/vorbisfile.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/include/vorbis/vorbisfile.h"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS libvorbis
	SOURCE_GROUP "Root"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/analysis.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/bitrate.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/block.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/codebook.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/envelope.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/floor0.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/floor1.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/info.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/lookup.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/lpc.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/lsp.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/mapping0.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/mdct.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/psy.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/registry.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/res0.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/sharedbook.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/smallft.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/synthesis.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/vorbisenc.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/window.c"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/backends.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/bitrate.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/codebook.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/include/vorbis/codec.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/codec_internal.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/envelope.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/floor_all.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/books/floor/floor_books.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/highlevel.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/lookup.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/lookup_data.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/lpc.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/lsp.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/masking.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/mdct.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/misc.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/os.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/psy.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/psych_11.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/psych_16.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/psych_44.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/psych_8.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/registry.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/books/coupled/res_books_stereo.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/books/uncoupled/res_books_uncoupled.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/residue_16.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/residue_44.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/residue_44u.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/residue_8.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/scales.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/setup_11.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/setup_16.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/setup_22.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/setup_32.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/setup_44.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/setup_44u.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/setup_8.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/modes/setup_X.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/smallft.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/include/vorbis/vorbisenc.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/include/vorbis/vorbisfile.h"
		"${SDL_MIXER_LIBVORBIS_DIR}/lib/window.h"
)

sources_platform(WINDOWS)
add_sources("NoUberFile"
	PROJECTS mpg123
	SOURCE_GROUP "Root"
		"${SDL_MIXER_MPG123_DIR}/ports/MSVC++/msvc.c"
		"${SDL_MIXER_MPG123_DIR}/ports/MSVC++/config.h"
		"${SDL_MIXER_MPG123_DIR}/ports/MSVC++/mpg123.h"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS mpg123
	SOURCE_GROUP "Root"
		"${SDL_MIXER_MPG123_DIR}/src/compat/compat.c"
		"${SDL_MIXER_MPG123_DIR}/src/compat/compat_str.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/dct64.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/equalizer.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/feature.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/format.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/frame.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/icy.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/icy2utf8.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/id3.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/index.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/layer1.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/layer2.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/layer3.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/libmpg123.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/ntom.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/optimize.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/parse.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/readers.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/stringbuf.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/synth.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/synth_8bit.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/synth_real.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/synth_s32.c"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/tabinit.c"
		"${SDL_MIXER_MPG123_DIR}/src/compat/compat.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/debug.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/decode.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/frame.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/getbits.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/getcpuflags.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/huffman.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/icy.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/icy2utf8.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/id3.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/index.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/l2tables.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/mangle.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/mpg123lib_intern.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/optimize.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/parse.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/reader.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/sample.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/synth.h"
		"${SDL_MIXER_MPG123_DIR}/src/libmpg123/true.h"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS opus
	SOURCE_GROUP "Root"
		"${SDL_MIXER_OPUS_DIR}/src/opus.c"
		"${SDL_MIXER_OPUS_DIR}/src/opus_decoder.c"
		"${SDL_MIXER_OPUS_DIR}/src/opus_encoder.c"
		"${SDL_MIXER_OPUS_DIR}/src/opus_multistream.c"
		"${SDL_MIXER_OPUS_DIR}/src/repacketizer.c"
		"${SDL_MIXER_OPUS_DIR}/src/opus_private.h"
		"${SDL_MIXER_OPUS_DIR}/celt/arch.h"
		"${SDL_MIXER_OPUS_DIR}/celt/celt.h"
		"${SDL_MIXER_OPUS_DIR}/celt/entdec.h"
		"${SDL_MIXER_OPUS_DIR}/celt/entenc.h"
		"${SDL_MIXER_OPUS_DIR}/celt/float_cast.h"
		"${SDL_MIXER_OPUS_DIR}/celt/os_support.h"
		"${SDL_MIXER_OPUS_DIR}/celt/stack_alloc.h"
		"${SDL_MIXER_OPUS_DIR}/include/opus.h"
		"${SDL_MIXER_OPUS_DIR}/include/opus_defines.h"
		"${SDL_MIXER_OPUS_DIR}/include/opus_types.h"
		"${SDL_MIXER_OPUS_DIR}/include/opus_multistream.h"
)

sources_platform(WINDOWS)
add_sources("NoUberFile"
	PROJECTS opus
	SOURCE_GROUP "Root"
		"${SDL_MIXER_OPUS_DIR}/win32/config.h"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS opusfile
	SOURCE_GROUP "Root"
		"${SDL_MIXER_OPUSFILE_DIR}/src/internal.h"
		"${SDL_MIXER_OPUSFILE_DIR}/src/winerrno.h"
		"${SDL_MIXER_OPUSFILE_DIR}/src/http.c"
		"${SDL_MIXER_OPUSFILE_DIR}/src/info.c"
		"${SDL_MIXER_OPUSFILE_DIR}/src/internal.c"
		"${SDL_MIXER_OPUSFILE_DIR}/src/opusfile.c"
		"${SDL_MIXER_OPUSFILE_DIR}/src/stream.c"
		"${SDL_MIXER_OPUSFILE_DIR}/src/wincerts.c"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS celt
	SOURCE_GROUP "Root"
		"${SDL_MIXER_OPUS_DIR}/win32/config.h"
		"${SDL_MIXER_OPUS_DIR}/celt/arch.h"
		"${SDL_MIXER_OPUS_DIR}/celt/bands.h"
		"${SDL_MIXER_OPUS_DIR}/celt/celt.h"
		"${SDL_MIXER_OPUS_DIR}/celt/cwrs.h"
		"${SDL_MIXER_OPUS_DIR}/celt/ecintrin.h"
		"${SDL_MIXER_OPUS_DIR}/celt/entcode.h"
		"${SDL_MIXER_OPUS_DIR}/celt/entdec.h"
		"${SDL_MIXER_OPUS_DIR}/celt/entenc.h"
		"${SDL_MIXER_OPUS_DIR}/celt/fixed_debug.h"
		"${SDL_MIXER_OPUS_DIR}/celt/fixed_generic.h"
		"${SDL_MIXER_OPUS_DIR}/celt/float_cast.h"
		"${SDL_MIXER_OPUS_DIR}/celt/kiss_fft.h"
		"${SDL_MIXER_OPUS_DIR}/celt/laplace.h"
		"${SDL_MIXER_OPUS_DIR}/celt/mathops.h"
		"${SDL_MIXER_OPUS_DIR}/celt/mdct.h"
		"${SDL_MIXER_OPUS_DIR}/celt/modes.h"
		"${SDL_MIXER_OPUS_DIR}/celt/os_support.h"
		"${SDL_MIXER_OPUS_DIR}/celt/pitch.h"
		"${SDL_MIXER_OPUS_DIR}/celt/celt_lpc.h"
		"${SDL_MIXER_OPUS_DIR}/celt/quant_bands.h"
		"${SDL_MIXER_OPUS_DIR}/celt/rate.h"
		"${SDL_MIXER_OPUS_DIR}/celt/stack_alloc.h"
		"${SDL_MIXER_OPUS_DIR}/celt/vq.h"
		"${SDL_MIXER_OPUS_DIR}/celt/bands.c"
		"${SDL_MIXER_OPUS_DIR}/celt/celt.c"
		"${SDL_MIXER_OPUS_DIR}/celt/cwrs.c"
		"${SDL_MIXER_OPUS_DIR}/celt/entcode.c"
		"${SDL_MIXER_OPUS_DIR}/celt/entdec.c"
		"${SDL_MIXER_OPUS_DIR}/celt/entenc.c"
		"${SDL_MIXER_OPUS_DIR}/celt/kiss_fft.c"
		"${SDL_MIXER_OPUS_DIR}/celt/laplace.c"
		"${SDL_MIXER_OPUS_DIR}/celt/mathops.c"
		"${SDL_MIXER_OPUS_DIR}/celt/mdct.c"
		"${SDL_MIXER_OPUS_DIR}/celt/modes.c"
		"${SDL_MIXER_OPUS_DIR}/celt/pitch.c"
		"${SDL_MIXER_OPUS_DIR}/celt/celt_lpc.c"
		"${SDL_MIXER_OPUS_DIR}/celt/quant_bands.c"
		"${SDL_MIXER_OPUS_DIR}/celt/rate.c"
		"${SDL_MIXER_OPUS_DIR}/celt/vq.c"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS silk
	SOURCE_GROUP "Root"
		"${SDL_MIXER_OPUS_DIR}/silk/float/apply_sine_window_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/autocorrelation_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/burg_modified_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/bwexpander_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/corrMatrix_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/encode_frame_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/energy_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/find_LPC_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/find_LTP_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/find_pitch_lags_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/find_pred_coefs_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/inner_product_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/k2a_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/levinsondurbin_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/LPC_analysis_filter_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/LPC_inv_pred_gain_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/LTP_analysis_filter_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/LTP_scale_ctrl_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/noise_shape_analysis_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/pitch_analysis_core_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/prefilter_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/process_gains_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/regularize_correlations_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/residual_energy_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/scale_copy_vector_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/scale_vector_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/schur_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/solve_LS_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/sort_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/warped_autocorrelation_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/wrappers_FLP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/float/main_FLP.h"
		"${SDL_MIXER_OPUS_DIR}/silk/float/SigProc_FLP.h"
		"${SDL_MIXER_OPUS_DIR}/silk/float/structs_FLP.h"
		"${SDL_MIXER_OPUS_DIR}/silk/A2NLSF.c"
		"${SDL_MIXER_OPUS_DIR}/silk/ana_filt_bank_1.c"
		"${SDL_MIXER_OPUS_DIR}/silk/biquad_alt.c"
		"${SDL_MIXER_OPUS_DIR}/silk/bwexpander.c"
		"${SDL_MIXER_OPUS_DIR}/silk/bwexpander_32.c"
		"${SDL_MIXER_OPUS_DIR}/silk/check_control_input.c"
		"${SDL_MIXER_OPUS_DIR}/silk/CNG.c"
		"${SDL_MIXER_OPUS_DIR}/silk/code_signs.c"
		"${SDL_MIXER_OPUS_DIR}/silk/control_audio_bandwidth.c"
		"${SDL_MIXER_OPUS_DIR}/silk/control_codec.c"
		"${SDL_MIXER_OPUS_DIR}/silk/control_SNR.c"
		"${SDL_MIXER_OPUS_DIR}/silk/debug.c"
		"${SDL_MIXER_OPUS_DIR}/silk/decoder_set_fs.c"
		"${SDL_MIXER_OPUS_DIR}/silk/decode_core.c"
		"${SDL_MIXER_OPUS_DIR}/silk/decode_frame.c"
		"${SDL_MIXER_OPUS_DIR}/silk/decode_indices.c"
		"${SDL_MIXER_OPUS_DIR}/silk/decode_parameters.c"
		"${SDL_MIXER_OPUS_DIR}/silk/decode_pitch.c"
		"${SDL_MIXER_OPUS_DIR}/silk/decode_pulses.c"
		"${SDL_MIXER_OPUS_DIR}/silk/dec_API.c"
		"${SDL_MIXER_OPUS_DIR}/silk/encode_indices.c"
		"${SDL_MIXER_OPUS_DIR}/silk/encode_pulses.c"
		"${SDL_MIXER_OPUS_DIR}/silk/enc_API.c"
		"${SDL_MIXER_OPUS_DIR}/silk/gain_quant.c"
		"${SDL_MIXER_OPUS_DIR}/silk/HP_variable_cutoff.c"
		"${SDL_MIXER_OPUS_DIR}/silk/init_decoder.c"
		"${SDL_MIXER_OPUS_DIR}/silk/init_encoder.c"
		"${SDL_MIXER_OPUS_DIR}/silk/inner_prod_aligned.c"
		"${SDL_MIXER_OPUS_DIR}/silk/interpolate.c"
		"${SDL_MIXER_OPUS_DIR}/silk/lin2log.c"
		"${SDL_MIXER_OPUS_DIR}/silk/log2lin.c"
		"${SDL_MIXER_OPUS_DIR}/silk/LPC_analysis_filter.c"
		"${SDL_MIXER_OPUS_DIR}/silk/LPC_inv_pred_gain.c"
		"${SDL_MIXER_OPUS_DIR}/silk/LP_variable_cutoff.c"
		"${SDL_MIXER_OPUS_DIR}/silk/table_LSF_cos.c"
		"${SDL_MIXER_OPUS_DIR}/silk/NLSF2A.c"
		"${SDL_MIXER_OPUS_DIR}/silk/NLSF_decode.c"
		"${SDL_MIXER_OPUS_DIR}/silk/NLSF_del_dec_quant.c"
		"${SDL_MIXER_OPUS_DIR}/silk/NLSF_encode.c"
		"${SDL_MIXER_OPUS_DIR}/silk/NLSF_stabilize.c"
		"${SDL_MIXER_OPUS_DIR}/silk/NLSF_unpack.c"
		"${SDL_MIXER_OPUS_DIR}/silk/NLSF_VQ.c"
		"${SDL_MIXER_OPUS_DIR}/silk/NLSF_VQ_weights_laroia.c"
		"${SDL_MIXER_OPUS_DIR}/silk/NSQ.c"
		"${SDL_MIXER_OPUS_DIR}/silk/NSQ_del_dec.c"
		"${SDL_MIXER_OPUS_DIR}/silk/pitch_est_tables.c"
		"${SDL_MIXER_OPUS_DIR}/silk/PLC.c"
		"${SDL_MIXER_OPUS_DIR}/silk/process_NLSFs.c"
		"${SDL_MIXER_OPUS_DIR}/silk/quant_LTP_gains.c"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler.c"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler_down2.c"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler_down2_3.c"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler_private_AR2.c"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler_private_down_FIR.c"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler_private_IIR_FIR.c"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler_private_up2_HQ.c"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler_rom.c"
		"${SDL_MIXER_OPUS_DIR}/silk/shell_coder.c"
		"${SDL_MIXER_OPUS_DIR}/silk/sigm_Q15.c"
		"${SDL_MIXER_OPUS_DIR}/silk/sort.c"
		"${SDL_MIXER_OPUS_DIR}/silk/stereo_decode_pred.c"
		"${SDL_MIXER_OPUS_DIR}/silk/stereo_encode_pred.c"
		"${SDL_MIXER_OPUS_DIR}/silk/stereo_find_predictor.c"
		"${SDL_MIXER_OPUS_DIR}/silk/stereo_LR_to_MS.c"
		"${SDL_MIXER_OPUS_DIR}/silk/stereo_MS_to_LR.c"
		"${SDL_MIXER_OPUS_DIR}/silk/stereo_quant_pred.c"
		"${SDL_MIXER_OPUS_DIR}/silk/sum_sqr_shift.c"
		"${SDL_MIXER_OPUS_DIR}/silk/tables_gain.c"
		"${SDL_MIXER_OPUS_DIR}/silk/tables_LTP.c"
		"${SDL_MIXER_OPUS_DIR}/silk/tables_NLSF_CB_NB_MB.c"
		"${SDL_MIXER_OPUS_DIR}/silk/tables_NLSF_CB_WB.c"
		"${SDL_MIXER_OPUS_DIR}/silk/tables_other.c"
		"${SDL_MIXER_OPUS_DIR}/silk/tables_pitch_lag.c"
		"${SDL_MIXER_OPUS_DIR}/silk/tables_pulses_per_block.c"
		"${SDL_MIXER_OPUS_DIR}/silk/VAD.c"
		"${SDL_MIXER_OPUS_DIR}/silk/VQ_WMat_EC.c"
		"${SDL_MIXER_OPUS_DIR}/include/opus_types.h"
		"${SDL_MIXER_OPUS_DIR}/win32/config.h"
		"${SDL_MIXER_OPUS_DIR}/silk/control.h"
		"${SDL_MIXER_OPUS_DIR}/silk/debug.h"
		"${SDL_MIXER_OPUS_DIR}/silk/define.h"
		"${SDL_MIXER_OPUS_DIR}/silk/errors.h"
		"${SDL_MIXER_OPUS_DIR}/silk/Inlines.h"
		"${SDL_MIXER_OPUS_DIR}/silk/MacroCount.h"
		"${SDL_MIXER_OPUS_DIR}/silk/MacroDebug.h"
		"${SDL_MIXER_OPUS_DIR}/silk/macros.h"
		"${SDL_MIXER_OPUS_DIR}/silk/main.h"
		"${SDL_MIXER_OPUS_DIR}/silk/pitch_est_defines.h"
		"${SDL_MIXER_OPUS_DIR}/silk/PLC.h"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler_private.h"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler_rom.h"
		"${SDL_MIXER_OPUS_DIR}/silk/resampler_structs.h"
		"${SDL_MIXER_OPUS_DIR}/silk/API.h"
		"${SDL_MIXER_OPUS_DIR}/silk/SigProc_FIX.h"
		"${SDL_MIXER_OPUS_DIR}/silk/structs.h"
		"${SDL_MIXER_OPUS_DIR}/silk/tables.h"
		"${SDL_MIXER_OPUS_DIR}/silk/tuning_parameters.h"
		"${SDL_MIXER_OPUS_DIR}/silk/typedef.h"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS native_midi
	SOURCE_GROUP "Root"
		"${SDL_MIXER_NATIVE_MIDI_DIR}/native_midi.h"
		"${SDL_MIXER_NATIVE_MIDI_DIR}/native_midi_common.c"
		"${SDL_MIXER_NATIVE_MIDI_DIR}/native_midi_common.h"
		"${SDL_MIXER_NATIVE_MIDI_DIR}/native_midi_haiku.cpp"
		"${SDL_MIXER_NATIVE_MIDI_DIR}/native_midi_mac.c"
		"${SDL_MIXER_NATIVE_MIDI_DIR}/native_midi_macosx.c"
		"${SDL_MIXER_NATIVE_MIDI_DIR}/native_midi_win32.c"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS timidity
	SOURCE_GROUP "Root"
		"${SDL_MIXER_TIMIDITY_DIR}/common.c"
		"${SDL_MIXER_TIMIDITY_DIR}/common.h"
		"${SDL_MIXER_TIMIDITY_DIR}/instrum.c"
		"${SDL_MIXER_TIMIDITY_DIR}/instrum.h"
		"${SDL_MIXER_TIMIDITY_DIR}/mix.c"
		"${SDL_MIXER_TIMIDITY_DIR}/mix.h"
		"${SDL_MIXER_TIMIDITY_DIR}/options.h"
		"${SDL_MIXER_TIMIDITY_DIR}/output.c"
		"${SDL_MIXER_TIMIDITY_DIR}/output.h"
		"${SDL_MIXER_TIMIDITY_DIR}/playmidi.c"
		"${SDL_MIXER_TIMIDITY_DIR}/playmidi.h"
		"${SDL_MIXER_TIMIDITY_DIR}/readmidi.c"
		"${SDL_MIXER_TIMIDITY_DIR}/readmidi.h"
		"${SDL_MIXER_TIMIDITY_DIR}/resample.c"
		"${SDL_MIXER_TIMIDITY_DIR}/resample.h"
		"${SDL_MIXER_TIMIDITY_DIR}/tables.c"
		"${SDL_MIXER_TIMIDITY_DIR}/tables.h"
		"${SDL_MIXER_TIMIDITY_DIR}/timidity.c"
		"${SDL_MIXER_TIMIDITY_DIR}/timidity.h"
)

sources_platform(ALL)
add_sources("NoUberFile"
	PROJECTS sdl_mixer
	SOURCE_GROUP "Root"
		"${SDL_MIXER_SDK_DIR}/effect_position.c"
		"${SDL_MIXER_SDK_DIR}/effects_internal.c"
		"${SDL_MIXER_SDK_DIR}/effects_internal.h"
		"${SDL_MIXER_SDK_DIR}/effect_stereoreverse.c"
		"${SDL_MIXER_SDK_DIR}/load_aiff.c"
		"${SDL_MIXER_SDK_DIR}/load_aiff.h"
		"${SDL_MIXER_SDK_DIR}/load_voc.c"
		"${SDL_MIXER_SDK_DIR}/load_voc.h"
		"${SDL_MIXER_SDK_DIR}/mixer.c"
		"${SDL_MIXER_SDK_DIR}/mixer.h"
		"${SDL_MIXER_SDK_DIR}/music.c"
		"${SDL_MIXER_SDK_DIR}/music.h"
		"${SDL_MIXER_SDK_DIR}/music_cmd.c"
		"${SDL_MIXER_SDK_DIR}/music_cmd.h"
		"${SDL_MIXER_SDK_DIR}/music_flac.c"
		"${SDL_MIXER_SDK_DIR}/music_flac.h"
		"${SDL_MIXER_SDK_DIR}/music_fluidsynth.c"
		"${SDL_MIXER_SDK_DIR}/music_fluidsynth.h"
		"${SDL_MIXER_SDK_DIR}/music_mad.c"
		"${SDL_MIXER_SDK_DIR}/music_mad.h"
		"${SDL_MIXER_SDK_DIR}/music_mikmod.c"
		"${SDL_MIXER_SDK_DIR}/music_mikmod.h"
		"${SDL_MIXER_SDK_DIR}/music_modplug.c"
		"${SDL_MIXER_SDK_DIR}/music_modplug.h"
		"${SDL_MIXER_SDK_DIR}/music_mpg123.c"
		"${SDL_MIXER_SDK_DIR}/music_mpg123.h"
		"${SDL_MIXER_SDK_DIR}/music_nativemidi.c"
		"${SDL_MIXER_SDK_DIR}/music_nativemidi.h"
		"${SDL_MIXER_SDK_DIR}/music_ogg.c"
		"${SDL_MIXER_SDK_DIR}/music_ogg.h"
		"${SDL_MIXER_SDK_DIR}/music_opus.c"
		"${SDL_MIXER_SDK_DIR}/music_opus.h"
		"${SDL_MIXER_SDK_DIR}/music_timidity.c"
		"${SDL_MIXER_SDK_DIR}/music_timidity.h"
		"${SDL_MIXER_SDK_DIR}/music_wav.c"
		"${SDL_MIXER_SDK_DIR}/music_wav.h"
		"${SDL_MIXER_SDK_DIR}/playmus.c"
		"${SDL_MIXER_SDK_DIR}/playwave.c"
		"${SDL_MIXER_SDK_DIR}/SDL_mixer.h"
)

end_sources()

set(SDL_MIXER_SLN_DIR "Libs/Audio/SDL_mixer")

################### flac ###################
CryEngineStaticModule(flac SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_link_libraries(${THIS_PROJECT} PRIVATE libogg)
target_include_directories(${THIS_PROJECT} PRIVATE
	"${SDL_MIXER_FLAC_DIR}"
	"${SDL_MIXER_FLAC_DIR}/include"
	"${SDL_MIXER_FLAC_DIR}/src/libFLAC/include"
)
target_compile_definitions(${THIS_PROJECT} PRIVATE
	FLAC__ALIGN_MALLOC_DATA=1
	FLAC__HAS_OGG=1
	FLAC__USE_AVX=1
	VERSION="1.3.2"
)

if(WINDOWS)
	target_compile_definitions(${THIS_PROJECT} PUBLIC FLAC__NO_DLL)
	target_compile_definitions(${THIS_PROJECT} PRIVATE FLAC__CPU_X86_64=1)
	target_compile_options(${THIS_PROJECT} PRIVATE /EHsc /W0)
elseif(LINUX)
	target_compile_definitions(${THIS_PROJECT} PRIVATE
		FLAC__CPU_X86_64=1
		CPU_IS_BIG_ENDIAN=0
		CPU_IS_LITTLE_ENDIAN=1
		ENABLE_64_BIT_WORDS=0
		FLAC__HAS_X86INTRIN=1
		FLAC__SYS_LINUX=1
		HAVE_BSWAP16=1
		HAVE_BSWAP32=1
		HAVE_BYTESWAP_H=1
		HAVE_CLOCK_GETTIME=1
		HAVE_CPUID_H=1
		HAVE_CXX_VARARRAYS=1
		HAVE_C_VARARRAYS=1
		HAVE_DLFCN_H=1
		HAVE_FSEEKO=1
		HAVE_GETOPT_LONG=1
		HAVE_ICONV=1
		HAVE_INTTYPES_H=1
		HAVE_LANGINFO_CODESET=1
		HAVE_LROUND=1
		HAVE_MEMORY_H=1
		HAVE_SOCKLEN_T=1
		HAVE_STDINT_H=1
		HAVE_STDLIB_H=1
		HAVE_STRINGS_H=1
		HAVE_STRING_H=1
		HAVE_SYS_IOCTL_H=1
		HAVE_SYS_PARAM_H=1
		HAVE_SYS_STAT_H=1
		HAVE_SYS_TYPES_H=1
		HAVE_TERMIOS_H=1
		HAVE_TYPEOF=1
		HAVE_UNISTD_H=1
		HAVE_X86INTRIN_H=1
		ICONV_CONST
		LT_OBJDIR=".libs/"
		PACKAGE="flac"
		PACKAGE_BUGREPORT="flac-dev@xiph.org"
		PACKAGE_NAME="flac"
		PACKAGE_STRING="flac 1.3.2"
		PACKAGE_TARNAME="flac"
		PACKAGE_URL="https://www.xiph.org/flac/"
		PACKAGE_VERSION="1.3.2"
		SIZEOF_OFF_T=8
		SIZEOF_VOIDP=8
		STDC_HEADERS=1
		WORDS_BIGENDIAN=0
)
	target_compile_options(${THIS_PROJECT} PRIVATE
		-w
		-frtti
)
elseif(ANDROID)
	target_compile_definitions(${THIS_PROJECT} PRIVATE
		CPU_IS_BIG_ENDIAN=0
		CPU_IS_LITTLE_ENDIAN=1
		ENABLE_64_BIT_WORDS=0
		FLAC__HAS_X86INTRIN=1
		FLAC__SYS_LINUX=1
		HAVE_BSWAP16=1
		HAVE_BSWAP32=1
		HAVE_BYTESWAP_H=1
		HAVE_CLOCK_GETTIME=1
		HAVE_CXX_VARARRAYS=1
		HAVE_C_VARARRAYS=1
		HAVE_DLFCN_H=1
		HAVE_FSEEKO=1
		HAVE_GETOPT_LONG=1
		HAVE_ICONV=1
		HAVE_INTTYPES_H=1
		HAVE_LANGINFO_CODESET=1
		HAVE_LROUND=1
		HAVE_MEMORY_H=1
		HAVE_SOCKLEN_T=1
		HAVE_STDINT_H=1
		HAVE_STDLIB_H=1
		HAVE_STRINGS_H=1
		HAVE_STRING_H=1
		HAVE_SYS_IOCTL_H=1
		HAVE_SYS_PARAM_H=1
		HAVE_SYS_STAT_H=1
		HAVE_SYS_TYPES_H=1
		HAVE_TERMIOS_H=1
		HAVE_TYPEOF=1
		HAVE_UNISTD_H=1
		HAVE_X86INTRIN_H=1
		ICONV_CONST
		LT_OBJDIR=".libs/"
		PACKAGE="flac"
		PACKAGE_BUGREPORT="flac-dev@xiph.org"
		PACKAGE_NAME="flac"
		PACKAGE_STRING="flac 1.3.2"
		PACKAGE_TARNAME="flac"
		PACKAGE_URL="https://www.xiph.org/flac/"
		PACKAGE_VERSION="1.3.2"
		SIZEOF_OFF_T=8
		SIZEOF_VOIDP=8
		STDC_HEADERS=1
		WORDS_BIGENDIAN=0
)
	target_compile_options(${THIS_PROJECT} PRIVATE
		-w
		-frtti
)
endif()

################### libmodplug ###################
CryEngineStaticModule(libmodplug SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_include_directories(${THIS_PROJECT} PRIVATE
	"${SDL_MIXER_LIBMODPLUG_DIR}/config"
	"${SDL_MIXER_LIBMODPLUG_DIR}/src"
	"${SDL_MIXER_LIBMODPLUG_DIR}/src/libmodplug"
)

target_compile_definitions(${THIS_PROJECT} PRIVATE MODPLUG_HEADER="modplug.h")
set_property(TARGET ${THIS_PROJECT} PROPERTY OUTPUT_NAME "modplug")

target_compile_definitions(${THIS_PROJECT} PUBLIC
	MODPLUG_STATIC=1
	MODPLUG_BUILD=1
)

target_compile_definitions(${THIS_PROJECT} PRIVATE
	HAVE_CMAKE_CONFIG_H=1
	HAVE_MALLOC_H=1
	HAVE_INTTYPES_H=1
	HAVE_STDINT_H=1
	_REENTRANT=1
	_USE_MATH_DEFINES=1
)

if(WINDOWS)
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX OR ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${THIS_PROJECT} PRIVATE -Wno-register)
endif()

##################### libogg ######################
CryEngineStaticModule(libogg SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_include_directories(${THIS_PROJECT} PUBLIC "${SDL_MIXER_LIBOGG_DIR}/include")
set_property(TARGET ${THIS_PROJECT} PROPERTY OUTPUT_NAME "ogg")

if(WINDOWS)
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX)
	target_compile_definitions(${THIS_PROJECT} PRIVATE
		HAVE_DLFCN_H=1
		HAVE_INTTYPES_H=1
		HAVE_MEMORY_H=1
		HAVE_STDINT_H=1
		HAVE_STDLIB_H=1
		HAVE_STRINGS_H=1
		HAVE_STRING_H=1
		HAVE_SYS_STAT_H=1
		HAVE_SYS_TYPES_H=1
		HAVE_UNISTD_H=1
		STDC_HEADERS=1
)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
elseif (ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
	target_compile_definitions(${THIS_PROJECT} PRIVATE
		HAVE_DLFCN_H=1
		HAVE_INTTYPES_H=1
		HAVE_MEMORY_H=1
		HAVE_STDINT_H=1
		HAVE_STDLIB_H=1
		HAVE_STRINGS_H=1
		HAVE_STRING_H=1
		HAVE_SYS_STAT_H=1
		HAVE_SYS_TYPES_H=1
		HAVE_UNISTD_H=1
		STDC_HEADERS=1
	)
	target_link_libraries(${THIS_PROJECT} PRIVATE m)
endif()

################# libvorbis ##################
CryEngineStaticModule(libvorbis SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_link_libraries(${THIS_PROJECT} PRIVATE libogg)
set_property(TARGET ${THIS_PROJECT} PROPERTY OUTPUT_NAME "vorbis")
target_include_directories(${THIS_PROJECT} PUBLIC "${SDL_MIXER_LIBVORBIS_DIR}/include")

if(WINDOWS)
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX OR ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
	target_include_directories(${THIS_PROJECT} PRIVATE "${SDL_MIXER_LIBVORBIS_DIR}/lib")
endif()

################### libvorbisfile ###################
CryEngineStaticModule(libvorbisfile SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_link_libraries(${THIS_PROJECT} PRIVATE libogg libvorbis)
set_property(TARGET ${THIS_PROJECT} PROPERTY OUTPUT_NAME "vorbisfile")

if(WINDOWS)
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX OR ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
	target_include_directories(${THIS_PROJECT} PRIVATE "${SDL_MIXER_LIBVORBIS_DIR}/lib")
endif()

##################### mpg123 ######################
CryEngineStaticModule(mpg123 SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_compile_definitions(${THIS_PROJECT} PRIVATE OPT_MULTI OPT_GENERIC)
target_include_directories(${THIS_PROJECT} PRIVATE
	"${SDL_MIXER_MPG123_DIR}/src/compat"
	"${SDL_MIXER_MPG123_DIR}/src"
	"${SDL_MIXER_MPG123_DIR}/src/libmpg123"
)

if(WINDOWS)
	target_link_libraries(${THIS_PROJECT} PRIVATE Shlwapi)
	target_include_directories(${THIS_PROJECT} PRIVATE "${SDL_MIXER_MPG123_DIR}/ports/MSVC++")
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX OR ANDROID)
	# This include directory as well as config.h and mpg123.h need to be re-created in the event of an update!
	target_include_directories(${THIS_PROJECT} PRIVATE "${SDL_MIXER_MPG123_DIR}/ports/Linux")
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
endif()

##################### opus ######################
CryEngineStaticModule(opus SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_link_libraries(${THIS_PROJECT} PRIVATE celt silk)
target_include_directories(${THIS_PROJECT} PUBLIC "${SDL_MIXER_OPUS_DIR}/include")
target_include_directories(${THIS_PROJECT} PRIVATE
	"${SDL_MIXER_OPUS_DIR}/celt"
	"${SDL_MIXER_OPUS_DIR}/silk"
)

target_compile_definitions(${THIS_PROJECT} PRIVATE
	USE_ALLOCA=1
	OPUS_ENABLE_ENCODER=1
	OPUS_BUILD=1
)

if(WINDOWS)
	target_include_directories(${THIS_PROJECT} PRIVATE "${SDL_MIXER_OPUS_DIR}/win32")
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX OR ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
endif()

##################### opusfile ######################
CryEngineStaticModule(opusfile SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_link_libraries(${THIS_PROJECT} PRIVATE libogg opus)
target_include_directories(${THIS_PROJECT} PRIVATE "${SDL_MIXER_OPUSFILE_DIR}/include")

if(WINDOWS)
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX OR ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
endif()

##################### celt ######################
CryEngineStaticModule(celt SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_include_directories(${THIS_PROJECT} PRIVATE
	"${SDL_MIXER_OPUS_DIR}/include"
)

target_compile_definitions(${THIS_PROJECT} PRIVATE OPUS_BUILD)

if(WINDOWS)
	target_include_directories(${THIS_PROJECT} PRIVATE
		"${SDL_MIXER_OPUS_DIR}/win32"
)
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)

	target_compile_definitions(${THIS_PROJECT} PRIVATE
		USE_ALLOCA=1
		OPUS_ENABLE_ENCODER=1
)
elseif(LINUX OR ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)

	target_compile_definitions(${THIS_PROJECT} PRIVATE
		HAVE_ALLOCA_H=1
		HAVE_DLFCN_H=1
		HAVE_GETOPT_H=1
		HAVE_INTTYPES_H=1
		HAVE_LRINT=1
		HAVE_LRINTF=1
		HAVE_MEMORY_H=1
		HAVE_STDINT_H=1
		HAVE_STDLIB_H=1
		HAVE_STRINGS_H=1
		HAVE_STRING_H=1
		HAVE_SYS_STAT_H=1
		HAVE_SYS_TYPES_H=1
		HAVE___MALLOC_HOOK=1
		LT_OBJDIR=".libs/"
		PACKAGE_BUGREPORT="opus@xiph.org"
		PACKAGE_NAME="opus"
		PACKAGE_STRING="opus 1.0.3"
		PACKAGE_TARNAME="opus"
		PACKAGE_URL=""
		PACKAGE_VERSION="1.0.3"
		SIZEOF_INT=4
		SIZEOF_LONG=8
		SIZEOF_LONG_LONG=8
		SIZEOF_SHORT=2
		STDC_HEADERS=1
		VAR_ARRAYS
		restrict=__restrict
)
endif()

##################### silk ######################
CryEngineStaticModule(silk SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_include_directories(${THIS_PROJECT} PRIVATE
	"${SDL_MIXER_OPUS_DIR}/include"
	"${SDL_MIXER_OPUS_DIR}/celt"
	"${SDL_MIXER_OPUS_DIR}/win32"
	"${SDL_MIXER_OPUS_DIR}/silk"
	"${SDL_MIXER_OPUS_DIR}/silk/float"
)

target_compile_definitions(${THIS_PROJECT} PRIVATE
	USE_ALLOCA=1
	OPUS_ENABLE_ENCODER=1
	OPUS_BUILD=1
)

if(WINDOWS)
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX OR ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
endif()

#################### native_midi ####################
CryEngineStaticModule(native_midi SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_include_directories(${THIS_PROJECT} PRIVATE "${SDL_MIXER_NATIVE_MIDI_DIR}")
target_link_libraries(${THIS_PROJECT} PRIVATE SDL2)

if(WINDOWS)
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX OR ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
endif()

##################### timidity ######################
CryEngineStaticModule(timidity SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_include_directories(${THIS_PROJECT} PRIVATE "${SDL_MIXER_TIMIDITY_DIR}")
target_link_libraries(${THIS_PROJECT} PRIVATE SDL2)

if(WINDOWS)
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX OR ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
endif()

################# sdl_mixer ##################
CryEngineStaticModule(sdl_mixer SOLUTION_FOLDER ${SDL_MIXER_SLN_DIR})

target_include_directories(${THIS_PROJECT} PUBLIC "${SDL_MIXER_SDK_DIR}")
target_link_libraries(${THIS_PROJECT} PUBLIC
	SDL2
)
target_link_libraries(${THIS_PROJECT} PRIVATE
	flac
	libmodplug
	libvorbisfile
	mpg123
	opusfile
	timidity
	native_midi
)

if(WINDOWS)
	target_compile_definitions(${THIS_PROJECT} PRIVATE
		MUSIC_WAV
		MUSIC_FLAC
		MUSIC_MOD_MODPLUG
		MUSIC_OGG
		MUSIC_OPUS
		MUSIC_MP3_MPG123
		MUSIC_MID_TIMIDITY
		MUSIC_MID_NATIVE
)
	target_include_directories(${THIS_PROJECT} PRIVATE "${SDL_MIXER_SDK_DIR}/VisualC/external/include")
	target_link_libraries(${THIS_PROJECT} PRIVATE Winmm)
	target_compile_options(${THIS_PROJECT} PRIVATE /W0)
elseif(LINUX OR ANDROID)
	target_compile_options(${THIS_PROJECT} PRIVATE -w)
endif()

unset(SDL_MIXER_SDK_DIR)
unset(SDL_MIXER_NATIVE_MIDI_DIR)
unset(SDL_MIXER_TIMIDITY_DIR)
unset(SDL_MIXER_EXTERNALS_DIR)
unset(SDL_MIXER_FLAC_DIR)
unset(SDL_MIXER_SLN_DIR)
unset(SDL_MIXER_LIBMODPLUG_DIR)
unset(SDL_MIXER_LIBOGG_DIR)
unset(SDL_MIXER_LIBVORBIS_DIR)
unset(SDL_MIXER_LIBVORBISIDEC_DIR)
unset(SDL_MIXER_MPG123_DIR)
unset(SDL_MIXER_OPUS_DIR)
unset(SDL_MIXER_OPUSFILE_DIR)
