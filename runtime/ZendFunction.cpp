
#include "ZendFunction.h"
#include "Coroutine.h"
#include "ZendString.h"

void ZendFunction::freehash(zval *zval_ptr)
{

//    if(Z_TYPE_P(zval_ptr) == IS_NULL){
//        cout << "该变量为 null" <<endl;
//        return;
//    }
    zval_ptr_dtor(zval_ptr);
}
ZendFunction::ZendFunction(zend_function *func,zval *argv,uint32_t argc):argv(argv),argc(argc),is_new(1)
{
    arena_checkpoint = zend_arena_checkpoint(CG(arena));;
    this->func = copy_function(func);
}
ZendFunction::~ZendFunction()
{
    zend_op_array *op = &func->op_array;
    if(op->static_variables != nullptr){
        zend_hash_destroy(op->static_variables);
//        FREE_HASHTABLE(op->static_variables);
//        free(op->static_variables);
    }
    free(op->literals);
    free(op->refcount);
    free(op->opcodes);
    free(op->live_range);
    ZendString::free_string(op->function_name);

    zend_string   **variables = op->vars;
    int end = op->last_var;
    int it = 0;
    while (it < end) {
        ZendString::free_string(variables[it]);
        it++;
    }
    free(op->vars);
    free(op);
//    zend_arena_release(&CG(arena),arena_checkpoint);
}
void ZendFunction::prepare_functions(Coroutine *co) {
    ZendFunction *call = co->callback;
    zend_string *key, *name;
    void *value = NULL,*prepared = NULL;
    ZEND_HASH_FOREACH_STR_KEY_PTR(GO_CG(co->creator, function_table), key, value)
    {
        if (((zend_function*)value)->type == ZEND_INTERNAL_FUNCTION ||
            zend_hash_exists(GO_CG(GO_ZG(local), function_table), key))
            continue;
        name = zend_string_new(key);
        prepared = copy_function((zend_function*)value);
        if (!zend_hash_add_ptr(CG(function_table), name, prepared)) {
            destroy_op_array((zend_op_array*)prepared);
        }

        zend_string_release(name);
    }
    ZEND_HASH_FOREACH_END();
}

zend_function* ZendFunction::copy_function(zend_function *function) {
    zend_function *copy;
    if (!(function->op_array.fn_flags & ZEND_ACC_CLOSURE)) {
        copy = (zend_function*)zend_hash_index_find_ptr(&GO_ZG(resolve), (zend_ulong)function);
        if (copy) {
            function_add_ref(copy);
            return copy;
        }
    }
    if (function->type == ZEND_USER_FUNCTION) {
        copy = copy_user_function(function);
    } else {
//        copy = copy_internal_function(function);
    }
    if (function->op_array.fn_flags & ZEND_ACC_CLOSURE) { //don't cache closures
        return copy;
    }
    return (zend_function*)zend_hash_index_update_ptr(&GO_ZG(resolve), (zend_ulong) function, copy);
}
zend_function* ZendFunction::copy_user_function(zend_function *function)
{
    zend_function  *copy;
    zend_op_array  *op_array;
    zend_string   **variables, *filename_copy;
    zval           *literals;
    zend_arg_info  *arg_info;
    if(is_new) copy = (zend_function*)malloc(sizeof(zend_op_array));
    else       copy = (zend_function*) zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
    memcpy(copy, function, sizeof(zend_op_array));

    op_array = &copy->op_array;
    variables = op_array->vars;
    literals = op_array->literals;
    arg_info = op_array->arg_info;

//    op_array->function_name = zend_string_new(op_array->function_name);
    op_array->function_name = ZendString::copy_string(op_array->function_name,is_new);
    /* we don't care about prototypes */
    op_array->prototype = NULL;

    if(is_new) op_array->refcount = (uint32_t*)malloc(sizeof(uint32_t));
    else op_array->refcount = (uint32_t*)emalloc(sizeof(uint32_t));

    (*op_array->refcount) = 1;
    /* we never want to share the same runtime cache */
    op_array->run_time_cache = NULL;
#if PHP_VERSION_ID >= 70300
    op_array->fn_flags &= ~ZEND_ACC_DONE_PASS_TWO;
#endif
    if (op_array->doc_comment) {
        op_array->doc_comment = zend_string_new(op_array->doc_comment);
    }
    if (!(filename_copy = (zend_string *)zend_hash_find_ptr(&GO_ZG(filenames), op_array->filename))) {
        filename_copy = zend_string_new(op_array->filename);
        zend_hash_add_ptr(&GO_ZG(filenames), filename_copy, filename_copy);
        zend_string_release(filename_copy);
    }

    op_array->filename = filename_copy;

    if (op_array->literals) op_array->literals = copy_literals (literals, op_array->last_literal);

    op_array->opcodes = copy_opcodes(op_array, literals);

    if (op_array->arg_info) 	op_array->arg_info = copy_arginfo(op_array, arg_info, op_array->num_args);
    if (op_array->live_range)		op_array->live_range = copy_live(op_array->live_range, op_array->last_live_range);
    if (op_array->try_catch_array)  op_array->try_catch_array = copy_try(op_array->try_catch_array, op_array->last_try_catch);
    if (op_array->vars) 		op_array->vars = copy_variables(variables, op_array->last_var);
    if (op_array->static_variables) op_array->static_variables = copy_statics(op_array->static_variables);

    return copy;
}
zval* ZendFunction::copy_literals(zval *old, int last) {
    zval *literals;
    if(is_new) literals = (zval*) calloc(last, sizeof(zval));
    else literals = (zval*) safe_emalloc(last, sizeof(zval), 0);

    zval *literal = literals,
        *end = literals + last;

    memcpy(literals, old, sizeof(zval) * last);

    while (literal < end) {
        switch (Z_TYPE_P(literal)) {
            case IS_ARRAY:
//                store_separate(literal, literal, 1);
                break;
#if PHP_VERSION_ID < 70300
            case IS_CONSTANT:
#endif
            case IS_CONSTANT_AST:
                zval_copy_ctor(literal);
                break;

        }
        literal++;
    }

    return literals;
}
HashTable* ZendFunction::copy_statics(HashTable *old) {
    HashTable *statics = NULL;

    if (old) {
        zend_string *key;
        zval *value;
        if(is_new) statics = (HashTable *) malloc(sizeof(HashTable));
        else ALLOC_HASHTABLE(statics);
        zend_hash_init(statics,
                       zend_hash_num_elements(old),
                       NULL, freehash, is_new);

        ZEND_HASH_FOREACH_STR_KEY_VAL(old, key, value) {
            zend_string *name = zend_string_new(key);
            zval *next = value;
            while (Z_TYPE_P(next) == IS_REFERENCE)
                next = &Z_REF_P(next)->val;

            if (Z_REFCOUNTED_P(next)) {
                zval copy;

                switch (Z_TYPE_P(next)) {
                    case IS_STRING:
                        ZVAL_STR(&copy,
                                 zend_string_new(Z_STR_P(next)));
                        zend_hash_add(statics, name, &copy);
                        break;

                    case IS_OBJECT:
                        if (instanceof_function(Z_OBJCE_P(next), go_coroutine_ce_ptr) ||
                            instanceof_function(Z_OBJCE_P(next), zend_ce_closure)) {
                            zend_hash_add(statics, name, &copy);
                        } else zend_hash_add_empty_element(statics, name);
                        break;

                    case IS_CONSTANT_AST:
#if PHP_VERSION_ID < 70300
                        ZVAL_NEW_AST(&copy, zend_ast_copy(Z_AST_P(next)->ast));
#else
                        ZVAL_AST(&copy, zend_ast_copy(GC_AST(Z_AST_P(next))));
#endif
                        zend_hash_add(statics, name, &copy);
                        break;

                    default:
                        zend_hash_add_empty_element(statics, name);
                }
            } else zend_hash_add(statics, name, next);
            zend_string_release(name);
        } ZEND_HASH_FOREACH_END();
    }

    return statics;
}
zend_string** ZendFunction::copy_variables(zend_string **old, int end) {
    zend_string **variables;
    if(is_new) variables = (zend_string** )calloc(end, sizeof(zend_string*));
    else variables = (zend_string** )safe_emalloc(end, sizeof(zend_string*), 0);
    int it = 0;

    while (it < end) {
        if(is_new)        variables[it]  = ZendString::copy_string(old[it],is_new);
        else   variables[it] = zend_string_new(old[it]);

        it++;
    }

    return variables;
}
zend_try_catch_element* ZendFunction::copy_try(zend_try_catch_element *old, int end) {
    zend_try_catch_element *try_catch;
    if(is_new) try_catch = (zend_try_catch_element *)calloc(end, sizeof(zend_try_catch_element));
    else try_catch = (zend_try_catch_element *)safe_emalloc(end, sizeof(zend_try_catch_element), 0);
    memcpy(
        try_catch,
        old,
        sizeof(zend_try_catch_element) * end);

    return try_catch;
}
zend_live_range* ZendFunction::copy_live(zend_live_range *old, int end) {
    zend_live_range *range;
    if(is_new) range = (zend_live_range *)calloc(end, sizeof(zend_live_range));
    else range = (zend_live_range *)safe_emalloc(end, sizeof(zend_live_range), 0);
    memcpy(
        range,
        old,
        sizeof(zend_live_range) * end);

    return range;
}
zend_arg_info* ZendFunction::copy_arginfo(zend_op_array *op_array, zend_arg_info *old, uint32_t end)
{
    zend_arg_info *info;
    uint32_t it = 0;

    if (op_array->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
        old--;
        end++;
    }

    if (op_array->fn_flags & ZEND_ACC_VARIADIC) {
        end++;
    }

    if(is_new) info = (zend_arg_info *)calloc(end, sizeof(zend_arg_info));
    else info = (zend_arg_info *)safe_emalloc(end, sizeof(zend_arg_info), 0);
    memcpy(info, old, sizeof(zend_arg_info) * end);

    while (it < end) {
        if (info[it].name)
            info[it].name = zend_string_new(old[it].name);

        if (ZEND_TYPE_IS_SET(old[it].type) && ZEND_TYPE_IS_CLASS(old[it].type)) {
            info[it].type = ZEND_TYPE_ENCODE_CLASS(
                zend_string_new(
                    ZEND_TYPE_NAME(info[it].type)),
                ZEND_TYPE_ALLOW_NULL(info[it].type));
        }
        it++;
    }

    if (op_array->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
        info++;
    }

    return info;
}

zend_op* ZendFunction::copy_opcodes(zend_op_array *op_array, zval *literals)
{
    zend_op *copy;
    if(is_new) copy = (zend_op*)calloc(op_array->last, sizeof(zend_op));
    else copy = (zend_op*)safe_emalloc(op_array->last, sizeof(zend_op), 0);

    memcpy(copy, op_array->opcodes, sizeof(zend_op) * op_array->last);

    /* The following code comes from ext/opcache/zend_persft.c */
    zend_op *opline = copy;
    zend_op *end    = copy + op_array->last;

    for (; opline < end; opline++) {
#if ZEND_USE_ABS_CONST_ADDR
        if (opline->op1_type == IS_CONST) {
				opline->op1.zv = (zval*)((char*)opline->op1.zv + ((char*)op_array->literals - (char*)literals));
			}
			if (opline->op2_type == IS_CONST) {
				opline->op2.zv = (zval*)((char*)opline->op2.zv + ((char*)op_array->literals - (char*)literals));
			}
#else
        if (opline->op1_type == IS_CONST) {
            opline->op1.constant =
                (char*)(op_array->literals +
                        ((zval*)((char*)(op_array->opcodes + (opline - copy)) +
                                 (int32_t)opline->op1.constant) - literals)) -
                (char*)opline;
        }
        if (opline->op2_type == IS_CONST) {
            opline->op2.constant =
                (char*)(op_array->literals +
                        ((zval*)((char*)(op_array->opcodes + (opline - copy)) +
                                 (int32_t)opline->op2.constant) - literals)) -
                (char*)opline;
        }
#endif
#if ZEND_USE_ABS_JMP_ADDR
        if (op_array->fn_flags & ZEND_ACC_DONE_PASS_TWO) {
			/* fix jumps to point to new array */
			switch (opline->opcode) {
				case ZEND_JMP:
				case ZEND_FAST_CALL:
					opline->op1.jmp_addr = &copy[opline->op1.jmp_addr - op_array->opcodes];
					break;
				case ZEND_JMPZNZ:
					/* relative extended_value don't have to be changed */
					/* break omitted intentionally */
				case ZEND_JMPZ:
				case ZEND_JMPNZ:
				case ZEND_JMPZ_EX:
				case ZEND_JMPNZ_EX:
				case ZEND_JMP_SET:
				case ZEND_COALESCE:
				case ZEND_FE_RESET_R:
				case ZEND_FE_RESET_RW:
				case ZEND_ASSERT_CHECK:
					opline->op2.jmp_addr = &copy[opline->op2.jmp_addr - op_array->opcodes];
					break;
				case ZEND_DECLARE_ANON_CLASS:
				case ZEND_DECLARE_ANON_INHERITED_CLASS:
				case ZEND_FE_FETCH_R:
				case ZEND_FE_FETCH_RW:
				case ZEND_SWITCH_LONG:
				case ZEND_SWITCH_STRING:
					/* relative extended_value don't have to be changed */
					break;
			}
		}
#endif
    }

    return copy;
}