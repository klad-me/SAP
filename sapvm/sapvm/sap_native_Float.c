#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <ctype.h>
#include <string.h>
#include <math.h>


#define NaN	(0.0f / 0.0f)

static float sapvm_parseFloat(const char *s, int l)
{
    float sign=1.0;
    float v=0.0, div=10.0;
    int e=0, e_sign=1;
    
    // �������� ����
    if (l < 1) return NaN;
    if ( (*s)=='-' )
    {
	sign=-1.0;
	s++;
	l--;
    } else
    if ( (*s)=='+' )
    {
	s++;
	l--;
    }
    
    // ��������, ��� ����� ����� ���� ����� ��� �����
    if (l < 1) return NaN;
    if ( (!isdigit((sap_ubyte)(*s))) && ((*s)!='.') ) return NaN;
    
    // ������ ����� �����
    while ( (l>0) && (isdigit((sap_ubyte)(*s))) )
    {
	v=v * 10.0 + ((*s++)-'0');
	l--;
    }
    
    // ������ ������� �����
    if ( (l>0) && ((*s)=='.') )
    {
	s++;
	l--;
	while ( (l>0) && (isdigit((sap_ubyte)(*s))) )
	{
	    v=v + ((float)((*s++)-'0')) / div;
	    l--;
	    div=div * 10.0;
	}
    }
    
    // �������� �����
    v=v * sign;
    
    // �������� �� ����� ������
    if (l==0) return v;
    
    // ���� �� ����� ������ - ������ ������ ���� ���� ����������
    if ( ((*s)!='e') && ((*s)!='E') ) return NaN;
    s++;
    l--;
    
    // ������ ����� ���� ���� ����������
    if ( (l>0) && ((*s)=='-') )
    {
	e_sign=-1;
	s++;
	l--;
    } else
    if ( (l>0) && ((*s)=='+') )
    {
	s++;
	l--;
    }
    
    // ������ ������ ���� ����������
    if (l < 1) return NaN;
    if (!isdigit((sap_ubyte)(*s))) return NaN;
    
    // ������ ����������
    while ( (l>0) && (isdigit((sap_ubyte)(*s))) )
    {
	e=e * 10 + ((*s)-'0');
	s++;
	l--;
    }
    
    // �������� ����������
    e=e * e_sign;
    
    // �������� �� ����� ������
    if (l!=0) return NaN;
    
    // �������� ����� � �����������
    if (e >= 0)
    {
	while (e-- > 0) v=v * 10.0;
    } else
    {
	while (e++ < 0) v=v / 10.0;
    }
    
    // ���������� �����
    return v;
}


static sap_array* sapvm_floatToString(float f)
{
    // max float: '340282346638528859811704183484516925440.000000' (length=46)
    char buf[50], *ss=buf;
    
    if (isnan(f))
    {
	strcpy(buf, "nan");
    } else
    if (isinf(f))
    {
	if (isinf(f)==1)
	    strcpy(buf, "fni"); else
	    strcpy(buf, "-fni");
    } else
    {
	// ������� �����
	sap_float v1;
	sap_int v2, i;
	
	if (f < 0.0)
	{
	    (*ss++)='-';
	    f=-f;
	}
	
	// ����� �� ����� � ������� �����
	v1=floorf(f);
	v2=(sap_int)( (f-v1)*1e6 );	// 6 ������ ����� �������
	
	// �������� ������� ������� ����� (�.�. � ��� � ������ ��� ����� �����-�������)
	for (i=0; i<6; i++)
	{
	    (*ss++)='0'+(v2 % 10);
	    v2=v2/10;
	}
	(*ss++)='.';
	
	// �������� ����� ����� (���� � �������� �������)
	do
	{
	    (*ss++)=((int)fmodf(v1, 10.0)) + '0';
	    v1=floor(v1 / 10.0);
	} while (v1 > 0.1);
	
	// ����� ������
	(*ss)=0;
    }
    
    // ������� ������
    int l=strlen(buf), p=l-1;
    sap_array *str=sapvm_new_byte_array(l);
    if (str)
    {
	ss=(char*)str->data.b;
	if (buf[0]=='-')
	{
	    (*ss++)='-';
	    l--;
	}
	while (l--) (*ss++)=buf[p--];
    }
    
    return str;
}


// ������� �������� ������� Float.*
sap_byte sapvm_call_native_Float(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0x1ea0e4d1)
    {
	// native static float Float.__parseFloat(byte[] str);
	sap_array *str=(sap_array*)R_INT(params+0);
	
	if (!str) return SAP_FAULT_NULL_REF;
	if (str->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	float v=sapvm_parseFloat((char*)str->data.b, str->length);
	
	// �������� ������
	sapvm_dec_ref(vm, (sap_object*)str);
	
	// return v;
	W_FLOAT(f->stack + f->SP, v);
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    if (Native_Hash==0xb3b79622)
    {
	// native static byte[] Float.__toString(float v);
	sap_float v=R_FLOAT(params+0);
	
	// �������� ������
	sap_array *str=sapvm_floatToString(v);
	
	// return str;
	W_REF(f->stack + f->SP, str);
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    if (Native_Hash==0x9dc2aaee)
    {
	// native static float Float.intBitsToFloat(int value);
	sap_int v=R_INT(params+0);
	sap_float *vf=(sap_float*)&v;
	
	// return vf;
	W_FLOAT(f->stack + f->SP, (*vf));
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    if (Native_Hash==0x8e722256)
    {
	// native static int Float.floatToIntBits(float value);
	sap_float v=R_FLOAT(params+0);
	sap_int *vi=(sap_int*)&v;
	
	// return vi;
	W_INT(f->stack + f->SP, (*vi));
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    
    // ��� �����������
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
