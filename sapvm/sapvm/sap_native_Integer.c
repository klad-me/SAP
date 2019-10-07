#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <stdio.h>
#include <string.h>


// Вызвать нативную функцию Integer.*
sap_byte sapvm_call_native_Integer(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0xaa7abccb)
    {
	// native static byte[] Integer.__toString(int v);
	sap_int v=R_INT(params+0);
	sap_byte minus=0;
	sap_uint uv=(sap_uint)v;
	char str[12],*ss=str;
	if (v < 0)
	{
	    minus=1;
	    uv=(sap_uint)(-v);
	}
	do
	{
	    (*ss++)=(char)('0'+(char)(uv % 10));
	    uv/=10;
	} while (uv > 0);
	if (minus) (*ss++)='-';
	sap_array* array=sapvm_new_byte_array((sap_int)(ss-str));
	if (array)
	{
	    sap_byte *ptr=array->data.b;
	    do
	    {
		(*ptr++)=(*--ss);
	    } while (ss!=str);
	}
	
	// return array;
	W_REF(f->stack + f->SP, array);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x676d8bed)
    {
	// native static int Integer.__parseInt(byte[] str);
	sap_array *array=(sap_array*)R_REF(params);
	sap_int v=0, sign=1, i=0;
	
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Парсим строку
	if (array->data.b[0]=='-')
	{
	    i=1;
	    sign=-1;
	}
	for (; i<array->length; i++)
	{
	    if ( (array->data.b[i]<'0') ||
		 (array->data.b[i]>'9') ) break;
	    
	    v*=10;
	    v+=array->data.b[i]-'0';
	}
	
	// Забываем массив
	sapvm_dec_ref(vm, (sap_object*)array);
	
	// return v*sign;
	W_INT(f->stack + f->SP, v*sign);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
