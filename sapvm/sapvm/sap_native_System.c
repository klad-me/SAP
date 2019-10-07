#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <stdio.h>
#include <string.h>


// Вызвать нативную функцию System.*
sap_byte sapvm_call_native_System(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0x25febbb5)
    {
	// native static void System.arrayCopy(Object src, int srcOffs, Object dst, int dstOffs, int len);
	sap_array *src=(sap_array*)R_REF(params+0);
	sap_int srcOffs=R_INT(params+4);
	sap_array *dst=(sap_array*)R_REF(params+8);
	sap_int dstOffs=R_INT(params+12);
	sap_int len=R_INT(params+16);
	
	// Должны быть типы-массивы
	if (!src) return SAP_FAULT_NULL_REF;
	if (!dst) return SAP_FAULT_NULL_REF;
	if ( (!(src->ID & 0x8000)) || (src->ID != dst->ID) ) return SAP_FAULT_BAD_SPEC;
	
	if (len>0)
	{
	    // Есть что копировать
	    
	    // Проверяем размерности
	    if ( (srcOffs < 0) ||
		 (dstOffs < 0) ||
		 (len < 0) ||
		 (srcOffs+len > src->length) ||
		 (dstOffs+len > dst->length) )
	    {
		/*printf("src.length=%d srcOffs=%d dst.length=%d dstOffs=%d len=%d\n",
		    src->length, srcOffs,
		    dst->length, dstOffs,
		    len );*/
		return SAP_FAULT_ARRAY_INDEX;
	    }
	    
	    if ((src->ID & 0xf000) == 0x8000)
	    {
		// Массив байт
		memmove((void*)(dst->data.b+dstOffs), (void*)(src->data.b+srcOffs), len);
	    } else
	    if ((src->ID & 0xf000) == 0x9000)
	    {
		// Массив short-ов
		memmove((void*)(dst->data.b+dstOffs*2), (void*)(src->data.b+srcOffs*2), len*2);
	    } else
	    if ((src->ID & 0xf000) == 0xa000)
	    {
		// Массив int-ов или float-ов
		memmove((void*)(dst->data.i+dstOffs*4), (void*)(src->data.i+srcOffs*4), len*4);
	    } else
	    if ((src->ID & 0xf000) == 0xb000)
	    {
		// Массив ссылок
		sap_int i;
		
		if (src != dst)
		{
		    // Разные массивы
		    for (i=0; i<len; i++)
		    {
			sap_object *o=dst->data.r[i+dstOffs];
			
			dst->data.r[i+dstOffs]=src->data.r[i+srcOffs];
			sapvm_inc_ref(vm, dst->data.r[i+dstOffs]);
			sapvm_dec_ref(vm, o);
		    }
		} else
		{
		    // Один и тот же массив. Надо проверить индексы, чтобы корректно работать при пересечении
		    if (srcOffs <= dstOffs)
		    {
			// Обратное копирование
			for (i=len-1; i>0; i--)
			{
			    sap_object *o=dst->data.r[i+dstOffs];
			    
			    dst->data.r[i+dstOffs]=src->data.r[i+srcOffs];
			    sapvm_inc_ref(vm, dst->data.r[i+dstOffs]);
			    sapvm_dec_ref(vm, o);
			}
		    } else
		    {
			// Прямое копирование
			for (i=0; i<len; i++)
			{
			    sap_object *o=dst->data.r[i+dstOffs];
			    
			    dst->data.r[i+dstOffs]=src->data.r[i+srcOffs];
			    sapvm_inc_ref(vm, dst->data.r[i+dstOffs]);
			    sapvm_dec_ref(vm, o);
			}
		    }
		}
	    } else
	    {
		// Ошибочный тип
		return SAP_FAULT_BAD_SPEC;
	    }
	}
	
	// Забываем массивы
	sapvm_dec_ref(vm, (sap_object*)src);
	sapvm_dec_ref(vm, (sap_object*)dst);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x462022D4)
    {
	// native static void System.__print(byte[] str);
	sap_array *array=(sap_array*)R_REF(params);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	sapvm_print(array->data.b, array->length);
	
	// Забываем массив
	sapvm_dec_ref(vm, (sap_object*)array);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xde3d8fa9)
    {
	// native static int System.__argc();
	
	// return argc;
	W_INT(f->stack + f->SP, vm->argc);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x5d075f23)
    {
	// native static byte[] System.__argv(int n);
	sap_int n=R_INT(params+0);
	
	sap_array* array=0;
	
	if ( (n>=0) && (n<vm->argc) )
	{
	    // Копируем значение
	    array=sapvm_new_byte_array(strlen(vm->argv[n]));
	    if (array)
	    {
		memcpy(array->data.b, vm->argv[n], array->length);
	    }
	}
	
	// return array;
	W_REF(f->stack + f->SP, array);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x7c2e9ca5)
    {
	// native static int System.currentTick();
	
	// return tm;
	W_INT(f->stack + f->SP, (sap_int)TIME_COUNTER());
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x23c155f7)
    {
	// native static int System.currentTime();
	
	// return tm;
	W_INT(f->stack + f->SP, (sap_int)sapvm_get_current_time());
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x74325c5b)
    {
	// native static void System.setCurrentTime(int ut);
	sap_int ut=R_INT(params+0);
	
	sapvm_set_current_time((sap_uint)ut);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x0f2a9993)
    {
	// native static byte[] System.__getProperty(byte[] name);
	sap_array *name=(sap_array*)R_REF(params);
	sap_array *value=0;
	
	if (!name) return SAP_FAULT_NULL_REF;
	if (name->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Создаем строку
	char *sname=sapvm_byte_array_to_string(name);
	if (sname)
	{
	    const char *svalue=sapvm_get_property(sname);
	    FREE((void*)sname);
	    if (svalue)
	    {
		// Есть значение
		value=sapvm_new_byte_array(strlen(svalue));
		if (value)
		{
		    memcpy(value->data.b, svalue, value->length);
		}
		FREE((void*)svalue);
	    }
	}
	
	// Забываем имя
	sapvm_dec_ref(vm, (sap_object*)name);
	
	// return value;
	W_REF(f->stack + f->SP, value);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xfed18bbc)
    {
	// native static void System.__setProperty(byte[] name, byte[] value);
	sap_array *name=(sap_array*)R_REF(params+0);
	sap_array *value=(sap_array*)R_REF(params+4);
	
	if (!name) return SAP_FAULT_NULL_REF;
	if (name->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	if (!value) return SAP_FAULT_NULL_REF;
	if (value->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Создаем строки
	char *sname=sapvm_byte_array_to_string(name);
	char *svalue=sapvm_byte_array_to_string(value);
	if ( (sname) && (svalue) )
	{
	    sapvm_set_property(sname, svalue);
	}
	if (sname) FREE((void*)sname);
	if (svalue) FREE((void*)svalue);
	
	// Забываем имя и значение
	sapvm_dec_ref(vm, (sap_object*)name);
	sapvm_dec_ref(vm, (sap_object*)value);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xdb73f559)
    {
	// native static void System.exit(int code);
	sap_int code=R_INT(params+0);
	
	sapvm_exit(code);
	return SAP_FAULT_BAD_OPCODE;	// по хорошему - не должно сюда вернуться
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
