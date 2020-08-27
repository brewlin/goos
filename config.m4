PHP_ARG_ENABLE(go, whether to enable go support,
Make sure that the comment is aligned:
[  --enable-go           Enable go support])

AC_ARG_ENABLE(debug,
    [  --enable-debug,         compile with debug symbols],
    [GO_DEBUG=1],
    [GO_DEBUG=0]
)

if test "$GO_DEBUG" != "0"; then
    AC_DEFINE(GO_DEBUG, 1, [enable debug])
fi

if test "$PHP_GO" != "no"; then

    PHP_ADD_LIBRARY(pthread)
    CFLAGS="-Wall -pthread $CFLAGS"

    go_source_file="\
        go.cpp                   \
        wrapper/coroutine.cpp    \
        wrapper/runtime.cpp      \

        coroutine/Coroutine.cpp  \
        coroutine/PHPCoroutine.cpp \

        runtime/Context.cpp      \
        runtime/Proc.cpp         \
        runtime/ZendFunction.cpp \
        runtime/Sysmon.cpp       \
        runtime/Log.cpp       \

        runtime/asm/jump_context.s \
        runtime/asm/make_context.cpp

    "

    PHP_NEW_EXTENSION(go, $go_source_file, $ext_shared, ,-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1, cxx)

    PHP_ADD_INCLUDE([$ext_srcdir])
    PHP_ADD_INCLUDE([$ext_srcdir/lib])
    PHP_ADD_INCLUDE([$ext_srcdir/coroutine])
    PHP_ADD_INCLUDE([$ext_srcdir/runtime])
    PHP_ADD_INCLUDE([$ext_srcdir/runtime/asm])

    PHP_INSTALL_HEADERS([ext/go], [*.h config.h coroutine/*.h runtime/*.h])
    PHP_ADD_MAKEFILE_FRAGMENT

    PHP_REQUIRE_CXX()

    CXXFLAGS="$CXXFLAGS -O0 -Wall -Wno-unused-function -Wno-deprecated -Wno-deprecated-declarations"
    CXXFLAGS="$CXXFLAGS -std=c++11"
fi