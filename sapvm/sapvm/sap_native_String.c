#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <stdio.h>
#include <string.h>


// Вызвать нативную функцию String.*
sap_byte sapvm_call_native_String(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0xb398c67e)
    {
	// native constructor(byte[] data);
	sap_array *array=(sap_array*)R_REF(params+0);
	
	// Проверим строку
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Запоминаем строку (ref_count трогать не надо, т.к. берем из параметра и помним в поле -  +1, -1)
	W_REF(obj->fields + 0, array);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x89891ccd)
    {
	// native int length();
	
	// Получим строку
	sap_array *array=(sap_array*)R_REF(obj->fields + 0);
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// return length;
	W_INT(f->stack + f->SP, array->length);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x9f83a925)
    {
	// native byte charAt(int idx);
	sap_int idx=R_INT(params+0);
	
	// Получим строку
	sap_array *array=(sap_array*)R_REF(obj->fields + 0);
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Проверим индекс
	if ( (idx < 0) || (idx >= array->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Получаем символ
	sap_byte c=array->data.b[idx];
	
	// return c;
	W_INT(f->stack + f->SP, c);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xedb3add3)
    {
	// native byte[] getBytes();
	
	// Получим строку
	sap_array *array=(sap_array*)R_REF(obj->fields + 0);
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	sapvm_inc_ref(vm, (sap_object*)array);
	
	// return data;
	W_REF(f->stack + f->SP, array);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x349c8474)
    {
	// native short[] toUnicode();
	
	// Получим строку
	sap_array *array=(sap_array*)R_REF(obj->fields + 0);
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Создаем массив UTF8
	sap_array *unicode=sapvm_new_short_array(array->length);
	if (unicode)
	{
	    // Заполняем данные
	    sap_int i;
	    for (i=0; i<array->length; i++)
	    {
		unicode->data.s[i]=sapvm_8bit_to_unicode(array->data.b[i]);
	    }
	}
	
	// return unicode;
	W_REF(f->stack + f->SP, unicode);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xcfded02a)
    {
	// native String fromUnicode(short[] data, int offs, int size);
	sap_array *array=(sap_array*)R_REF(params+0);
	sap_int offs=R_INT(params+4);
	sap_int size=R_INT(params+8);
	
	// Проверим данные
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x9100) return SAP_FAULT_BAD_SPEC;
	if ( (offs < 0) ||
	     (size < 0) ||
	     (offs+size > array->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Выделяем строку
	sap_object *result=0;
	sap_array *str=sapvm_new_byte_array(size);
	if (str)
	{
	    // Заполняем данные
	    sap_int i;
	    for (i=0; i<size; i++)
	    {
		str->data.b[i]=(sap_byte)sapvm_unicode_to_8bit( (sap_ushort)array->data.s[offs+i] );
	    }
	    
	    // Создаем объект-строку
	    result=sapvm_new_String(vm, str);
	    if (!result) FREE((void*)str);
	}
	
	// Забываем параметр
	sapvm_dec_ref(vm, (sap_object*)array);
	
	// return result;
	W_REF(f->stack + f->SP, result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xb9c95b20)
    {
	// native byte[] toUTF8();
	
	// Получим строку
	sap_array *array=(sap_array*)R_REF(obj->fields + 0);
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	sap_int l=0, i;
	
	// Считаем длину получающийся строки
	for (i=0; i<array->length; i++)
	{
	    sap_ushort u=sapvm_8bit_to_unicode(array->data.b[i]);
	    
	    if (u < 0x80) l++; else
	    if (u < 0x800) l+=2; else
		l+=3;
	}
	
	// Создаем массив UTF8
	sap_array *utf8=sapvm_new_byte_array(l);
	if (utf8)
	{
	    // Заполняем данные
	    sap_int p=0;
	    for (i=0; i<array->length; i++)
	    {
		sap_ushort u=sapvm_8bit_to_unicode(array->data.b[i]);
		
		if (u < 0x80)
		{
		    utf8->data.b[p]=(sap_byte)u;
		    p++;
		} else
		if (u < 0x800)
		{
		    utf8->data.b[p+0]=(sap_byte)( ((u>>6) & 0x1f) | 0xc0 );
		    utf8->data.b[p+1]=(sap_byte)( (u & 0x3f) | 0x80 );
		    p+=2;
		} else
		{
		    utf8->data.b[p+0]=(sap_byte)( ((u>>12) & 0x0f) | 0xe0 );
		    utf8->data.b[p+1]=(sap_byte)( ((u>>6) & 0x3f) | 0x80 );
		    utf8->data.b[p+2]=(sap_byte)( (u & 0x3f) | 0x80 );
		    p+=3;
		}
	    }
	}
	
	// return utf8;
	W_REF(f->stack + f->SP, utf8);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x9dbbab34)
    {
	// native static String fromUTF8(byte[] data, int offs, int size);
	sap_array *array=(sap_array*)R_REF(params+0);
	sap_int offs=R_INT(params+4);
	sap_int size=R_INT(params+8);
	
	// Проверим строку
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	if ( (offs < 0) ||
	     (size < 0) ||
	     (offs+size > array->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Считаем длину получающийся строки
	sap_int p=0, l=0;
	while ( (p < size) &&
		(array->data.b[offs+p]!=0) )
	{
	    sap_ubyte b=(sap_ubyte)array->data.b[offs+p];
	    
	    if ( (b & 0x80) == 0x00 )
	    {
		// 1 байт
		l++;
		p++;
	    } else
	    if ( ( (b & 0xe0) == 0xc0 ) &&
		 (size-p >= 2) )
	    {
		// 2 байта
		l++;
		p+=2;
	    } else
	    if ( ( (b & 0xf0) == 0xe0 ) &&
		 (size-p >= 3) )
	    {
		// 3 байта
		l++;
		p+=3;
	    } else
	    {
		// Ошибка ! - обрубаем строку
		size-=(size-p);
		break;
	    }
	}
	
	// Выделяем строку
	sap_object *result=0;
	sap_array *str=sapvm_new_byte_array(l);
	if (str)
	{
	    // Заполняем данные
	    p=0;
	    l=0;
	    while ( (p < size) &&
		    (array->data.b[offs+p]!=0) )
	    {
		sap_ubyte b=(sap_ubyte)array->data.b[offs+p];
		sap_ushort u=0;
		
		if ( (b & 0x80) == 0x00 )
		{
		    // 1 байт
		    u=(sap_ushort)b;
		    
		    p++;
		} else
		if ( ( (b & 0xe0) == 0xc0 ) &&
		     (size-p >= 2) )
		{
		    // 2 байта
		    u=b & 0x1f;
		    u<<=6;
		    u|=((sap_ubyte)array->data.b[offs+p+1]) & 0x3f;
		    
		    p+=2;
		} else
		if ( ( (b & 0xf0) == 0xe0 ) &&
		     (size-p >= 3) )
		{
		    // 3 байта
		    u=b & 0x0f;
		    u<<=6;
		    u|=((sap_ubyte)array->data.b[offs+p+1]) & 0x3f;
		    u<<=6;
		    u|=((sap_ubyte)array->data.b[offs+p+2]) & 0x3f;
		    
		    p+=3;
		} else
		{
		    // Ошибка !
		    break;
		}
		
		str->data.b[l++]=(sap_byte)sapvm_unicode_to_8bit(u);
	    }
	    
	    // Создаем объект-строку
	    result=sapvm_new_String(vm, str);
	    if (!result) FREE((void*)str);
	}
	
	// Забываем параметр
	sapvm_dec_ref(vm, (sap_object*)array);
	
	// return result;
	W_REF(f->stack + f->SP, result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x6c4c470a)
    {
	// native boolean equals(Object o);
	sap_object *o=R_REF(params+0);
	
	if (!o) return SAP_FAULT_NULL_REF;
	
	sap_byte ok=0;
	
	if (o==obj)
	{
	    // Тот же объект
	    ok=1;
	} else
	if ( (o) && (sapvm_is_instanceof(vm, o, vm->clsid_String)) )
	{
	    // Получаем строки
	    sap_array *array1=(sap_array*)R_REF(obj->fields+0);
	    sap_array *array2=(sap_array*)R_REF(o->fields+0);
	    
	    // Проверим строку
	    if ( (!array1) || (!array2) ) return SAP_FAULT_NULL_REF;
	    if (array1->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	    if (array2->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	    
	    // Сравниваем
	    if (array1->length == array2->length)
	    {
		ok=1;
		sap_int i;
		for (i=0; i<array1->length; i++)
		{
		    if (array1->data.b[i] != array2->data.b[i])
		    {
			ok=0;
			break;
		    }
		}
	    }
	}
	
	// Забываем параметры
	sapvm_dec_ref(vm, (sap_object*)o);
	
	// return ok;
	W_INT(f->stack + f->SP, ok);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x497ab04d)
    {
	// native int compareTo(String s);
	sap_object *o=R_REF(params+0);
	
	if (!o) return SAP_FAULT_NULL_REF;
	
	// Получаем строки
	sap_array *array1=(sap_array*)R_REF(obj->fields+0);
	sap_array *array2=(sap_array*)R_REF(o->fields+0);
	
	// Проверим строки
	if ( (!array1) || (!array2) ) return SAP_FAULT_NULL_REF;
	if (array1->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	if (array2->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Сравниваем
	sap_int l=(array1->length > array2->length)?array2->length : array1->length;
	sap_int r=memcmp(array1->data.b, array2->data.b, l);
	if (r==0)
	{
	    // Начала строк одинаковые - сравниваем размеры
	    if (array1->length < array2->length) r=-1; else
	    if (array1->length > array2->length) r=1;
	}
	
	// Забываем параметры
	sapvm_dec_ref(vm, (sap_object*)o);
	
	// return r;
	W_INT(f->stack + f->SP, r);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xb055e3e5)
    {
	// native int indexOf(byte c);
	sap_byte c=R_BYTE(params+0);
	
	// Получим строку
	sap_array *array=(sap_array*)R_REF(obj->fields + 0);
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Ищем
	sap_int result=-1;
	sap_int i;
	for (i=0; i<array->length; i++)
	{
	    if (array->data.b[i] == c)
	    {
		result=i;
		break;
	    }
	}
	
	// return result;
	W_INT(f->stack + f->SP, result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xf09150d3)
    {
	// native boolean startsWith(String str);
	sap_object *o=R_REF(params+0);
	if (!o) return SAP_FAULT_NULL_REF;
	
	// Получаем строки
	sap_array *array1=(sap_array*)R_REF(obj->fields+0);
	sap_array *array2=(sap_array*)R_REF(o->fields+0);
	
	// Проверим строки
	if ( (!array1) || (!array2) ) return SAP_FAULT_NULL_REF;
	if (array1->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	if (array2->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Сравниваем
	sap_byte result=1;
	if (array2->length > array1->length) result=0; else
	{
	    sap_int i;
	    for (i=0; i<array2->length; i++)
	    {
		if (array1->data.b[i] != array2->data.b[i])
		{
		    result=0;
		    break;
		}
	    }
	}
	
	// Забываем параметры
	sapvm_dec_ref(vm, (sap_object*)o);
	
	// return result;
	W_INT(f->stack + f->SP, result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x3c668e8a)
    {
	// native boolean endsWith(String str);
	sap_object *o=R_REF(params+0);
	if (!o) return SAP_FAULT_NULL_REF;
	
	// Получаем строки
	sap_array *array1=(sap_array*)R_REF(obj->fields+0);
	sap_array *array2=(sap_array*)R_REF(o->fields+0);
	
	// Проверим строки
	if ( (!array1) || (!array2) ) return SAP_FAULT_NULL_REF;
	if (array1->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	if (array2->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Сравниваем
	sap_byte result=1;
	if (array2->length > array1->length) result=0; else
	{
	    sap_int offs=array1->length - array2->length;
	    sap_int i;
	    for (i=0; i<array2->length; i++)
	    {
		if (array1->data.b[offs+i] != array2->data.b[i])
		{
		    result=0;
		    break;
		}
	    }
	}
	
	// Забываем параметры
	sapvm_dec_ref(vm, (sap_object*)o);
	
	// return result;
	W_INT(f->stack + f->SP, result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xf0e24e66)
    {
	// native String substring(int begin, int end);
	sap_int begin=R_INT(params+0);
	sap_int end=R_INT(params+4);
	
	// Получим строку
	sap_array *array=(sap_array*)R_REF(obj->fields + 0);
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Работаем
	if (begin < 0) begin=0;
	if (begin > array->length) begin=array->length;
	if (end < begin) end=begin;
	if (end > array->length) end=array->length;
	sap_int l=end-begin;
	
	// Создаем строку
	sap_object *result=0;
	sap_array *str=sapvm_new_byte_array(l);
	if (str)
	{
	    sap_int i;
	    for (i=0; i<l; i++)
	    {
		str->data.b[i]=array->data.b[begin+i];
	    }
	    
	    // Создаем объект-строку
	    result=sapvm_new_String(vm, str);
	    if (!result) FREE((void*)str);
	}
	
	// return result;
	W_REF(f->stack + f->SP, result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x35b4cfc3)
    {
	// native String append(String str);
	sap_object *o=R_REF(params+0);
	
	sap_object *result=0;
	if (!o)
	{
	    // Прибавляют null
	    result=obj;
	    sapvm_inc_ref(vm, obj);
	} else
	{
	    sap_array *array1=(sap_array*)R_REF(obj->fields+0);
	    sap_array *array2=(sap_array*)R_REF(o->fields+0);
	    
	    // Проверим строки
	    if ( (!array1) || (!array2) ) return SAP_FAULT_NULL_REF;
	    if (array1->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	    if (array2->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	    
	    // Создаем строку
	    sap_array *str=sapvm_new_byte_array(array1->length + array2->length);
	    if (str)
	    {
		sap_int i;
		for (i=0; i<array1->length; i++)
		{
		    str->data.b[i]=array1->data.b[i];
		}
		for (i=0; i<array2->length; i++)
		{
		    str->data.b[array1->length+i]=array2->data.b[i];
		}
		
		// Создаем объект-строку
		result=sapvm_new_String(vm, str);
		if (!result) FREE((void*)str);
	    }
	}
	
	// Забываем параметры
	sapvm_dec_ref(vm, (sap_object*)o);
	
	// return result;
	W_REF(f->stack + f->SP, result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x59967969)
    {
	// native String toLowerCase();
	
	// Получим строку
	sap_array *array=(sap_array*)R_REF(obj->fields + 0);
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Создаем новую строку
	sap_object *result=0;
	sap_array *str=sapvm_new_byte_array(array->length);
	if (str)
	{
	    sap_int i;
	    for (i=0; i<array->length; i++)
	    {
		str->data.b[i]=sapvm_lower_case(array->data.b[i]);
	    }
	    
	    // Создаем объект-строку
	    result=sapvm_new_String(vm, str);
	    if (!result) FREE((void*)str);
	}
	
	// return result;
	W_REF(f->stack + f->SP, result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x6cd95b85)
    {
	// native String toUpperCase();
	
	// Получим строку
	sap_array *array=(sap_array*)R_REF(obj->fields + 0);
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Создаем новую строку
	sap_object *result=0;
	sap_array *str=sapvm_new_byte_array(array->length);
	if (str)
	{
	    sap_int i;
	    for (i=0; i<array->length; i++)
	    {
		str->data.b[i]=sapvm_upper_case(array->data.b[i]);
	    }
	    
	    // Создаем объект-строку
	    result=sapvm_new_String(vm, str);
	    if (!result) FREE((void*)str);
	}
	
	// return result;
	W_REF(f->stack + f->SP, result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
