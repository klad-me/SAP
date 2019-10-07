#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"


// ������� �������� ������� Random.*
sap_byte sapvm_call_native_Random(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0xd3e8a3e8)
    {
	// native static int Random.rand();
	
	// return rand();
	W_INT(f->stack + f->SP, sapvm_rand());
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    
    // ��� �����������
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
