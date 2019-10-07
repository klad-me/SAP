#ifndef SAPVM_VM_H
#define SAPVM_VM_H


#include "sapvm.h"


// Сколько команд должна выполнять функция sapvm_execute_thread
#define STEP_N_COMMANDS		128


// Уменьшить ref_count на объекте, и если надо - удалить его
void sapvm_dec_ref(sap_vm *vm, sap_object *obj);

// Увеличить ref_count на объекте
void sapvm_inc_ref(sap_vm *vm, sap_object *obj);


// Выполнить нить
sap_byte sapvm_execute_thread(sap_vm *vm, sap_thread *thr);


#endif
