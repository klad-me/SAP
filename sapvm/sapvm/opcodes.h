#ifndef SAPVM_OPCODES_H
#define SAPVM_OPCODES_H


// Имена команд
extern const const char *op_name[];



// Другие команды
#define	OP_NOP			0xC0


// Операции над стеком
#define	OP_B_PUSH		0x00
#define	OP_S_PUSH		0x40
#define	OP_IF_PUSH		0x80
#define	OP_I_PUSH_0		0xC6

#define	OP_B_POP		0x01
#define	OP_S_POP		0x41
#define	OP_IF_POP		0x81
#define	OP_R_POP		0xC1

#define	OP_B_DUP		0x02
#define	OP_S_DUP		0x42
#define	OP_IF_DUP		0x82
#define	OP_R_DUP		0xC2

#define	OP_SWAP			0x44
#define	OP_SWAP2		0x85


// Преобразования базовых типов
#define	OP_B2BOOL		0x03

#define	OP_B2S			0x04
#define	OP_B2I			0x05
#define	OP_B2F			0x06

#define	OP_S2B			0x43
#define	OP_S2I			0x45
#define	OP_S2F			0x46

#define	OP_I2B			0x83
#define	OP_I2S			0x84
#define	OP_I2F			0x86

#define	OP_F2B			0xC3
#define	OP_F2S			0xC4
#define	OP_F2I			0xC5


// Работа с локальными переменными
#define	OP_B_LOAD		0x07
#define	OP_S_LOAD		0x47
#define	OP_IF_LOAD		0x87
#define	OP_R_LOAD		0xC7

#define	OP_B_STORE		0x08
#define	OP_S_STORE		0x48
#define	OP_IF_STORE		0x88
#define	OP_R_STORE		0xC8


// Работа с полями
#define	OP_B_LOAD_STATIC	0x09
#define	OP_S_LOAD_STATIC	0x49
#define	OP_IF_LOAD_STATIC	0x89
#define	OP_R_LOAD_STATIC	0xC9

#define	OP_B_STORE_STATIC	0x0A
#define	OP_S_STORE_STATIC	0x4A
#define	OP_IF_STORE_STATIC	0x8A
#define	OP_R_STORE_STATIC	0xCA

#define	OP_B_LOAD_FIELD		0x0B
#define	OP_S_LOAD_FIELD		0x4B
#define	OP_IF_LOAD_FIELD	0x8B
#define	OP_R_LOAD_FIELD		0xCB

#define	OP_B_STORE_FIELD	0x0C
#define	OP_S_STORE_FIELD	0x4C
#define	OP_IF_STORE_FIELD	0x8C
#define	OP_R_STORE_FIELD	0xCC


// Работа с массивами
#define	OP_B_LOAD_ARRAY		0x0D
#define	OP_S_LOAD_ARRAY		0x4D
#define	OP_IF_LOAD_ARRAY	0x8D
#define	OP_R_LOAD_ARRAY		0xCD

#define	OP_B_STORE_ARRAY	0x0E
#define	OP_S_STORE_ARRAY	0x4E
#define	OP_IF_STORE_ARRAY	0x8E
#define	OP_R_STORE_ARRAY	0xCE

#define	OP_ARRAY_LENGTH		0x20

#define	OP_FILL_ARRAY_RAW	0xA2


// Математика
#define	OP_B_ADD		0x0F
#define	OP_S_ADD		0x4F
#define	OP_I_ADD		0x8F
#define	OP_F_ADD		0xCF

#define	OP_B_MUL		0x10
#define	OP_S_MUL		0x50
#define	OP_I_MUL		0x90
#define	OP_F_MUL		0xD0

#define	OP_B_DIV		0x11
#define	OP_S_DIV		0x51
#define	OP_I_DIV		0x91
#define	OP_F_DIV		0xD1

#define	OP_B_REM		0x12
#define	OP_S_REM		0x52
#define	OP_I_REM		0x92
#define	OP_F_REM		0xD2

#define	OP_B_UMINUS		0x13
#define	OP_S_UMINUS		0x53
#define	OP_I_UMINUS		0x93
#define	OP_F_UMINUS		0xD3

#define	OP_B_INC		0x14
#define	OP_S_INC		0x54
#define	OP_I_INC		0x94
#define	OP_F_INC		0xD4

#define	OP_B_DEC		0x15
#define	OP_S_DEC		0x55
#define	OP_I_DEC		0x95
#define	OP_F_DEC		0xD5


// Битовые операции
#define	OP_B_NEG		0x16
#define	OP_S_NEG		0x56
#define	OP_I_NEG		0x96

#define	OP_B_AND		0x17
#define	OP_S_AND		0x57
#define	OP_I_AND		0x97

#define	OP_B_OR			0x18
#define	OP_S_OR			0x58
#define	OP_I_OR			0x98

#define	OP_B_XOR		0x19
#define	OP_S_XOR		0x59
#define	OP_I_XOR		0x99

#define	OP_B_SHL		0x1A
#define	OP_S_SHL		0x5A
#define	OP_I_SHL		0x9A

#define	OP_B_SHR		0x1B
#define	OP_S_SHR		0x5B
#define	OP_I_SHR		0x9B

#define	OP_B_SHRU		0x1C
#define	OP_S_SHRU		0x5C
#define	OP_I_SHRU		0x9C


// Логические операции
#define	OP_B_LT			0x1D
#define	OP_S_LT			0x5D
#define	OP_I_LT			0x9D
#define	OP_F_LT			0xDD

#define	OP_B_GT			0x1E
#define	OP_S_GT			0x5E
#define	OP_I_GT			0x9E
#define	OP_F_GT			0xDE

#define	OP_B_EQ			0x1F
#define	OP_S_EQ			0x5F
#define	OP_I_EQ			0x9F
#define	OP_F_EQ			0xDF
#define	OP_R_EQ			0xD6

#define	OP_LOGIC_NOT		0xD7


// Операции перехода
#define	OP_JZ			0xD8
#define	OP_JNZ			0xD9
#define OP_JNULL		0xE2
#define	OP_GOTO			0xDA
#define	OP_CASE			0xDB
#define	OP_RETURN		0xDC


// Операции вызова методов
#define	OP_CALL_VIRTUAL		0x60
#define	OP_CALL_INTERFACE	0xA0
#define	OP_CALL_METHOD		0xE0


// Операции работы с объектами
#define	OP_NEW_ARRAY		0x21
#define	OP_NEW_INSTANCE		0x61
#define	OP_CAST			0xA1
#define	OP_INSTANCEOF		0xE1
#define	OP_MONITOR_ENTER	0x22
#define	OP_MONITOR_EXIT		0x62


#endif
