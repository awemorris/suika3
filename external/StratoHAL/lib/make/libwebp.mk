all: alpha_dec.o buffer_dec.o frame_dec.o idec_dec.o io_dec.o quant_dec.o tree_dec.o vp8_dec.o vp8l_dec.o webp_dec.o anim_decode.o demux.o alpha_processing.o cost.o cost_neon.o cost_sse2.o cpu.o dec.o dec_neon.o dec_sse2.o dec_sse41.o dec_clip_tables.o enc.o filters.o filters_neon.o filters_sse2.o lossless.o lossless_neon.o lossless_sse2.o lossless_sse41.o lossless_enc.o lossless_enc_neon.o lossless_enc_sse2.o lossless_enc_sse41.o rescaler.o rescaler_neon.o rescaler_sse2.o ssim.o ssim_sse2.o upsampling.o upsampling_neon.o upsampling_sse2.o upsampling_sse41.o yuv.o yuv_neon.o yuv_sse2.o yuv_sse41.o alpha_enc.o analysis_enc.o backward_references_cost_enc.o backward_references_enc.o config_enc.o cost_enc.o filter_enc.o frame_enc.o histogram_enc.o iterator_enc.o near_lossless_enc.o picture_csp_enc.o picture_enc.o picture_psnr_enc.o picture_rescale_enc.o picture_tools_enc.o predictor_enc.o quant_enc.o syntax_enc.o token_enc.o tree_enc.o vp8l_enc.o webp_enc.o anim_encode.o muxedit.o muxinternal.o muxread.o bit_reader_utils.o bit_writer_utils.o color_cache_utils.o filters_utils.o huffman_encode_utils.o huffman_utils.o quant_levels_dec_utils.o quant_levels_utils.o random_utils.o rescaler_utils.o thread_utils.o utils.o
	$(AR) rcs $(PREFIX)/lib/libwebp.a *.o

alpha_dec.o: src/dec/alpha_dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

buffer_dec.o: src/dec/buffer_dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

frame_dec.o: src/dec/frame_dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

idec_dec.o: src/dec/idec_dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

io_dec.o: src/dec/io_dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

quant_dec.o: src/dec/quant_dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

tree_dec.o: src/dec/tree_dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

vp8_dec.o: src/dec/vp8_dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

vp8l_dec.o: src/dec/vp8l_dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

webp_dec.o: src/dec/webp_dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

anim_decode.o: src/demux/anim_decode.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

demux.o: src/demux/demux.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

alpha_processing.o: src/dsp/alpha_processing.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

cost.o: src/dsp/cost.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

cost_neon.o: src/dsp/cost_neon.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

cost_sse2.o: src/dsp/cost_sse2.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

cpu.o: src/dsp/cpu.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

dec.o: src/dsp/dec.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

dec_neon.o: src/dsp/dec_neon.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

dec_sse2.o: src/dsp/dec_sse2.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

dec_sse41.o: src/dsp/dec_sse41.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

dec_clip_tables.o: src/dsp/dec_clip_tables.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

enc.o: src/dsp/enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

filters.o: src/dsp/filters.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

filters_neon.o: src/dsp/filters_neon.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

filters_sse2.o: src/dsp/filters_sse2.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

lossless.o: src/dsp/lossless.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

lossless_neon.o: src/dsp/lossless_neon.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

lossless_sse2.o: src/dsp/lossless_sse2.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

lossless_sse41.o: src/dsp/lossless_sse41.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

lossless_enc.o: src/dsp/lossless_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

lossless_enc_neon.o: src/dsp/lossless_enc_neon.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

lossless_enc_sse2.o: src/dsp/lossless_enc_sse2.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

lossless_enc_sse41.o: src/dsp/lossless_enc_sse41.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

rescaler.o: src/dsp/rescaler.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

rescaler_neon.o: src/dsp/rescaler_neon.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

rescaler_sse2.o: src/dsp/rescaler_sse2.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

ssim.o: src/dsp/ssim.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

ssim_sse2.o: src/dsp/ssim_sse2.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

upsampling.o: src/dsp/upsampling.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

upsampling_neon.o: src/dsp/upsampling_neon.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

upsampling_sse2.o: src/dsp/upsampling_sse2.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

upsampling_sse41.o: src/dsp/upsampling_sse41.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

yuv.o: src/dsp/yuv.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

yuv_neon.o: src/dsp/yuv_neon.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

yuv_sse2.o: src/dsp/yuv_sse2.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

yuv_sse41.o: src/dsp/yuv_sse41.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

alpha_enc.o: src/enc/alpha_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

analysis_enc.o: src/enc/analysis_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

backward_references_cost_enc.o: src/enc/backward_references_cost_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

backward_references_enc.o: src/enc/backward_references_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

config_enc.o: src/enc/config_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

cost_enc.o: src/enc/cost_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

filter_enc.o: src/enc/filter_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

frame_enc.o: src/enc/frame_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

histogram_enc.o: src/enc/histogram_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

iterator_enc.o: src/enc/iterator_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

near_lossless_enc.o: src/enc/near_lossless_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

picture_csp_enc.o: src/enc/picture_csp_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

picture_enc.o: src/enc/picture_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

picture_psnr_enc.o: src/enc/picture_psnr_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

picture_rescale_enc.o: src/enc/picture_rescale_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

picture_tools_enc.o: src/enc/picture_tools_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

predictor_enc.o: src/enc/predictor_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

quant_enc.o: src/enc/quant_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

syntax_enc.o: src/enc/syntax_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

token_enc.o: src/enc/token_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

tree_enc.o: src/enc/tree_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

vp8l_enc.o: src/enc/vp8l_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

webp_enc.o: src/enc/webp_enc.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

anim_encode.o: src/mux/anim_encode.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

muxedit.o: src/mux/muxedit.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

muxinternal.o: src/mux/muxinternal.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

muxread.o: src/mux/muxread.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

bit_reader_utils.o: src/utils/bit_reader_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

bit_writer_utils.o: src/utils/bit_writer_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

color_cache_utils.o: src/utils/color_cache_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

filters_utils.o: src/utils/filters_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

huffman_encode_utils.o: src/utils/huffman_encode_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

huffman_utils.o: src/utils/huffman_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

quant_levels_dec_utils.o: src/utils/quant_levels_dec_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

quant_levels_utils.o: src/utils/quant_levels_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

random_utils.o: src/utils/random_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

rescaler_utils.o: src/utils/rescaler_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

thread_utils.o: src/utils/thread_utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<

utils.o: src/utils/utils.c
	$(CC) -I. -I./src $(CFLAGS) -c $<
