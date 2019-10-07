#ifndef SAP_NATIVE_H
#define SAP_NATIVE_H


#include "sapvm.h"


// Создать массив байт указанной длины (заполняется нулями)
sap_array* sapvm_new_byte_array(sap_int size);

// Создать массив short-ов указанной длины (заполняется нулями)
sap_array* sapvm_new_short_array(sap_int size);

// Создать массив Object-ов указанного типа и длины (заполняется нулями)
sap_array* sapvm_new_Object_array(sap_ubyte cls, sap_int size);

// Создает объект String
sap_object* sapvm_new_String(sap_vm *vm, sap_array *stringData);

// Создает объект StringBuffer
sap_object* sapvm_new_StringBuffer(sap_vm *vm, sap_int initial_size);


// Вызвать нативную функцию
sap_byte sapvm_call_native(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);

// Для разных классов:
sap_byte sapvm_call_native_System(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_Integer(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_Float(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_Math(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_Thread(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_IPC(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_Random(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_Pack(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_String(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_StringBuffer(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_DateTime(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_Vector(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);
sap_byte sapvm_call_native_CRC16(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);


#endif
