/*
 * The MIT License
 *
 * Copyright (c) 2010 - 2011 Shuhei Tanuma
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "php_git.h"
#include <spl/spl_array.h>
#include <zend_interfaces.h>
#include <string.h>
#include <time.h>

static void php_git_tree_free_storage(php_git_tree_t *obj TSRMLS_DC)
{
    zend_object_std_dtor(&obj->zo TSRMLS_CC);
    
    //RepositoryでFreeされるよ
    obj->tree = NULL;
    obj->repository = NULL;
    efree(obj);
}

zend_object_value php_git_tree_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	php_git_tree_t *obj;
	zval *tmp;

	obj = ecalloc(1, sizeof(*obj));
	zend_object_std_init( &obj->zo, ce TSRMLS_CC );
	zend_hash_copy(obj->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(obj, 
        (zend_objects_store_dtor_t)zend_objects_destroy_object,
        (zend_objects_free_object_storage_t)php_git_tree_free_storage,
        NULL TSRMLS_CC);
	retval.handlers = zend_get_std_object_handlers();
	return retval;
}



PHP_METHOD(git_tree, count)
{
	long cnt = 0;
    zval *entries;

    entries = zend_read_property(git_tree_entry_class_entry,getThis(),"entries",7,0 TSRMLS_DC);
	cnt = zend_hash_num_elements(Z_ARRVAL_P(entries));

    RETURN_LONG(cnt);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_git_tree_path, 0, 0, 1)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

PHP_METHOD(git_tree, path)
{
    //TODO
    zval *object = getThis();
    char *path;
    int path_len= 0;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
        "s", &path, &path_len) == FAILURE){
        return;
    }
}


PHP_METHOD(git_tree, getId)
{
    zval *object = getThis();
    git_oid *oid;
    char out[40];
    php_git_tree_t *git_tree = (php_git_tree_t *) zend_object_store_get_object(object TSRMLS_CC);
    
    oid = git_tree_id(git_tree->tree);
    git_oid_to_string(out,41,oid);
    RETVAL_STRING(out,1);
}


ZEND_BEGIN_ARG_INFO_EX(arginfo_git_tree_add, 0, 0, 1)
    ZEND_ARG_INFO(0, entry)
ZEND_END_ARG_INFO()
PHP_METHOD(git_tree, add)
{
    zval *object = getThis();
    zval *entry;
    char *filename;
    git_oid oid;
    git_tree *tree;
    int attr;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
        "z", &entry) == FAILURE){
        return;
    }
    
    php_git_tree_t *myobj = (php_git_tree_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
    tree = myobj->tree;
    //php_printf("tree-oid: %s\n",Z_STRVAL_P(zend_read_property(git_tree_entry_class_entry,entry,"oid",3,1 TSRMLS_CC)));
    git_oid_mkstr(&oid, Z_STRVAL_P(zend_read_property(git_tree_entry_class_entry,entry,"oid",3,1 TSRMLS_CC)));
    filename = Z_STRVAL_P(zend_read_property(git_tree_entry_class_entry,entry,"name",4,1 TSRMLS_CC));
    attr = Z_LVAL_P(zend_read_property(git_tree_entry_class_entry,entry,"attr",4,1 TSRMLS_CC));
    //git_tree_add_entry (git_tree *tree, const git_oid *id, const char *filename, int attributes)

    char uhi[40];
    git_oid *mo;
    git_oid_to_string(uhi,41,&oid);
    //printf("oid: %s\n",uhi);
    //printf("path: %s\n",filename);
    mo = git_tree_id(tree);
    git_oid_to_string(&uhi,41,mo);
    //printf("tree-oid: %s\n",uhi);
    
    git_tree_remove_entry_byname(tree, filename);
    git_tree_add_entry(tree, &oid, filename, attr);

    int ret = git_object_write((git_object *)tree);
    if(ret != GIT_SUCCESS){
        php_printf("can't write object");
    }
    
    char out[40];
    git_oid *om;
    om = git_object_id((git_object *)tree);
    git_oid_to_string(&out,41,om);
    
    RETVAL_STRING(&out, 1);
}

PHP_METHOD(git_tree, write)
{
    zval *this = getThis();
    php_git_tree_t *tree_t;
    git_oid *oid;
    char out[40];
    int ret = 0;
    tree_t = (php_git_tree_t *)zend_object_store_get_object(this TSRMLS_CC);
    
    ret = git_object_write((git_object *)tree_t->tree);
    if(ret == GIT_SUCCESS){
        oid = git_object_id((git_object *)tree_t->tree);
        git_oid_to_string(&out,41,oid);
        
        RETVAL_STRINGL(out,40,1 TSRMLS_DC);
    }else{
        RETURN_FALSE;
    }
}

// GitTree
PHPAPI function_entry php_git_tree_methods[] = {
    PHP_ME(git_tree, count, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, path, arginfo_git_tree_path, ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, add,  arginfo_git_tree_add, ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, getId,NULL, ZEND_ACC_PUBLIC)
    PHP_ME(git_tree, write, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

void git_init_tree(TSRMLS_D)
{
    zend_class_entry git_tree_ce;
    INIT_CLASS_ENTRY(git_tree_ce, "GitTree", php_git_tree_methods);
    git_tree_class_entry = zend_register_internal_class(&git_tree_ce TSRMLS_CC);
	git_tree_class_entry->create_object = php_git_tree_new;
    zend_class_implements(git_tree_class_entry TSRMLS_CC, 1, spl_ce_Countable);
}