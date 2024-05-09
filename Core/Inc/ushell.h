#ifndef _USHELL_H
#define _USHELL_H

#include "stdio.h"
#include "stdint.h"



#define USHELL_COM_POOL_SIZE 50
#define USHELL_BUF_SIZE 50

typedef void (*ushell_command_func)(void* data);

typedef enum{
    USHELL_VALUE_INT32,
    USHELL_VALUE_FLOAT,
	USHELL_VALUE_RAW,
}ushell_value;

typedef struct{
    ushell_value type;
    int32_t max;
    int32_t min;
}ushell_param;


typedef struct{
    char* name;
    char* alias;
    ushell_param value;
    ushell_command_func func;
}ushell_command;


typedef enum{
    USHELL_ANS_WRONG_COMMAND,
    USHELL_ANS_WRONG_PARAM,
    USHELL_ANS_SUCCESS,
}ushell_status;


void ushell_init();
int ushell_add_command(ushell_command* com);

int ushell_parse_bytes(char *str, uint8_t *bytes,uint8_t len);
int ushell_parse_byte(char *str, uint8_t* out);
int ushell_parse_uint32(char *str, uint32_t* out,uint32_t* offset);

void ushell_write(uint8_t* data,uint8_t size);


int parse_command();
void ushell_load_buf(char* str);

#endif
