#ifndef GOOS_ZENDSTRING_H
#define GOOS_ZENDSTRING_H

#include "php_go.h"

class ZendString
{
public:
    static char* strndup(const char *s, size_t n)
    {
        char *p = (char *) malloc(n + 1);
        if (likely(p))
        {
            strncpy(p, s, n)[n] = '\0';
        }
        return p;
    }
    static zend_string* copy_string(zend_string *str,int persistent)
    {
        if (ZSTR_IS_INTERNED(str)) {
            if (!GO_ZG(hard_copy_interned_strings)) {
                return str;
            }
//            return zend_new_interned_string(zend_string_init(ZSTR_VAL(str), ZSTR_LEN(str), GC_FLAGS(str) & IS_STR_PERSISTENT));
//            return zend_string_dup(str, GC_FLAGS(str) & IS_STR_PERSISTENT);
            return zend_string_init(ZSTR_VAL(str),ZSTR_LEN(str),persistent);
        }
        if (persistent)
            return zend_string_init(ZSTR_VAL(str),ZSTR_LEN(str),persistent);
        else
            return zend_string_dup(str, GC_FLAGS(str) & IS_STR_PERSISTENT);
    }
    static void free_string(zend_string *str)
    {
        if(str == NULL)return;

        if (ZSTR_IS_INTERNED(str))
            return;
        zend_string_release(str);
    }
};

#endif //GOOS_ZENDSTRING_H
