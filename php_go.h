/* go extension for PHP */

#ifndef PHP_GO_H
# define PHP_GO_H
#include "php.h"
#include "config.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_network.h"
#include "php_streams.h"

#include "php_globals.h"
#include "php_main.h"


#include "TSRM.h"
#include "zend_variables.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "zend_exceptions.h"
#include <mutex>
#include <condition_variable>
#include <queue>
#include <iostream>
using namespace std;

using timep = chrono::steady_clock::time_point;
extern zend_module_entry go_module_entry;
extern zend_class_entry *go_coroutine_ce_ptr;

class Freeq;
class Stackq;
class Coroutine;
class Runq;
ZEND_EXTERN_MODULE_GLOBALS(go)
#ifndef GO_ZG
ZEND_BEGIN_MODULE_GLOBALS(go)
    Stackq*     free_stack;
    Freeq*      q;
    Runq*       rq;
    pid_t       pid;
    int         signal;
    void***     local;
    //执行抢占标记检测
    timep       schedwhen;
    int         schedtick;
    zval        _this;
    Coroutine*  _g;
    mutex*      _glock;
    HashTable   resolve;
    HashTable   filenames;
    HashTable*  resources;
    int         hard_copy_interned_strings;
ZEND_END_MODULE_GLOBALS(go)
#   define FETCH_CTX(ls, id, type, element) (((type) (*((void ***) ls))[TSRM_UNSHUFFLE_RSRC_ID(id)])->element)
#   define FETCH_CTX_ALL(ls, id, type) ((type) (*((void ***) ls))[TSRM_UNSHUFFLE_RSRC_ID(id)])
#   define GO_CG(ls, v) FETCH_CTX(ls, compiler_globals_id, zend_compiler_globals*, v)
#   define GO_FETCH(ls, v) FETCH_CTX(ls, go_globals_id, zend_go_globals*, v)
#	define GO_ZG(v) TSRMG(go_globals_id, zend_go_globals *, v)
#   define GO_PID() GO_ZG(pid) ? GO_ZG(pid) : (GO_ZG(pid)=getpid())
#   define GO_SG(ls, v) FETCH_CTX(ls, sapi_globals_id, sapi_globals_struct*, v)
#endif

# define PHP_GO_VERSION "0.1.0"

# if defined(ZTS) && defined(COMPILE_DL_GO)
ZEND_TSRMLS_CACHE_EXTERN()
# endif
#define likely(x)        __builtin_expect(!!(x), 1)
#define unlikely(x)      __builtin_expect(!!(x), 0)

static zend_string *zend_string_new(zend_string *s)
{
    if (ZSTR_IS_INTERNED(s)) {
        if (!GO_ZG(hard_copy_interned_strings)) {
            return s;
        }
        return zend_new_interned_string(zend_string_init(ZSTR_VAL(s), ZSTR_LEN(s), GC_FLAGS(s) & IS_STR_PERSISTENT));
    }
    return zend_string_dup(s, GC_FLAGS(s) & IS_STR_PERSISTENT);
}

#endif	/* PHP_GO_H */
