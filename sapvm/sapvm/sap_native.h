#ifndef SAP_NATIVE_H
#define SAP_NATIVE_H


#include "sapvm.h"


// ������� ������ ���� ��������� ����� (����������� ������)
sap_array* sapvm_new_byte_array(sap_int size);

// ������� ������ short-�� ��������� ����� (����������� ������)
sap_array* sapvm_new_short_array(sap_int size);

// ������� ������ Object-�� ���������� ���� � ����� (����������� ������)
sap_array* sapvm_new_Object_array(sap_ubyte cls, sap_int size);

// ������� ������ String
sap_object* sapvm_new_String(sap_vm *vm, sap_array *stringData);

// ������� ������ StringBuffer
sap_object* sapvm_new_StringBuffer(sap_vm *vm, sap_int initial_size);


// ������� �������� �������
sap_byte sapvm_call_native(sap_vm *vm, sap_uint Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);

// ��� ������ �������:
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
