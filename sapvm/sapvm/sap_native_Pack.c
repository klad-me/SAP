#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"


// Вызвать нативную функцию Pack.*
sap_byte sapvm_call_native_Pack(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0xd55af517)
    {
	// native static short Pack.r_short(byte[] b, int offs);
	sap_array *array=(sap_array*)R_REF(params+0);
	sap_int offs=R_INT(params+4);
	sap_ushort result;
	
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	if ( (offs < 0) ||
	     (offs+2 > array->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Получаем результат
	result=(sap_ushort) ( ( ((sap_ubyte)array->data.b[offs+1]) << 8) | ((sap_ubyte)array->data.b[offs+0]) );
	
	// Забываем массив
	sapvm_dec_ref(vm, (sap_object*)array);
	
	// return result;
	W_INT(f->stack + f->SP, (sap_short)result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x95c24b96)
    {
	// native static void Pack.w_short(short v, byte[] b, int offs);
	sap_ushort v=(sap_ushort)R_SHORT(params+0);
	sap_array *array=(sap_array*)R_REF(params+2);
	sap_int offs=R_INT(params+6);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	if ( (offs < 0) ||
	     (offs+2 > array->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Устанавливаем значение
	array->data.b[offs+1]=v >> 8;
	array->data.b[offs+0]=v & 0xff;
	
	// Забываем массив
	sapvm_dec_ref(vm, (sap_object*)array);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x15bb492b)
    {
	// native static int Pack.r_int(byte[] b, int offs);
	sap_array *array=(sap_array*)R_REF(params+0);
	sap_int offs=R_INT(params+4);
	sap_uint result;
	
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	if ( (offs < 0) ||
	     (offs+4 > array->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Получаем результат
	result= ((((sap_ubyte)array->data.b[offs+3]) << 24) & 0xff000000) |
		((((sap_ubyte)array->data.b[offs+2]) << 16) & 0x00ff0000) |
		((((sap_ubyte)array->data.b[offs+1]) <<  8) & 0x0000ff00) |
		((((sap_ubyte)array->data.b[offs+0]) <<  0) & 0x000000ff);
	
	// Забываем массив
	sapvm_dec_ref(vm, (sap_object*)array);
	
	// return result;
	W_INT(f->stack + f->SP, (sap_int)result);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xb40d14f6)
    {
	// native static void Pack.w_int(int v, byte[] b, int offs);
	sap_uint v=(sap_uint)R_INT(params+0);
	sap_array *array=(sap_array*)R_REF(params+4);
	sap_int offs=R_INT(params+8);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	if ( (offs < 0) ||
	     (offs+2 > array->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Устанавливаем значение
	array->data.b[offs+3]=(sap_byte)((v >> 24) & 0xff);
	array->data.b[offs+2]=(sap_byte)((v >> 16) & 0xff);
	array->data.b[offs+1]=(sap_byte)((v >>  8) & 0xff);
	array->data.b[offs+0]=(sap_byte)((v >>  0) & 0xff);
	
	// Забываем массив
	sapvm_dec_ref(vm, (sap_object*)array);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x9cf9efc7)
    {
	// native static float Pack.r_float(byte[] b, int offs);
	sap_array *array=(sap_array*)R_REF(params+0);
	sap_int offs=R_INT(params+4);
	sap_uint iresult;
	sap_float *fresult=(sap_float*)&iresult;
	
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	if ( (offs < 0) ||
	     (offs+4 > array->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Получаем результат
	iresult=((((sap_ubyte)array->data.b[offs+3]) << 24) & 0xff000000) |
		((((sap_ubyte)array->data.b[offs+2]) << 16) & 0x00ff0000) |
		((((sap_ubyte)array->data.b[offs+1]) <<  8) & 0x0000ff00) |
		((((sap_ubyte)array->data.b[offs+0]) <<  0) & 0x000000ff);
	
	// Забываем массив
	sapvm_dec_ref(vm, (sap_object*)array);
	
	// return result;
	W_FLOAT(f->stack + f->SP, *fresult);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x77f64f64)
    {
	// native static void Pack.w_float(float v, byte[] b, int offs);
	sap_uint iv;
	sap_float *fv=(sap_float*)&iv;
	(*fv)=R_FLOAT(params+0);
	sap_array *array=(sap_array*)R_REF(params+4);
	sap_int offs=R_INT(params+8);
	
	if (!array) return SAP_FAULT_NULL_REF;
	if (array->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	if ( (offs < 0) ||
	     (offs+2 > array->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Устанавливаем значение
	array->data.b[offs+3]=(sap_byte)((iv >> 24) & 0xff);
	array->data.b[offs+2]=(sap_byte)((iv >> 16) & 0xff);
	array->data.b[offs+1]=(sap_byte)((iv >>  8) & 0xff);
	array->data.b[offs+0]=(sap_byte)((iv >>  0) & 0xff);
	
	// Забываем массив
	sapvm_dec_ref(vm, (sap_object*)array);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
