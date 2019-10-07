#include "opcodes.h"

#include "sapvm_opts.h"


#ifdef SAPVM_DEEP_DEBUG

#define null	(const char*)0
    
    // Имена команд
const const char *op_name[]=
    {
	// 0x
	"b_push",		"b_pop",		"b_dup",		"b2bool",
	"b2s",			"b2i",			"b2f",			"b_load",
	"b_store",		"b_load_static",	"b_store_static",	"b_load_field",
	"b_store_field",	"b_load_array",		"b_store_array",	"b_add",
	
	// 1x
	"b_mul",		"b_div",		"b_rem",		"b_uminus",
	"b_inc",		"b_dec",		"b_neg",		"b_and",
	"b_or",			"b_xor",		"b_shl",		"b_shr",
	"b_shru",		"b_lt",			"b_gt",			"b_eq",
	
	// 2x
	"array_length",		"new_array",		"monitor_enter",	null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	
	// 3x
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	
	// 4x
	"s_push",		"s_pop",		"s_dup",		"s2b",
	"swap",			"s2i",			"s2f",			"s_load",
	"s_store",		"s_load_static",	"s_store_static",	"s_load_field",
	"s_store_field",	"s_load_array",		"s_store_array",	"s_add",
	
	// 5x
	"s_mul",		"s_div",		"s_rem",		"s_uminus",
	"s_inc",		"s_dec",		"s_neg",		"s_and",
	"s_or",			"s_xor",		"s_shl",		"s_shr",
	"s_shru",		"s_lt",			"s_gt",			"s_eq",
	
	// 6x
	"call_virtual",		"new_instance",		"monitor_exit",		null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	
	// 7x
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	
	// 8x
	"if_push",		"if_pop",		"if_dup",		"i2b",
	"i2s",			"swap2",		"i2f",			"if_load",
	"if_store",		"if_load_static",	"if_store_static",	"if_load_field",
	"if_store_field",	"if_load_array",	"if_store_array",	"i_add",
	
	// 9x
	"i_mul",		"i_div",		"i_rem",		"i_uminus",
	"i_inc",		"i_dec",		"i_neg",		"i_and",
	"i_or",			"i_xor",		"i_shl",		"i_shr",
	"i_shru",		"i_lt",			"i_gt",			"i_eq",
	
	// Ax
	"call_interface",	"cast",			"fill_array_raw",	null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	
	// Bx
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	
	// Cx
	"nop",			"r_pop",		"r_dup",		"f2b",
	"f2s",			"f2i",			"i_push_0",		"r_load",
	"r_store",		"r_load_static",	"r_store_static",	"r_load_field",
	"r_store_field",	"r_load_array",		"r_store_array",	"f_add",
	
	// Dx
	"f_mul",		"f_div",		"f_rem",		"f_uminus",
	"f_inc",		"f_dec",		"r_eq",			"logic_not",
	"jz",			"jnz",			"goto",			"case",
	"return",		"f_lt",			"f_gt",			"f_eq",
	
	// Ex
	"call_method",		"instanceof",		"jnull",		null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	
	// Fx
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
	null,			null,			null,			null,
    };

#endif
