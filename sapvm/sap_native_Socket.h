#ifndef SAP_NATIVE_SOCKET_H
#define SAP_NATIVE_SOCKET_H


#include "sapvm/sapvm.h"


sap_byte sapvm_call_native_Socket(sap_vm *vm, sap_int Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);


#endif
