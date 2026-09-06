all: libvorbis.a libvorbisfile.a

libvorbis.a: analysis.o bitrate.o block.o codebook.o envelope.o floor0.o floor1.o info.o lookup.o lpc.o lsp.o mapping0.o mdct.o psy.o registry.o res0.o sharedbook.o smallft.o synthesis.o vorbisenc.o window.o
	$(AR) rcs $(PREFIX)/lib/libvorbis.a analysis.o bitrate.o block.o codebook.o envelope.o floor0.o floor1.o info.o lookup.o lpc.o lsp.o mapping0.o mdct.o psy.o registry.o res0.o sharedbook.o smallft.o synthesis.o vorbisenc.o window.o

libvorbisfile.a: vorbisfile.o
	$(AR) rcs $(PREFIX)/lib/libvorbisfile.a vorbisfile.o

analysis.o: lib/analysis.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

bitrate.o: lib/bitrate.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

block.o: lib/block.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

codebook.o: lib/codebook.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

envelope.o: lib/envelope.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

floor0.o: lib/floor0.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

floor1.o: lib/floor1.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

info.o: lib/info.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

lookup.o: lib/lookup.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

lpc.o: lib/lpc.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

lsp.o: lib/lsp.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

mapping0.o: lib/mapping0.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

mdct.o: lib/mdct.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

psy.o: lib/psy.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

registry.o: lib/registry.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

res0.o: lib/res0.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

sharedbook.o: lib/sharedbook.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

smallft.o: lib/smallft.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

synthesis.o: lib/synthesis.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

tone.o: lib/tone.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

vorbisenc.o: lib/vorbisenc.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

vorbisfile.o: lib/vorbisfile.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<

window.o: lib/window.c
	$(CC) -I./include -I./lib -I$(PREFIX)/include $(CFLAGS) -c $<
