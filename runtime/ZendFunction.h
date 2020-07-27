
#ifndef GO_ZENDFUNCTION_H
#define GO_ZENDFUNCTION_H
#include <mutex>
#include <queue>
#include "php_go.h"
#include "QList.h"
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

public:
    ZendFunction(zend_function *func,zval *argv,uint32_t argc);
    ~ZendFunction();
    static void                     prepare_functions(Coroutine *co);
    static zend_function*           copy_function(zend_function *function);
    static zend_function*           copy_user_function(zend_function *function);
    static zend_op*                 copy_opcodes(zend_op_array *op_array, zval *literals);
    static zend_arg_info*           copy_arginfo(zend_op_array *op_array, zend_arg_info *old, uint32_t end);
    static zend_live_range*         copy_live(zend_live_range *old, int end);
    static zend_try_catch_element*  copy_try(zend_try_catch_element *old, int end);
    static zend_string**            copy_variables(zend_string **old, int end);
    static HashTable*               copy_statics(HashTable *old);
    static zval*                    copy_literals(zval *old, int last);
};


class Freeq
{
public:
    Freeq(){func_list = new QList<ZendFunction*>();}
    ~Freeq(){if(func_list) delete func_list;}
    mutex gofq_lock;
    QList<ZendFunction*> *func_list;
};




#endif //GO_ZENDFUNCTION_H
