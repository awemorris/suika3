all: pngget.o pngread.o pngrutil.o pngtrans.o pngwtran.o png.o pngmem.o pngrio.o pngset.o pngwio.o pngwutil.o pngerror.o pngpread.o pngrtran.o pngwrite.o
	$(AR) rcs $(PREFIX)/lib/libpng.a pngget.o pngread.o pngrutil.o pngtrans.o pngwtran.o png.o pngmem.o pngrio.o pngset.o pngwio.o pngwutil.o pngerror.o pngpread.o pngrtran.o pngwrite.o

pngget.o: pngget.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngread.o: pngread.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngrutil.o: pngrutil.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngtrans.o: pngtrans.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngwtran.o: pngwtran.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

png.o: png.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngmem.o: pngmem.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngrio.o: pngrio.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngset.o: pngset.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngwio.o: pngwio.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngwutil.o: pngwutil.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngerror.o: pngerror.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngpread.o: pngpread.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngrtran.o: pngrtran.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<

pngwrite.o: pngwrite.c
	$(CC) -I. -I./src -I$(PREFIX)/include $(CFLAGS) -DPNG_ARM_NEON_OPT=0 -c $<
