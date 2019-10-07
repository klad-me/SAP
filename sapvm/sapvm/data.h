#ifndef SAPVM_DATA_H
#define SAPVM_DATA_H


#define R_BYTE(addr)		(*((sap_byte*)(addr)))
#define W_BYTE(addr, value)	do { (*((sap_byte*)(addr))) = (sap_byte)(value); } while (0)

#define R_SHORT(addr)		(*((sap_short*)(addr)))
#define W_SHORT(addr, value)	do { (*((sap_short*)(addr))) = (sap_short)(value); } while (0)

#define R_INT(addr)		(*((sap_int*)(addr)))
#define W_INT(addr, value)	do { (*((sap_int*)(addr))) = (sap_int)(value); } while (0)

#define R_FLOAT(addr)		(*((sap_float*)(addr)))
#define W_FLOAT(addr, value)	do { (*((sap_float*)(addr))) = (sap_float)(value); } while (0)

#define R_REF(addr)		((sap_object*)((sap_address)(*((sap_int*)(addr)))))
#define W_REF(addr, value)	do { (*((sap_int*)(addr))) = (sap_int)(value); } while (0)


#endif
