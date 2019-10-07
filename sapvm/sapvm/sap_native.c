#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"


// Создать массив байт указанной длины (заполняется нулями)
sap_array* sapvm_new_byte_array(sap_int size)
{
    sap_array *array=(sap_array*)ALLOC(8 + size, SAP_HEAP_ARRAY);
    if (array)
    {
        array->ID=0x8100;
        array->ref_count=1;
        array->length=size;
    }
    return array;
}


// Создать массив short-ов указанной длины (заполняется нулями)
sap_array* sapvm_new_short_array(sap_int size)
{
    sap_array *array=(sap_array*)ALLOC(8 + size*2, SAP_HEAP_ARRAY);
    if (array)
    {
        array->ID=0x9100;
        array->ref_count=1;
        array->length=size;
    }
    return array;
}


// Создать массив Object-ов указанного типа и длины (заполняется нулями)
sap_array* sapvm_new_Object_array(sap_ubyte cls, sap_int size)
{
    sap_array *array=(sap_array*)ALLOC(8 + size*4, SAP_HEAP_ARRAY);
    if (array)
    {
        array->ID=0xB100 | cls;
        array->ref_count=1;
        array->length=size;
    }
    return array;
}


// Создает объект String
sap_object* sapvm_new_String(sap_vm *vm, sap_array *stringData)
{
    sap_object *o=(sap_object*)ALLOC(4+4, SAP_HEAP_OBJECT);
    if (o)
    {
        o->ID=vm->clsid_String;
        o->ref_count=1;
        W_REF(o->fields+0, stringData);
    }
    return o;
}


// Создает объект StringBuffer
sap_object* sapvm_new_StringBuffer(sap_vm *vm, sap_int initial_size)
{
    sap_object *o=(sap_object*)ALLOC(4+8, SAP_HEAP_OBJECT);
    if (o)
    {
        o->ID=vm->clsid_StringBuffer;
        o->ref_count=1;
        W_REF(o->fields+0, sapvm_new_byte_array(initial_size));
        W_INT(o->fields+4, 0);	// length
    }
    return o;
}


// Вызвать нативную функцию
sap_byte sapvm_call_native(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0xB467EA61)
    {
	// void Object.<init>()
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    {
	// Ищем по классам
	sap_byte rv;
	
	rv=sapvm_call_native_String(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_StringBuffer(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_Thread(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_System(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_Integer(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_Float(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_Math(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_IPC(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_Random(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_Pack(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_DateTime(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_Vector(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_CRC16(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
	
	rv=sapvm_call_native_User(vm, Native_Hash, thr, f, obj, params_len, params);
	if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
    }
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
