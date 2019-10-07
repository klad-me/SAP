#ifndef SAPVM_GC_H
#define SAPVM_GC_H


#include "sapvm.h"


void sapvm_gc(sap_vm *vm, sap_object *obj2gc);
sap_object** sapvm_gc_delete(sap_vm *vm, sap_object **obj_AT);


#endif
