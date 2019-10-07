#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <math.h>


// Вызвать нативную функцию Math.*
sap_byte sapvm_call_native_Math(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0x37a26bed)
    {
	// native static int Math.abs(int x);
	sap_int x=R_INT(params+0);
	
	if (x<0) x=-x;
	
	// return |x|;
	W_INT(f->stack + f->SP, x);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x8ab97c3a)
    {
	// native static float Math.fabs(float x);
	sap_float x=R_FLOAT(params+0);
	
	if (x < 0.0) x=-x;
	
	// return |x|;
	W_FLOAT(f->stack + f->SP, x);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xbe61a6dd)
    {
	// native static float Math.ceil(float x);
	sap_float x=R_FLOAT(params+0);
	
	// return ceilf(x);
	W_FLOAT(f->stack + f->SP, ceilf(x));
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x0d497138)
    {
	// native static float Math.floor(float x);
	sap_float x=R_FLOAT(params+0);
	
	// return floorf(x);
	W_FLOAT(f->stack + f->SP, floorf(x));
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xaeb71f56)
    {
	// native static float Math.sin(float x);
	sap_float x=R_FLOAT(params+0);
	
	// return sin(x);
	W_FLOAT(f->stack + f->SP, sinf(x));
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x874033b6)
    {
	// native static float Math.cos(float x);
	sap_float x=R_FLOAT(params+0);
	
	// return cos(x);
	W_FLOAT(f->stack + f->SP, cosf(x));
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x8743a3b5)
    {
	// native static float Math.tan(float x);
	sap_float x=R_FLOAT(params+0);
	
	// return tan(x);
	W_FLOAT(f->stack + f->SP, tanf(x));
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xc444da60)
    {
	// native static int Math.min(int x, int y);
	sap_int x=R_INT(params+0);
	sap_int y=R_INT(params+4);
	
	if (x > y) x=y;
	
	// return min(x,y);
	W_INT(f->stack + f->SP, x);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0xc035c063)
    {
	// native static float Math.fmin(float x, float y);
	sap_float x=R_FLOAT(params+0);
	sap_float y=R_FLOAT(params+4);
	
	if (x > y) x=y;
	
	// return min(x,y);
	W_FLOAT(f->stack + f->SP, x);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x22247581)
    {
	// native static int Math.max(int x, int y);
	sap_int x=R_INT(params+0);
	sap_int y=R_INT(params+4);
	
	if (x < y) x=y;
	
	// return max(x,y);
	W_INT(f->stack + f->SP, x);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x26556f82)
    {
	// native static float Math.fmax(float x, float y);
	sap_float x=R_FLOAT(params+0);
	sap_float y=R_FLOAT(params+4);
	
	if (x < y) x=y;
	
	// return max(x,y);
	W_FLOAT(f->stack + f->SP, x);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x68ca0130)
    {
	// native static float Math.sqrt(float x);
	sap_float x=R_FLOAT(params+0);
	
	// return sqrt(x);
	W_FLOAT(f->stack + f->SP, sqrtf(x));
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
