LIB= foohash
SHLIB_MAJOR= 1
SHLIB_MINOR= 0
SRCS= hash.c
INCS= include/foo/hash.h
CFLAGS+= -O2 -I${.CURDIR}/include -I/usr/local/include
CFLAGS+= -DNDEBUG
DESTDIR= /usr/local
LIBDIR= /lib
INCLUDEDIR= /include/foo
DIRS+= INCLUDEDIR
.if 0
CSTD= gnu17
CC= gcc12
LDFLAGS+= -Wl,-rpath=/usr/local/lib/gcc12
.endif

MK_PROFILE= no
MK_SSP= no

.include <bsd.lib.mk>
