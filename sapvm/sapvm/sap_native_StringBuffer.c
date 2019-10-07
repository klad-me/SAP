#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <stdio.h>
#include <string.h>


#define INIT	16
#define ADD	16


// Расположение полей в классе
#define F_DATA	0
#define F_LEN	4


static sap_array* ensureCapacity(sap_vm *vm, sap_array *array, sap_int l)
{
    if (array->length >= l) return array;
    
    // Надо создать новый массив
    l+=ADD;
    sap_array *array2=sapvm_new_byte_array(l);
    if (!array2) return 0;
    
    // Копируем данные
    memcpy(array2->data.b, array->data.b, array->length);
    
    sapvm_dec_ref(vm, (sap_object*)array);
    
    return array2;
}


// Вызвать нативную функцию StringBuffer.*
sap_byte sapvm_call_native_StringBuffer(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0xcbcc31e9)
    {
	// native constructor();
	
	// Запоминаем данные
	W_REF(obj->fields + F_DATA, sapvm_new_byte_array(INIT) );
	W_INT(obj->fields + F_LEN, 0);
	
        // return 0;
        W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0xa52c3e01)
    {
	// native void appendChar(byte c);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int len=R_INT(obj->fields + F_LEN);
	sap_byte c=R_BYTE(params+0);
	
	array=ensureCapacity(vm, array, len+1);
	if (! array) return SAP_FAULT_NO_MEMORY;
	
	array->data.b[len++]=c;
	
	W_REF(obj->fields + F_DATA, array);
	W_INT(obj->fields + F_LEN, len);
	
        // return 0;
        W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x3f8a2270)
    {
	// native void appendString(String s);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int len=R_INT(obj->fields + F_LEN);
	sap_object *o=R_REF(params+0);
	
	if (!o) return SAP_FAULT_NULL_REF;
	sap_array *str=(sap_array*)R_REF(o->fields + 0);
	if (!str) return SAP_FAULT_NULL_REF;
	
	array=ensureCapacity(vm, array, len + str->length);
	if (! array) return SAP_FAULT_NO_MEMORY;
	
	sap_int i;
	for (i=0; i<str->length; i++)
	    array->data.b[len++]=str->data.b[i];
	
	W_REF(obj->fields + F_DATA, array);
	W_INT(obj->fields + F_LEN, len);
	
	// Забываем параметры
	sapvm_dec_ref(vm, o);
	
        // return 0;
        W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x9bf0b7bc)
    {
	// native void insertChar(byte c, int at);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int len=R_INT(obj->fields + F_LEN);
	sap_byte c=R_BYTE(params+0);
	sap_int at=R_INT(params+1);
	
	array=ensureCapacity(vm, array, len+1);
	if (! array) return SAP_FAULT_NO_MEMORY;
	
	if (at<0) at=0; else
	if (at>len) at=len;
	
	// Сдвигаем строку
	memmove(array->data.b+at+1, array->data.b+at, len-at);
	
	// Вставляем символ
	array->data.b[at]=c;
	len++;
	
	W_REF(obj->fields + F_DATA, array);
	W_INT(obj->fields + F_LEN, len);
	
        // return 0;
        W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0xc786c561)
    {
	// native void insertString(String s, int at);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int len=R_INT(obj->fields + F_LEN);
	sap_object *o=R_REF(params+0);
	sap_int at=R_INT(params+4);
	
	if (!o) return SAP_FAULT_NULL_REF;
	sap_array *str=(sap_array*)R_REF(o->fields + 0);
	if (!str) return SAP_FAULT_NULL_REF;
	
	array=ensureCapacity(vm, array, len + str->length);
	if (! array) return SAP_FAULT_NO_MEMORY;
	
	if (at<0) at=0; else
	if (at>len) at=len;
	
	// Сдвигаем строку
	memmove(array->data.b+at+str->length, array->data.b+at, len-at);
	
	sap_int i;
	for (i=0; i<str->length; i++)
	    array->data.b[at+i]=str->data.b[i];
	len+=str->length;
	
	W_REF(obj->fields + F_DATA, array);
	W_INT(obj->fields + F_LEN, len);
	
	// Забываем параметры
	sapvm_dec_ref(vm, o);
	
        // return 0;
        W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0xf248a14a)
    {
	// native String toString();
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int len=R_INT(obj->fields + F_LEN);
	
	// Создаем массив для строки
	sap_array *data=sapvm_new_byte_array(len);
	if (!data) return SAP_FAULT_NO_MEMORY;
	memcpy(data->data.b, array->data.b, len);
	
	// Создаем строку
	sap_object *str=sapvm_new_String(vm, data);
	if (!str) return SAP_FAULT_NO_MEMORY;
	
        // return str;
        W_REF(f->stack + f->SP, str);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0xb755592b)
    {
	// native int length();
	sap_int len=R_INT(obj->fields + F_LEN);
	
        // return len;
        W_INT(f->stack + f->SP, len);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
