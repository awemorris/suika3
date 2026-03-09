all: constants.o dictionary.o shared_dictionary.o context.o platform.o transform.o bit_reader.o decode.o huffman.o state.o transform.o
	$(AR) rcs $(PREFIX)/lib/libbrotlicommon.a constants.o dictionary.o shared_dictionary.o context.o platform.o
	$(AR) rcs $(PREFIX)/lib/libbrotlidec.a bit_reader.o decode.o huffman.o state.o

constants.o: c/common/constants.c
	$(CC) -I./c/include $(CFLAGS) -c c/common/constants.c

dictionary.o: c/common/dictionary.c
	$(CC) -I./c/include $(CFLAGS) -c c/common/dictionary.c

shared_dictionary.o: c/common/shared_dictionary.c
	$(CC) -I./c/include $(CFLAGS) -c c/common/shared_dictionary.c

context.o: c/common/context.c
	$(CC) -I./c/include $(CFLAGS) -c c/common/context.c

platform.o: c/common/platform.c
	$(CC) -I./c/include $(CFLAGS) -c c/common/platform.c

transform.o: c/common/transform.c
	$(CC) -I./c/include $(CFLAGS) -c c/common/transform.c

bit_reader.o: c/dec/bit_reader.c
	$(CC) -I./c/include $(CFLAGS) -c c/dec/bit_reader.c

decode.o: c/dec/decode.c
	$(CC) -I./c/include $(CFLAGS) -c c/dec/decode.c

huffman.o: c/dec/huffman.c
	$(CC) -I./c/include $(CFLAGS) -c c/dec/huffman.c

state.o: c/dec/state.c
	$(CC) -I./c/include $(CFLAGS) -c c/dec/state.c
