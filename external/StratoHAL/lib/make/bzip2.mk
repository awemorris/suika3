all: compile
	$(AR) rcs $(PREFIX)/lib/libbz2.a blocksort.o compress.o decompress.o huffman.o randtable.o unzcrash.o bzlib.o crctable.o

compile: unzcrash.o blocksort.o compress.o decompress.o huffman.o randtable.o bzlib.o crctable.o

unzcrash.o:
	$(CC) -I./c/include $(CFLAGS) -c -Wno-pointer-sign unzcrash.c

blocksort.o:
	$(CC) -I./c/include $(CFLAGS) -c -Wno-pointer-sign blocksort.c

compress.o:
	$(CC) -I./c/include $(CFLAGS) -c -Wno-pointer-sign compress.c

decompress.o:
	$(CC) -I./c/include $(CFLAGS) -c -Wno-pointer-sign decompress.c

huffman.o:
	$(CC) -I./c/include $(CFLAGS) -c -Wno-pointer-sign huffman.c

randtable.o:
	$(CC) -I./c/include $(CFLAGS) -c -Wno-pointer-sign randtable.c

bzlib.o:
	$(CC) -I./c/include $(CFLAGS) -c -Wno-pointer-sign bzlib.c

crctable.o:
	$(CC) -I./c/include $(CFLAGS) -c -Wno-pointer-sign crctable.c
