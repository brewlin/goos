#include "Coroutine.h"
#include "PHPCoroutine.h"
#include "php_go.h"
PHP_METHOD(coroutine,status)
{
    if(GO_ZG(_g) == nullptr){
        cout << " 当前g 不存在，异常，非协程内" <<endl;
    }else{
        cout << "co addr: " << GO_ZG(_g) <<endl;
    }
}
PHP_FUNCTION(go_yield)
{
    if(GO_ZG(_g) == nullptr){
        cout << " 当前g 不存在，异常，非协程内";
    }else{
        GO_ZG(_g)->yield();
    }
}
PHP_FUNCTION(go_create)
{
    zend_fcall_info fci = empty_fcall_info;
    zend_fcall_info_cache fcc = empty_fcall_info_cache;
    //1 -1 可变参数
    ZEND_PARSE_PARAMETERS_START(1,-1)
    Z_PARAM_FUNC(fci,fcc)
    Z_PARAM_VARIADIC("*",fci.params,fci.param_count)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);
    long cid = PHPCoroutine::go(fcc.function_handler,fci.params,fci.param_count);
    RETURN_LONG(cid);
}

const zend_function_entry go_coroutine_methods[] =
{
    ZEND_FENTRY(create,ZEND_FN(go_create),arg_go_create,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(coroutine,status,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_FENTRY(goyield,ZEND_FN(go_yield),NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};

zend_class_entry go_coroutine_ce;
zend_class_entry *go_coroutine_ce_ptr;

void register_php_coroutine()
{
    INIT_NS_CLASS_ENTRY(go_coroutine_ce,"","Coroutine",go_coroutine_methods);
    go_coroutine_ce_ptr = zend_register_internal_class(&go_coroutine_ce TSRMLS_CC);
    //给类增加短名机制
     zend_register_class_alias("Co", go_coroutine_ce_ptr);
}
