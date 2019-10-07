#include "sapvm.h"
#include "port.h"
#include "data.h"
#include "vm.h"


static inline sap_ushort CRC16(sap_ushort crc, const sap_ubyte *data, sap_uint size)
{
    sap_uint i;
    sap_ubyte x,cnt;

    for (i=0; i<size; i++)
    {
        x=data[i];
        for (cnt=0; cnt<8; cnt++)
        {
            if ((x^crc)&1) crc=((crc^0x4002)>>1)|0x8000; else
                           crc=crc>>1;
            x>>=1;
        }
    }

    return crc;
}



// Вызвать нативную функцию CRC16.*
sap_byte sapvm_call_native_CRC16(sap_vm *vm, sap_int Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0x439ba103)
    {
	// native static short CRC16.CRC16(short crc, byte[] data, int offs, int size);
	sap_short crc=R_SHORT(params+0);
	sap_array *data=(sap_array*)R_INT(params+2);
	sap_int offs=R_INT(params+6);
	sap_int size=R_INT(params+10);
	
	// Проверяем данные
	if (!data) return SAP_FAULT_NULL_REF;
	if (data->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Проверяем индексы
	if ( (offs < 0) ||
	     (size < 0) ||
	     (offs+size > data->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Считаем CRC
	crc=(sap_short)CRC16((sap_ushort)crc, (const sap_ubyte*)(data->data.b + offs), size);
	
	// Забываем массив
	sapvm_dec_ref(vm, (sap_object*)data);
	
	// return crc;
	W_INT(f->stack + f->SP, crc);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
