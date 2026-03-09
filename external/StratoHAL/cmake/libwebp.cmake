file(ARCHIVE_EXTRACT
  INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/libwebp-1.3.2.tar.gz
  DESTINATION ${CMAKE_BINARY_DIR}
)

file(GLOB LIBPNG_EXTRACTED_DIR ${CMAKE_BINARY_DIR}/libwebp-*)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/libwebp)
file(RENAME ${LIBPNG_EXTRACTED_DIR} ${CMAKE_BINARY_DIR}/libwebp)

add_library(webp OBJECT
  ${CMAKE_BINARY_DIR}/libwebp/src/dec/alpha_dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dec/buffer_dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dec/frame_dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dec/idec_dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dec/io_dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dec/quant_dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dec/tree_dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dec/vp8_dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dec/vp8l_dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dec/webp_dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/demux/anim_decode.c
  ${CMAKE_BINARY_DIR}/libwebp/src/demux/demux.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/alpha_processing.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/alpha_processing_neon.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/alpha_processing_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/alpha_processing_sse41.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/cost.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/cost_neon.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/cost_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/cpu.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/dec.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/dec_neon.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/dec_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/dec_sse41.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/dec_clip_tables.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/enc_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/enc_sse41.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/enc_neon.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/filters.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/filters_neon.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/filters_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/lossless.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/lossless_neon.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/lossless_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/lossless_sse41.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/lossless_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/lossless_enc_neon.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/lossless_enc_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/lossless_enc_sse41.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/rescaler.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/rescaler_neon.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/rescaler_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/ssim.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/ssim_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/upsampling.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/upsampling_neon.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/upsampling_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/upsampling_sse41.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/yuv.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/yuv_neon.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/yuv_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/src/dsp/yuv_sse41.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/alpha_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/analysis_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/backward_references_cost_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/backward_references_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/config_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/cost_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/filter_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/frame_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/histogram_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/iterator_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/near_lossless_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/picture_csp_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/picture_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/picture_psnr_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/picture_rescale_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/picture_tools_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/predictor_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/quant_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/syntax_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/token_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/tree_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/vp8l_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/enc/webp_enc.c
  ${CMAKE_BINARY_DIR}/libwebp/src/mux/anim_encode.c
  ${CMAKE_BINARY_DIR}/libwebp/src/mux/muxedit.c
  ${CMAKE_BINARY_DIR}/libwebp/src/mux/muxinternal.c
  ${CMAKE_BINARY_DIR}/libwebp/src/mux/muxread.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/bit_reader_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/bit_writer_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/color_cache_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/filters_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/huffman_encode_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/huffman_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/quant_levels_dec_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/quant_levels_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/random_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/rescaler_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/thread_utils.c
  ${CMAKE_BINARY_DIR}/libwebp/src/utils/utils.c
  ${CMAKE_BINARY_DIR}/libwebp/sharpyuv/sharpyuv.c
  ${CMAKE_BINARY_DIR}/libwebp/sharpyuv/sharpyuv_cpu.c
  ${CMAKE_BINARY_DIR}/libwebp/sharpyuv/sharpyuv_dsp.c
  ${CMAKE_BINARY_DIR}/libwebp/sharpyuv/sharpyuv_csp.c
  ${CMAKE_BINARY_DIR}/libwebp/sharpyuv/sharpyuv_gamma.c
  ${CMAKE_BINARY_DIR}/libwebp/sharpyuv/sharpyuv_sse2.c
  ${CMAKE_BINARY_DIR}/libwebp/sharpyuv/sharpyuv_neon.c
)

target_include_directories(webp PRIVATE ${CMAKE_BINARY_DIR}/libwebp)
target_include_directories(webp PUBLIC  ${CMAKE_BINARY_DIR}/libwebp/src)
set(WEBP_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/libwebp/src)

# Suppress compilation errors.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(webp PRIVATE -std=gnu99 -w)
elseif(MSVC)
  target_compile_options(webp PRIVATE /W0 /wd4267 /wd4334)
endif()
