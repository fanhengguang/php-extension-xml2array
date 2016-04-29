/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2008 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: fanhengguang@126.com                                         |
   +----------------------------------------------------------------------+
   */

/* $Id: header 252479 2008-02-07 19:39:50Z iliaa $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_xml2array.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
static zval * php_xml2array_loop(xmlNode *node);
static void php_xml2array_parse(zval* return_value, char * xml_str, long xml_len);
static void php_xml2array_add_val (zval *ret,const xmlChar *name, zval *r, char *son_key);
static void php_xml2array_get_properties (xmlNodePtr cur_node, zval * nodes, char *name);
static zval * init_zval_array();


/* True global resources - no need for thread safety here */
static int le_xml2array;

/* {{{ xml2array_functions[]
 *
 * Every user visible function must have an entry in xml2array_functions[].
 */
	const zend_function_entry xml2array_functions[] = {
		PHP_FE(xml2array,	NULL)
		{NULL, NULL, NULL}	/* Must be the last line in xml2array_functions[] */
	};
/* }}} */

/* {{{ xml2array_module_entry
*/
zend_module_entry xml2array_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"xml2array",
	xml2array_functions,
	PHP_MINIT(xml2array),
	PHP_MSHUTDOWN(xml2array),
	PHP_RINIT(xml2array),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(xml2array),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(xml2array),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_XML2ARRAY
ZEND_GET_MODULE(xml2array)
#endif


PHP_MINIT_FUNCTION(xml2array)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(xml2array)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(xml2array)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(xml2array)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(xml2array)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "xml2array support", "enabled");
	php_info_print_table_end();

}

PHP_FUNCTION(xml2array)
{
	char *arg = NULL;
	long arg_len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	php_xml2array_parse(return_value, arg, arg_len);
}



static void php_xml2array_parse(zval* return_value, char * xml_str, long xml_len) {

	xmlKeepBlanksDefault(0);
	xmlDoc *doc = xmlParseMemory(xml_str, xml_len);
	xmlNode *root_element;

	if (doc == NULL) {
		return;
	} else {
		root_element = xmlDocGetRootElement(doc);
		zval *z;
		z = (zval *) php_xml2array_loop(root_element);

		*return_value = *z;

		xmlFreeDoc(doc);;
		efree(z);
	}

	xmlCleanupParser();
}


static zval* php_xml2array_loop(xmlNodePtr a_node) {

	xmlNodePtr cur_node;
	zval * ret  = init_zval_array();
	zval *r;

	if (a_node->children == NULL) {
		MAKE_STD_ZVAL(r);
		ZVAL_STRING(r, "", 1);
		php_xml2array_add_val(ret, a_node->name, r, NULL);
	} else {
		for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
			char *cur_name = NULL;
			if (cur_node->type == XML_ELEMENT_NODE) {
				cur_name = (char*)cur_node->name;
				r =  php_xml2array_loop(cur_node);
				php_xml2array_get_properties (cur_node, r, cur_name);
			} else if (cur_node->type == XML_CDATA_SECTION_NODE || cur_node->type == XML_TEXT_NODE) {
				MAKE_STD_ZVAL(r);
				xmlChar *z = xmlNodeGetContent(cur_node);
				ZVAL_STRING(r, z, 1);
				xmlFree(z);
			} else {
				continue;
			}

			php_xml2array_add_val(ret, a_node->name, r, cur_name);
		}
	}
	return ret;
}

static zval * init_zval_array() {
	zval * ret = NULL;
	MAKE_STD_ZVAL(ret);
	array_init(ret);
	return ret;
}


static void php_xml2array_get_properties (xmlNodePtr cur_node, zval * nodes, char *name) {
	if (cur_node->properties) {
		xmlAttrPtr attr =  NULL;

		zval **tmp;
		if (zend_symtable_find(Z_ARRVAL_P(nodes),  name, strlen(name) + 1, (void**)&tmp) == FAILURE) {
			return;//this should not happen
		}
		zval *target = *tmp;
		if (Z_TYPE_PP(tmp) != IS_ARRAY) {
			zval *value_zval = init_zval_array();
			target = value_zval;
			zval *copy;
			MAKE_STD_ZVAL(copy);
			MAKE_COPY_ZVAL(tmp, copy);
			add_assoc_zval(value_zval, "value", copy);
			zend_symtable_update(Z_ARRVAL_P(nodes), name, strlen(name)+1, (void *) &value_zval, sizeof(zval *), NULL);
		}

		for(attr = cur_node->properties; NULL != attr; attr = attr->next) {
			xmlChar * prop = NULL;
			prop = xmlGetProp(cur_node, attr->name);
			char *attr_name = (char*)attr->name;
			zval *attr_zval;
			MAKE_STD_ZVAL(attr_zval);
			ZVAL_STRING(attr_zval, prop, 1);
			zend_symtable_update(Z_ARRVAL_P(target), attr_name, strlen(attr_name)+1, (void *) &attr_zval, sizeof(zval *), NULL);
			xmlFree(prop);
		}
	}
}



/**
 * @ret 父亲zval
 * @name 父亲name
 * @r 子zval
 * @son_name 子name
 */
static void php_xml2array_add_val (zval *ret, const xmlChar *name, zval *r, char *son_key) {
	zval **tmp = NULL;
	char *key = (char *)name;//要插入的node 的key

	int has_tmp = zend_symtable_find(Z_ARRVAL(*ret),  key, strlen(key) + 1, (void**)&tmp);

	if (has_tmp == SUCCESS && tmp != NULL && Z_TYPE_PP(tmp) == IS_ARRAY && son_key == NULL && Z_TYPE_P(r) == IS_STRING) {//avoid <xx></xx>,<xx></xx>
		zval_ptr_dtor(&r);
		return;
	}

	if(son_key != NULL && zend_symtable_find(Z_ARRVAL(*ret),  key, strlen(key) + 1, (void**)&tmp) != FAILURE
		&& Z_TYPE_PP(tmp) == IS_ARRAY) {

		zval **son_val = NULL;
		zend_symtable_find(Z_ARRVAL_P(r),  son_key , strlen(son_key)+1, (void**)&son_val);

		zval *son_val_copy;
		MAKE_STD_ZVAL(son_val_copy);
		MAKE_COPY_ZVAL(son_val, son_val_copy);

		zval **tmp_val = NULL;
		if (zend_symtable_find(Z_ARRVAL_P(*tmp),  son_key , strlen(son_key)+1, (void**)&tmp_val) != FAILURE) {//已经包含同名子元素
			if (Z_TYPE_PP(tmp_val)  == IS_ARRAY && zend_hash_index_exists(Z_ARRVAL_PP(tmp_val), 0)) {
				add_next_index_zval(*tmp_val, son_val_copy);
			} else {
				zval *son_arr = init_zval_array();
				zval *copy;
				MAKE_STD_ZVAL(copy);
				MAKE_COPY_ZVAL(tmp_val, copy);
				add_next_index_zval(son_arr, copy);
				add_next_index_zval(son_arr, son_val_copy);
				zend_symtable_update(Z_ARRVAL_PP(tmp), son_key, strlen(son_key)+1, (void *) &son_arr, sizeof(zval *), NULL);
			}
		} else {
			add_assoc_zval(*tmp, son_key, son_val_copy);
		}
		zval_ptr_dtor(&r);//accept a zval** param
	} else {
		add_assoc_zval(ret, key, r);
	}


}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
