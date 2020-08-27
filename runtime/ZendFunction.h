
#ifndef GO_ZENDFUNCTION_H
#define GO_ZENDFUNCTION_H
#include <mutex>
#include <queue>
#include "php_go.h"
using namespace std;

class Coroutine;
/**
 * 协程执行函数
 */
class ZendFunction
{
public:
    void                            *arena_checkpoint;
    zend_function                   *func;
    zval                            *argv;
    uint32_t                        argc;
    int                             is_new = 0;
public:
    ZendFunction(zend_function *func,zval *argv,uint32_t argc);
    ~ZendFunction();
    void                     prepare_functions(Coroutine *co);
    zend_function*           copy_function(zend_function *function);
    zend_function*           copy_user_function(zend_function *function);
    zend_op*                 copy_opcodes(zend_op_array *op_array, zval *literals);
    zend_arg_info*           copy_arginfo(zend_op_array *op_array, zend_arg_info *old, uint32_t end);
    zend_live_range*         copy_live(zend_live_range *old, int end);
    zend_try_catch_element*  copy_try(zend_try_catch_element *old, int end);
    zend_string**            copy_variables(zend_string **old, int end);
    HashTable*               copy_statics(HashTable *old);
    zval*                    copy_literals(zval *old, int last);
    static void              freehash(zval* zval_ptr);
};


#endif //GO_ZENDFUNCTION_H
