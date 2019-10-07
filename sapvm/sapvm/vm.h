#ifndef SAPVM_VM_H
#define SAPVM_VM_H


#include "sapvm.h"


// ������� ������ ������ ��������� ������� sapvm_execute_thread
#define STEP_N_COMMANDS		128


// ��������� ref_count �� �������, � ���� ���� - ������� ���
void sapvm_dec_ref(sap_vm *vm, sap_object *obj);

// ��������� ref_count �� �������
void sapvm_inc_ref(sap_vm *vm, sap_object *obj);


// ��������� ����
sap_byte sapvm_execute_thread(sap_vm *vm, sap_thread *thr);


#endif
