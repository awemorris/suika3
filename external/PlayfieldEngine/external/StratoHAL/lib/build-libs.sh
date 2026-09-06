#!/bin/sh

set -eu

MACHINE=$1
CC=$2
CFLAGS=$3
AR=$4
LIBPATH=$5
NPROC=16

if [ ! -z $MACHINE ]; then
    PREFIX=`pwd`/libroot-$MACHINE;
else
    PREFIX=`pwd`/libroot;
fi

TAR=tar
if [ ! -z "`which gtar`" ]; then
    TAR=gtar;
fi

MAKE=make
if [ ! -z "`which gmake`" ]; then
    MAKE=gmake;
fi

if [ ! -z "`which nproc`" ]; then
    NPROC=`nproc`;
fi

rm -rf tmp $PREFIX
mkdir -p tmp $PREFIX
mkdir -p $PREFIX/include $PREFIX/lib

cd tmp

echo 'Building brotli...'
$TAR xzf "$LIBPATH/archive/brotli-1.1.0.tar.gz"
cd brotli-1.1.0
$MAKE -j$NPROC -f "$LIBPATH/make/brotli.mk" CC="$CC" CFLAGS="$CFLAGS -w" AR="$AR" PREFIX="$PREFIX"
cp -R c/include/brotli $PREFIX/include/
cd ..

echo 'Building bzip2...'
$TAR xzf "$LIBPATH/archive/bzip2-1.0.6.tar.gz"
cd bzip2-1.0.6
$MAKE -j$NPROC -f "$LIBPATH/make/bzip2.mk" CC="$CC" CFLAGS="$CFLAGS -w" AR="$AR" PREFIX="$PREFIX"
cp bzlib.h $PREFIX/include/
cd ..

echo 'Building libwebp...'
$TAR xzf "$LIBPATH/archive/libwebp-1.3.2.tar.gz"
cd libwebp-1.3.2
sed \
    -e 's|#define WEBP_USE_NEON|#undef WEBP_USE_NEON|g' \
    -e 's|#define WEBP_USE_SSE2|#undef WEBP_USE_SSE2|g' \
    < src/dsp/cpu.h \
    > src/dsp/cpu.h.new
mv src/dsp/cpu.h.new src/dsp/cpu.h
$MAKE -j$NPROC -f "$LIBPATH/make/libwebp.mk" CC="$CC" CFLAGS="$CFLAGS -w" AR="$AR" PREFIX="$PREFIX"
cp -R src/webp $PREFIX/include/
cd ..

echo 'Building zlib...'
$TAR xzf "$LIBPATH/archive/zlib-1.3.1.tar.gz"
cd zlib-1.3.1
$MAKE -j$NPROC -f "$LIBPATH/make/zlib.mk" CC="$CC" CFLAGS="$CFLAGS -w" AR="$AR" PREFIX="$PREFIX"
cp zlib.h zconf.h $PREFIX/include/
cd ..

echo 'Building libpng...'
$TAR xzf "$LIBPATH/archive/libpng-1.6.43.tar.gz"
cd libpng-1.6.43
sed \
    -e 's|/\*#undef PNG_ARM_NEON_API_SUPPORTED\*/|#undef PNG_ARM_NEON_API_SUPPORTED|g' \
    -e 's|/\*#undef PNG_ARM_NEON_CHECK_SUPPORTED\*/|#undef PNG_ARM_NEON_CHECK_SUPPORTED|g' \
    < scripts/pnglibconf.h.prebuilt \
    > pnglibconf.h
$MAKE -j$NPROC -f "$LIBPATH/make/libpng.mk" CC="$CC" CFLAGS="$CFLAGS -w" AR="$AR" PREFIX="$PREFIX"
mkdir $PREFIX/include/png
cp *.h $PREFIX/include/png/
cd ..

echo 'Building jpeg...'
$TAR xzf "$LIBPATH/archive/jpegsrc.v9e.tar.gz"
cd jpeg-9e
cp jconfig.txt jconfig.h
$MAKE -j$NPROC -f "$LIBPATH/make/jpeg.mk" CC="$CC" CFLAGS="$CFLAGS -w" AR="$AR" PREFIX="$PREFIX"
mkdir $PREFIX/include/jpeg
cp *.h $PREFIX/include/jpeg/
cd ..

echo 'Building libogg...'
$TAR xzf "$LIBPATH/archive/libogg-1.3.3.tar.gz"
cd libogg-1.3.3
sed -e 's/@INCLUDE_INTTYPES_H@/1/g' \
    -e 's/@INCLUDE_STDINT_H@/1/g' \
    -e 's/@INCLUDE_SYS_TYPES_H@/1/g' \
    -e 's/@SIZE16@/int16_t/g' \
    -e 's/@USIZE16@/uint16_t/g' \
    -e 's/@SIZE32@/int32_t/g' \
    -e 's/@USIZE32@/uint32_t/g' \
    -e 's/@SIZE64@/int64_t/g' \
    < include/ogg/config_types.h.in \
    > include/ogg/config_types.h
$MAKE -j$NPROC -f "$LIBPATH/make/libogg.mk" CC="$CC" CFLAGS="$CFLAGS -w" AR="$AR" PREFIX="$PREFIX"
cp -R include/ogg $PREFIX/include/
cd ..

echo 'Building libvorbis...'
$TAR xzf "$LIBPATH/archive/libvorbis-1.3.7.tar.gz"
cd libvorbis-1.3.7
$MAKE -j$NPROC -f "$LIBPATH/make/libvorbis.mk" CC="$CC" CFLAGS="$CFLAGS -w" AR="$AR" PREFIX="$PREFIX"
cp -R include/vorbis $PREFIX/include/
cd ..

echo 'Building freetype2...'
$TAR xzf "$LIBPATH/archive/freetype-2.13.3.tar.gz"
cd freetype-2.13.3
sed -e 's/FONT_MODULES += type1//' \
    -e 's/FONT_MODULES += cid//' \
    -e 's/FONT_MODULES += pfr//' \
    -e 's/FONT_MODULES += type42//' \
    -e 's/FONT_MODULES += pcf//' \
    -e 's/FONT_MODULES += bdf//' \
    -e 's/FONT_MODULES += pshinter//' \
    -e 's/FONT_MODULES += raster//' \
    -e 's/FONT_MODULES += psaux//' \
    -e 's/FONT_MODULES += psnames//' \
    < modules.cfg > modules.cfg.new
mv modules.cfg.new modules.cfg
patch src/gzip/zutil.h < "$LIBPATH/patch/freetype.patch"
$MAKE -j$NPROC -f "$LIBPATH/make/freetype2.mk" CC="$CC" CFLAGS="$CFLAGS -w" AR="$AR" PREFIX="$PREFIX"
cp -R include/freetype $PREFIX/include/freetype
cp include/ft2build.h $PREFIX/include/
cd ..

cd ..
rm -rf tmp
echo 'Finished.'
