#include "ushell.h"
#include "string.h"
#include <stdlib.h>
#include "errno.h"
#include "stm32f1xx_ll_usart.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_ll_bus.h"

static ushell_command* comm_pool[USHELL_COM_POOL_SIZE];
static uint8_t comm_cnt=0;
static uint8_t rx_buffer[USHELL_BUF_SIZE];
static uint8_t rx_buf_count=0;
static uint8_t tx_buf_count=0;
static uint8_t tx_buffer[USHELL_BUF_SIZE];

static int parse_alias(char* cur_pos_ptr,uint8_t* next_pos);
static int parse_param_float(char* cur_str_pos, uint32_t comm_pool_pos, float* output_value);
static int parse_param_int(char* cur_str_pos, uint32_t comm_pool_pos,int32_t* output_value);


static char * status_str[]={
    "Wrong command!",
    "Wrong parameter!",
    "Right command!",
    "Right paramter!"
};

void ushell_init(){
	 LL_USART_InitTypeDef USART_InitStruct = {0};

	  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	  /* Peripheral clock enable */
	  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

	  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);
	  /**USART1 GPIO Configuration
	  PA9   ------> USART1_TX
	  PA10   ------> USART1_RX
	  */
	  GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
	   GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	   GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	   GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	   LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	   GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
	   GPIO_InitStruct.Mode = LL_GPIO_MODE_FLOATING;
	   LL_GPIO_Init(GPIOA, &GPIO_InitStruct);


	  NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
	  NVIC_EnableIRQ(USART1_IRQn);

	  USART_InitStruct.BaudRate =  460800;
	  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
	  LL_USART_EnableIT_RXNE(USART1);
	  LL_USART_Init(USART1, &USART_InitStruct);
	  LL_USART_ConfigAsyncMode(USART1);
	  LL_USART_Enable(USART1);

};

void ushell_write(uint8_t* data,uint8_t size){
	for(uint8_t i=0;i<size;i++){
		LL_USART_TransmitData8(USART1, data[i]);
		while(!LL_USART_IsActiveFlag_TXE(USART1));
	}
}



/*for debug*/
void ushell_load_buf(char* str){
    strcpy(&rx_buffer[0],str);
}

int ushell_add_command(ushell_command* comm){
    if(comm_cnt<=USHELL_COM_POOL_SIZE){
        comm_pool[comm_cnt]=comm;
        comm_cnt++;
        return 0;
    }
    else return -1;
}

int parse_command(){

    char* begin=&(rx_buffer[0]);

    uint8_t pos=0;

    int32_t comm_pool_pos=parse_alias(begin,&pos);
    if(comm_pool_pos==-1 || strlen(begin)<=2) {
    	return -1;
    }
    printf("success!\n");
    begin+=pos;
    if(*begin!=' ') return -1;
    begin++;
    if(!begin) return;

    if(comm_pool[comm_pool_pos]->value.type==USHELL_VALUE_RAW){
    	comm_pool[comm_pool_pos]->func(begin);
    }
    return 0;
}

static int parse_alias(char* cur_pos_ptr,uint8_t* next_pos){
    int i=0;
    int j=0;

    uint8_t cur_str_len=strlen(cur_pos_ptr);
    if(cur_str_len==0) return -1;
    for(i=0;i<comm_cnt;i++){
        uint8_t alias_len=strlen(comm_pool[i]->alias);
        if(cur_str_len<alias_len) continue;
        for(j=0;j<alias_len; j++){
            if(cur_pos_ptr[j]!=comm_pool[i]->alias[j]) break;
        }
        if(j==alias_len){
            *next_pos=j;
            return i;
        }
    }
    return -1;
}

static int parse_param_float(char* cur_str_pos, uint32_t comm_pool_pos, float* output_value){
    uint8_t param_len=strlen(cur_str_pos);
    if(param_len==0) return -1;
    float ans=strtof(cur_str_pos,NULL);
    if(ans<0.01) return -1;
    else{
        *output_value=ans;
        return 0;
        }
  }

static int parse_param_int(char* cur_str_pos, uint32_t comm_pool_pos,int32_t* output_value){
    uint8_t param_len=strlen(cur_str_pos);
    if(param_len==0) return -1;
    int32_t ans=atoi(cur_str_pos);
    if(ans!=0){
        *output_value=ans;
        return 0;
    }
    else return -1;
}

int ushell_parse_bytes(char *str, uint8_t *bytes,uint8_t len) {

	char *divider = NULL;
	uint8_t cur_byte;
	for (uint8_t i = 0; i < len; i++) {
		divider = strchr(str, ' ');
		if (!divider && !str) return -1;
		else {
			if(divider) *divider=0;
			if(ushell_parse_byte(str,&cur_byte)!=-1) {
				bytes[i]=cur_byte;
			}
			else return -1;

			if(divider){
				*divider=' ';
				str=divider+1;
			}
			else return 1;

		}
	}
	return 1;

}

int ushell_parse_byte(char *str, uint8_t* out) {
	uint32_t val = 0;

	if(*str!='0') return -1;
	str++;
	if(*str!='x') return -1;
	str++;
	while (*str) {
		// get current character then increment
		uint8_t byte = *str++;
		// transform hex character to the 4bit equivalent number, using the ascii table indexes
		if (byte >= '0' && byte <= '9')
			byte = byte - '0';
		else if (byte >= 'a' && byte <= 'f')
			byte = byte - 'a' + 10;
		else if (byte >= 'A' && byte <= 'F')
			byte = byte - 'A' + 10;
		// shift 4 to make space for new digit, and add the 4 bits of the new digit
		val = (val << 4) | (byte & 0xF);
	}
	*out=val;
	return 1;
}

 int ushell_parse_uint32(char *str, uint32_t* out,uint32_t* offset) {
	uint32_t val = 0;
	uint32_t pos=0;

	if(!str) return -1;
	if(*str!='0') return -1;
	str++;
	if(*str!='x') return -1;
	str++;
	pos+=2;
	while (*str!=0 && *str!=' ') {
		// get current character then increment
		uint8_t byte = *str++;
		// transform hex character to the 4bit equivalent number, using the ascii table indexes
		if (byte >= '0' && byte <= '9')
			byte = byte - '0';
		else if (byte >= 'a' && byte <= 'f')
			byte = byte - 'a' + 10;
		else if (byte >= 'A' && byte <= 'F')
			byte = byte - 'A' + 10;
		else return -1;
		// shift 4 to make space for new digit, and add the 4 bits of the new digit
		val = (val << 4) | (byte & 0xF);
		pos++;
	}
	*out=val;
	*offset=pos+1;
	return 1;
}





void USART1_IRQHandler(){
	if(LL_USART_IsActiveFlag_RXNE(USART1)){
		uint8_t data=LL_USART_ReceiveData8(USART1);
		if(data=='\r' || rx_buf_count==USHELL_BUF_SIZE-1) {
			NVIC_DisableIRQ(USART1_IRQn);
			rx_buffer[rx_buf_count]=0;
			parse_command();
			memset(&rx_buffer[0],0,USHELL_BUF_SIZE);
			rx_buf_count=0;
		    NVIC_EnableIRQ(USART1_IRQn);
		}
		else {
			rx_buffer[rx_buf_count]=data;
			rx_buf_count++;
		}
		LL_USART_ClearFlag_RXNE(USART1);
}
}

