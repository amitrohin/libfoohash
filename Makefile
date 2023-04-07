LIB= foohash
SHLIB_MAJOR= 1
SHLIB_MINOR= 0
SRCS= hash.c
INCS= include/foo/hash.h
CFLAGS+= -Wall -Wextra -I${.CURDIR}/include -I/usr/local/include
CFLAGS+= -DNDEBUG
DESTDIR= /usr/local
LIBDIR= /lib
INCLUDEDIR= /include/foo
DIRS+= INCLUDEDIR

CSTD= gnu17
.if 0
CC= gcc12
LDFLAGS+= -Wl,-rpath=/usr/local/lib/gcc12
.endif

MK_PROFILE= no
MK_SSP= no

.include <bsd.lib.mk>
