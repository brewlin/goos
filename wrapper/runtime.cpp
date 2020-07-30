#include "Coroutine.h"
#include "PHPCoroutine.h"
#include "php_go.h"
#include "Proc.h"
#include "Sysmon.h"

ZEND_BEGIN_ARG_INFO_EX(arg_num, 0, 0, 1)
ZEND_ARG_INFO(0, num)
ZEND_END_ARG_INFO()

PHP_METHOD(runtime,GOMAXPROCS)
{
    zend_long num;
    ZEND_PARSE_PARAMETERS_START(1, 1)
    Z_PARAM_LONG(num)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);
    Sysmon::newm(num);
}
PHP_METHOD(runtime,wait)
{
    this_thread::sleep_for(chrono::seconds(1));
    Sysmon::wait();
    Proc::free_func();
}
const zend_function_entry go_runtime_methods[] =
{
    PHP_ME(runtime,GOMAXPROCS,arg_num,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(runtime,wait,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};

zend_class_entry go_runtime_ce;
zend_class_entry *go_runtime_ce_ptr;

void register_php_runtime()
{
    INIT_NS_CLASS_ENTRY(go_runtime_ce,"","Runtime",go_runtime_methods);
    go_runtime_ce_ptr = zend_register_internal_class(&go_runtime_ce TSRMLS_CC);
    //给类增加短名机制
     zend_register_class_alias("Runtime", go_runtime_ce_ptr);
}
