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
#define F_SIZE	4


static sap_array* ensureCapacity(sap_vm *vm, sap_array *array, sap_int l)
{
    if (array->length >= l) return array;
    
    // Надо создать новый массив
    l+=ADD;
    sap_array *array2=sapvm_new_Object_array(vm->clsid_Object, l);
    if (!array2) return 0;
    
    // Копируем данные
    memcpy(array2->data.r, array->data.r, array->length*4);
    memset(array->data.r, 0x00, array->length*4);	// чтоб дважды не удалять данные
    
    sapvm_dec_ref(vm, (sap_object*)array);
    
    return array2;
}


// Вызвать нативную функцию Vector.*
sap_byte sapvm_call_native_Vector(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0x3e42d2e6)
    {
	// native constructor();
	
	// Запоминаем данные
	sap_array *array=sapvm_new_Object_array(vm->clsid_Object, INIT);
	if (!array) return SAP_FAULT_NO_MEMORY;
	W_REF(obj->fields + F_DATA, array);
	W_INT(obj->fields + F_SIZE, 0);
	
        // return 0;
        W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0xe734e847)
    {
	// native void trimToSize();
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	
	// Проверим - надо ли что-то делать
	if (array->length != size)
	{
	    // Создаем новый массив
	    sap_array *array2=sapvm_new_Object_array(vm->clsid_Object, size);
	    if (array2)
	    {
		memcpy(array2->data.r, array->data.r, size*4);
		sap_int i;
		for (i=size; i<array->length; i++)
		{
		    if (array->data.r[i])
		    {
			sapvm_dec_ref(vm, array->data.r[i]);
			array->data.r[i]=0;
		    }
		}
		memset(array->data.r, 0x00, array->length*4);
		sapvm_dec_ref(vm, (sap_object*)array);
		W_REF(obj->fields + F_DATA, array2);
	    }
	}
	
        // return 0;
        W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x0864805c)
    {
	// native void setSize(int newSize);
	sap_int newSize=R_INT(params+0);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	
	array=ensureCapacity(vm, array, newSize);
	if (!array) return SAP_FAULT_NO_MEMORY;
	size=newSize;
	
	// Убираем лишние элементы
	sap_int i;
	for (i=newSize; i<array->length; i++)
	{
	    if (array->data.r[i])
	    {
		sapvm_dec_ref(vm, array->data.r[i]);
		array->data.r[i]=0;
	    }
	}
	
	W_REF(obj->fields + F_DATA, array);
	W_INT(obj->fields + F_SIZE, size);
	
        // return 0;
        W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x2e25d11e)
    {
	// native int size();
	sap_int size=R_INT(obj->fields + F_SIZE);
	
        // return size;
        W_INT(f->stack + f->SP, size);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x66017069)
    {
	// native boolean isEmpty();
	sap_int size=R_INT(obj->fields + F_SIZE);
	
        // return (size==0);
        W_REF(f->stack + f->SP, (size==0)?1:0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0xff99cda6)
    {
	// native Object elementAt(int index);
	sap_int idx=R_INT(params+0);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if ( (idx<0) || (idx>=size) ) return SAP_FAULT_ARRAY_INDEX;
	
	sap_object *o=array->data.r[idx];
	sapvm_inc_ref(vm, o);
	
        // return o;
        W_REF(f->stack + f->SP, o);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x2bee001b)
    {
	// native void setElementAt(Object obj, int index);
	sap_object *o=R_REF(params+0);
	sap_int idx=R_INT(params+4);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if ( (idx<0) || (idx>=size) ) return SAP_FAULT_ARRAY_INDEX;
	
	sap_object *old=array->data.r[idx];
	sapvm_dec_ref(vm, old);
	
	array->data.r[idx]=o;
	// ref-count для объекта не меняем, т.к. его просто запоминаем
	
        // return 0;
        W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0xdeb80860)
    {
	// native void removeElementAt(int index);
	sap_int idx=R_INT(params+0);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if ( (idx<0) || (idx>=size) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Удаляем элемент
	sapvm_dec_ref(vm, array->data.r[idx]);
	
	// Сдвигаем элементы
	sap_int i;
	for (i=idx+1; i<size; i++)
	    array->data.r[i-1]=array->data.r[i];
	array->data.r[size-1]=0;
	
	size--;
	W_INT(obj->fields + F_SIZE, size);
	
        // return 0;
        W_REF(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x3edf59b5)
    {
	// native void insertElementAt(Object obj, int index);
	sap_object *o=R_REF(params+0);
	sap_int idx=R_INT(params+4);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if ( (idx<0) || (idx>size) ) return SAP_FAULT_ARRAY_INDEX;
	
	array=ensureCapacity(vm, array, size+1);
	if (!array) return SAP_FAULT_NO_MEMORY;
	
	// Сдвигаем элементы
	sapvm_dec_ref(vm, array->data.r[size]);
	array->data.r[size]=0;
	sap_int i;
	for (i=size-1; i>=idx; i--)
	    array->data.r[i+1]=array->data.r[i];
	
	// Устанавливаем элемент
	array->data.r[idx]=o;
	
	size++;
	W_REF(obj->fields + F_DATA, array);
	W_INT(obj->fields + F_SIZE, size);
	
        // return 0;
        W_REF(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0xf20f89c0)
    {
	// native void add(Object obj);
	sap_object *o=R_REF(params+0);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	
	array=ensureCapacity(vm, array, size+1);
	if (!array) return SAP_FAULT_NO_MEMORY;
	
	// Добавляем элемент
	array->data.r[size++]=o;
	
	W_REF(obj->fields + F_DATA, array);
	W_INT(obj->fields + F_SIZE, size);
	
        // return 0;
        W_REF(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0xea278f60)
    {
	// native void removeAllElements();
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	
	// Удаляем элементы
	sap_int i;
	for (i=0; i<array->length; i++)
	{
	    if (array->data.r[i])
	    {
		sapvm_dec_ref(vm, array->data.r[i]);
		array->data.r[i]=0;
	    }
	}
	size=0;
	
	W_INT(obj->fields + F_SIZE, size);
	
        // return 0;
        W_REF(f->stack + f->SP, 0);
        f->SP+=4;
	
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x7e71099c)
    {
	// native Object set(int index, Object element);
	sap_int idx=R_INT(params+0);
	sap_object *o=R_REF(params+4);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if ( (idx<0) || (idx>=size) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Берем старый элемент
	sap_object *old=array->data.r[idx];
	
	// Устанавливаем новый
	array->data.r[idx]=o;
	
	// ref-count не трогаем
	
        // return old;
        W_REF(f->stack + f->SP, old);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x2709542a)
    {
	// native Object remove(int index);
	sap_int idx=R_INT(params+0);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if ( (idx<0) || (idx>=size) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Удаляем элемент
	sap_object *old=array->data.r[idx];
	
	// Сдвигаем элементы
	sap_int i;
	for (i=idx+1; i<size; i++)
	    array->data.r[i-1]=array->data.r[i];
	array->data.r[size-1]=0;
	
	size--;
	W_INT(obj->fields + F_SIZE, size);
	
        // return old;
        W_REF(f->stack + f->SP, old);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0xa1435670)
    {
	// native void removeElement(Object obj);
	sap_object *o=R_REF(params+0);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	
	// Ищем элемент
	sap_int i, idx=-1;
	for (i=0; i<size; i++)
	{
	    if (array->data.r[i]==o)
	    {
		idx=i;
		break;
	    }
	}
	
	// Забываем параметр
	sapvm_dec_ref(vm, o);
	
	if (idx>=0)
	{
	    // Удаляем элемент
	    sapvm_dec_ref(vm, o);
	    
	    // Сдвигаем элементы
	    for (i=idx+1; i<size; i++)
		array->data.r[i-1]=array->data.r[i];
	    array->data.r[size-1]=0;
	    
	    size--;
	    W_INT(obj->fields + F_SIZE, size);
	}
	
	// return 0;
    	W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x07d96ae9)
    {
	// native void addAll(Vector v);
	sap_object *v=R_REF(params+0);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if (!v) return SAP_FAULT_NULL_REF;
	
	sap_array *array2=(sap_array*)R_REF(v->fields + F_DATA);
	sap_int size2=R_INT(v->fields + F_SIZE);
	
	if (!array2) return SAP_FAULT_NULL_REF;
	
	array=ensureCapacity(vm, array, size+size2);
	if (!array) return SAP_FAULT_NO_MEMORY;
	
	// Добавляем элементы
	sap_int i;
	for (i=0; i<size2; i++)
	{
	    if (array->data.r[size+i]) sapvm_dec_ref(vm, array->data.r[size+i]);
	    array->data.r[size+i]=array2->data.r[i];
	    sapvm_inc_ref(vm, array->data.r[size+i]);
	}
	size+=size2;
	
	// Забываем параметр
	sapvm_dec_ref(vm, v);
	
	W_REF(obj->fields + F_DATA, array);
	W_INT(obj->fields + F_SIZE, size);
	
        // return 0;
        W_REF(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x700dadb5)
    {
	// native boolean contains(Object obj);
	sap_object *o=R_REF(params+0);
	sap_array *array=(sap_array*)R_REF(obj->fields + F_DATA);
	sap_int size=R_INT(obj->fields + F_SIZE);
	
	if (!array) return SAP_FAULT_NULL_REF;
	
	// Ищем элемент
	sap_byte result=0;
	sap_int i;
	for (i=0; i<size; i++)
	{
	    if (array->data.r[i]==o)
	    {
		result=1;
		break;
	    }
	}
	
	// Забываем параметр
	sapvm_dec_ref(vm, o);
	
	// return result;
    	W_INT(f->stack + f->SP, result);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
