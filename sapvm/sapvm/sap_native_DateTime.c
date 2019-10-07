#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <stdio.h>
#include <string.h>


#define INIT	16
#define ADD	16


// Расположение полей в классе
#define F_YEAR	0	// short
#define F_MONTH	2	// byte
#define F_DAY	3	// byte
#define F_WDAY	4	// byte
#define F_HOUR	5	// byte
#define F_MIN	6	// byte
#define F_SEC	7	// byte


// С какого дня года начинается каждый месяц
static const sap_ushort month_days[13] = { 0,31,59,90,120,151,181,212,243,273,304,334,365 };

// Сколько дней в каждом месяце
static const sap_ubyte days_in_month[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };


// Вызвать нативную функцию DateTime.*
sap_byte sapvm_call_native_DateTime(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0x8f757144)
    {
	// native int to_unixtime();
	sap_short year=R_SHORT(obj->fields + F_YEAR);
	sap_byte month=R_BYTE(obj->fields + F_MONTH);
	sap_byte day=R_BYTE(obj->fields + F_DAY);
	sap_byte hour=R_BYTE(obj->fields + F_HOUR);
	sap_byte min=R_BYTE(obj->fields + F_MIN);
	sap_byte sec=R_BYTE(obj->fields + F_SEC);
	
	if (year < 1970) year=1970;
	if (month < 1) month=1;
	if (month > 12) month=12;
	if (day < 1) day=1;
	if (day > 31) day=31;
	
	year=year-1900;
	sap_ushort days=month_days[month-1];
	
	if ( (!(year%4)) && (month>2) ) days++;
	
	sap_uint ut=(86400L * (
            365L*(sap_uint)(year-80) + (sap_uint)(year-77)/4 +
            (sap_uint)days +
            (sap_uint)(day-1) + 3652L) +
            3600L * (sap_uint)hour +
            60L * (sap_uint)min + (sap_uint)sec);
	
        // return ut;
        W_INT(f->stack + f->SP, ut);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x8bdd22a6)
    {
	// native void from_unixtime(int ut);
	sap_uint ut=(sap_uint)R_INT(params+0);
	
#define monsize(mon) (mon == 25 ? 29 : days_in_month[mon%12])
	sap_ushort year;
	sap_ubyte  mon=0;
	sap_ubyte  mday;
	sap_ubyte  wday;
	sap_ubyte  hour;
	sap_ubyte  min;
	sap_ubyte  sec;
	sap_uint days=ut/86400L;
	sap_ushort fouryears=(sap_ushort)((sap_uint)days/1461L);
	sap_short overdays=(sap_short)((sap_uint)days%1461L);
	
	for (;;)
	{
    	    sap_short day=overdays;
	    
    	    if ( (overdays-=monsize(mon))<0 )
    	    {
        	year =4*fouryears+(mon/12)+1970;
        	mon=mon%12+1;
        	mday  =day+1;
        	wday =(days+4)%7;
        	hour =(sap_uint)(ut%86400L)/3600L;
        	min  =(sap_uint)(ut%3600L)/60L;
        	sec  =ut%60L;
		
                if (wday==0) wday=7;
        	wday--;
        	break;
    	    }
    	    mon++;
	}
	
	
	W_SHORT(obj->fields + F_YEAR, year);
	W_BYTE(obj->fields + F_MONTH, mon);
	W_BYTE(obj->fields + F_DAY, mday);
	W_BYTE(obj->fields + F_WDAY, wday);
	W_BYTE(obj->fields + F_HOUR, hour);
	W_BYTE(obj->fields + F_MIN, min);
	W_BYTE(obj->fields + F_SEC, sec);
	
        // return 0;
        W_INT(f->stack + f->SP, 0);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    if (Native_Hash==0x541ba123)
    {
        // native boolean equals(Object o);
        sap_object *o=R_REF(params+0);
	
	if (!o) return SAP_FAULT_NULL_REF;
	
        sap_byte ok=0;
	
        if (o==obj)
        {
            // Тот же объект
            ok=1;
        } else
        if ( (o) && (sapvm_is_instanceof(vm, o, vm->clsid_DateTime)) )
        {
            // Сверяем поля (все, кроме wday)
            ok=( (R_SHORT(obj->fields + F_YEAR)  == R_SHORT(o->fields + F_YEAR)) &&
        	 (R_BYTE (obj->fields + F_MONTH) == R_BYTE (o->fields + F_MONTH)) &&
        	 (R_BYTE (obj->fields + F_DAY)   == R_BYTE (o->fields + F_DAY)) &&
        	 (R_BYTE (obj->fields + F_HOUR)  == R_BYTE (o->fields + F_HOUR)) &&
        	 (R_BYTE (obj->fields + F_MIN)   == R_BYTE (o->fields + F_MIN)) &&
        	 (R_BYTE (obj->fields + F_SEC)   == R_BYTE (o->fields + F_SEC)) );
        }
	
        // Забываем параметры
        sapvm_dec_ref(vm, o);
	
        // return ok;
        W_INT(f->stack + f->SP, ok);
        f->SP+=4;
	
        // Нет ошибки
        return 0;
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
