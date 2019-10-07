#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <stdio.h>
#include <string.h>


// ������� �������� ������� IPC.*
sap_byte sapvm_call_native_IPC(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0x06ecd5d7)
    {
	// native static byte IPC.tryLock(Object o);
	sap_object *o=R_REF(params+0);
	if (!o) return SAP_FAULT_NULL_REF;
	
        // ��������� - ������������ �� ������
	sap_byte ok=1;
        sap_lock *l=vm->locks;
        while (l)
        {
            if (l->obj == o)
            {
                // ������ ������������
		
                // �������� - ����� ������
                if (l->thr != thr)
                {
                    // �� ����� ������ - �������� �����
		    ok=0;
                } else
                {
                    // ���������� ����� ������. ������ ��������� ������� ����������
                    l->count++;
                }
                break;
            }
            l=l->next;
        }
	
        if ( (ok) && (!l) )
        {
            // ������ ����� �� ������������. ���� ������� ����� ����������
            l=(sap_lock*)ALLOC(sizeof(sap_lock), SAP_HEAP_VM);
            if (!l)
            {
                // �� ���������� �������� ����������
                vm->fault_cause=SAP_FAULT_NO_MEMORY;
                return SAP_FAULT;
            }
            l->obj=o;
            l->thr=thr;
            l->count=1;
            l->next=vm->locks;
            vm->locks=l;
        }
	
	// �������� ������
	sapvm_dec_ref(vm, o);
	
	// return ok;
	W_INT(f->stack + f->SP, ok);
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    if (Native_Hash==0x5b0bbae9)
    {
        // native static void IPC.lock(Object o);
	sap_object *o=R_REF(params+0);
	if (!o) return SAP_FAULT_NULL_REF;
	
        // ��������� - ������������ �� ������
        sap_lock *l=vm->locks;
        while (l)
        {
            if (l->obj == o)
            {
                // ������ ������������
		
                // �������� - ����� ������
                if (l->thr != thr)
                {
                    // �� ����� ������ - �������� �����
                    thr->unlock_obj=o;
                } else
                {
                    // ���������� ����� ������. ������ ��������� ������� ����������
                    l->count++;
                }
                break;
            }
            l=l->next;
        }
	
        if ( (!thr->unlock_obj) && (!l) )
        {
            // ������ ����� �� ������������. ���� ������� ����� ����������
            l=(sap_lock*)ALLOC(sizeof(sap_lock), SAP_HEAP_VM);
            if (!l)
            {
                // �� ���������� �������� ����������
                vm->fault_cause=SAP_FAULT_NO_MEMORY;
                return SAP_FAULT;
            }
            l->obj=o;
            l->thr=thr;
            l->count=1;
            l->next=vm->locks;
            vm->locks=l;
        }
	
	// �������� ������
	sapvm_dec_ref(vm, o);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// ��� ������
	return thr->unlock_obj ? -1 : 0;	// ������ ������������ ����� ��� ����������
    } else
    if (Native_Hash==0x46180de7)
    {
	// native static void IPC.unlock(Object o);
	sap_object *o=R_REF(params+0);
	if (!o) return SAP_FAULT_NULL_REF;
	
        // ������� ����������
	sap_int rv=0;
        sap_lock *prev=0, *l=vm->locks;
        while (l)
        {
            if (l->obj == o)
            {
                // ����� ��������������� ������
                l->count--;
                if (l->count <= 0)
                {
                    // ���� ������� ����������
		    
                    // ��������� - ���� �����-�� ����� ����� ���� ������ - ��������� �� � ��������� ������ �� ����� �����, ����� ������� ��������
                    sap_thread *t=vm->threads;
                    while (t)
                    {
                        if (t->unlock_obj == o)
                        {
                            t->unlock_obj=0;
                            l->thr=t;
                            l->count=1;
                            rv=-1;       // ������� �������������� ������������ �����
                            break;
                        }
                        t=t->next;
                    }
		    
                    if (l->count <= 0)
                    {
                        // ��� �� ���� ������� ����������
                        if (prev)
                            prev->next=l->next; else
                	    vm->locks=l->next;
                        FREE((void*)l);
                    }
                }
                break;
            }
            prev=l;
            l=l->next;
        }
	
	// �������� ������
	sapvm_dec_ref(vm, o);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// ��� ������
	return rv;
    } else
    
    // ��� �����������
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
