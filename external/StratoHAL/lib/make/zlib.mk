all: adler32.o infback.o trees.o compress.o inffast.o uncompr.o crc32.o inflate.o deflate.o inftrees.o zutil.o
	$(AR) rcs $(PREFIX)/lib/libz.a adler32.o infback.o trees.o compress.o inffast.o uncompr.o crc32.o inflate.o deflate.o inftrees.o zutil.o

adler32.o: adler32.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<

infback.o: infback.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<

trees.o: trees.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<

compress.o: compress.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<

inffast.o: inffast.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<

uncompr.o: uncompr.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<

crc32.o: crc32.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<

inflate.o: inflate.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<

deflate.o: deflate.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<

inftrees.o: inftrees.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<

zutil.o: zutil.c
	$(CC) -I. -I./src $(CFLAGS) -std=c89 -w -c $<
