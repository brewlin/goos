#ifndef GO_PHPCORUTINE_H
#define GO_PHPCORUTINE_H
#include "php_go.h"
#include <stack>
class Coroutine;


/**
 * php 协程栈
 */
struct php_sp
{
    //协程栈顶
    zval *vm_stack_top;
    //协程栈栈底
    zval *vm_stack_end;
    //协程栈指针
    zend_vm_stack vm_stack;
    //协程栈页大小
    size_t vm_stack_page_size;
    //协程栈的栈帧
    zend_execute_data *execute_data;
    Coroutine *co;
};

class PHPCoroutine {
public:
    static long                     go(zend_function *call,zval *argv,uint32_t argc);
    static void                     save_stack(php_sp *sp);
    static void                     init_stack();
    static void                     run(void *args);

};

ZEND_BEGIN_ARG_INFO_EX(arg_go_create,0,0,1)
ZEND_ARG_CALLABLE_INFO(0,func,0)
ZEND_END_ARG_INFO()
PHP_FUNCTION(go_create);
PHP_FUNCTION(go_exit);
PHP_FUNCTION(go_yield);
void register_php_coroutine();
#endif //GO_PHPCORUTINE_H
