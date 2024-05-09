/*
 * can.c
 *
 *  Created on: 5 мая 2024 г.
 *      Author: ramaf
 */
#include "can.h"
#include "stm32f1xx_hal.h"
#include "ushell.h"

static CAN_HandleTypeDef hcan;
static CAN_TxHeaderTypeDef TxHeader;
static uint32_t TxMailbox = 0;
static uint8_t TxData[8] = { 0 };

CAN_RxHeaderTypeDef RxHeader;
uint8_t RxData[10] = { 0 };

static int can_write_callback(void *arg);
static ushell_command can_command = { .alias = "canwrite", .func = &can_write_callback, .name = "can", .value = USHELL_VALUE_RAW };

void can_init(void) {

	//1000k

	hcan.Instance = CAN1;
	// hcan.Init.Prescaler = 90;

	hcan.Init.Mode = CAN_MODE_NORMAL;

	/* 100kbit
	 hcan.Init.Prescaler = 20;
	 hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
	 hcan.Init.TimeSeg1 = CAN_BS1_12TQ;
	 hcan.Init.TimeSeg2 = CAN_BS2_5TQ;
	 */


	//250kbit
	hcan.Init.Prescaler = 16;
	hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
	hcan.Init.TimeSeg1 = CAN_BS1_6TQ;
	hcan.Init.TimeSeg2 = CAN_BS2_2TQ;

	hcan.Init.TimeTriggeredMode = DISABLE;
	hcan.Init.AutoBusOff = ENABLE;
	hcan.Init.AutoWakeUp = DISABLE;
	hcan.Init.AutoRetransmission = DISABLE;
	hcan.Init.ReceiveFifoLocked = DISABLE;
	hcan.Init.TransmitFifoPriority = ENABLE;
	if (HAL_CAN_Init(&hcan) != HAL_OK) {
		Error_Handler();
	}

	CAN_FilterTypeDef canFilterConfig;
	canFilterConfig.FilterBank = 0;
	canFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
	canFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	canFilterConfig.FilterIdHigh = CAN_ID << 5;
	canFilterConfig.FilterIdLow = 0x0000;
	canFilterConfig.FilterMaskIdHigh = CAN_ID << 5;
	canFilterConfig.FilterMaskIdLow = 0x0000;
	canFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
	canFilterConfig.FilterActivation = ENABLE;

	if (HAL_CAN_ConfigFilter(&hcan, &canFilterConfig) != HAL_OK) {
		Error_Handler();
	}

	HAL_StatusTypeDef status = HAL_CAN_Start(&hcan);

	HAL_CAN_ActivateNotification(&hcan,
	CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_LAST_ERROR_CODE);

	RxData[0] = 0xFF;
	RxData[1] = 0xFA;

	ushell_add_command(&can_command);

}

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	if (hcan->Instance == CAN1) {
		/* USER CODE BEGIN CAN1_MspInit 0 */

		/* USER CODE END CAN1_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_CAN1_CLK_ENABLE();

		__HAL_RCC_GPIOA_CLK_ENABLE();
		/*CAN GPIO Configuration
		 PA11     ------> CAN_RX
		 PA12     ------> CAN_TX
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_11;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_12;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
		HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
	}

}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, &RxData[2]) == HAL_OK) {
		ushell_write(RxData, 10);
		// can_test();
	}
}

static int can_write(uint32_t id, uint8_t *data, uint32_t len) {

	TxHeader.StdId = id;
	TxHeader.ExtId = 0;
	TxHeader.RTR = CAN_RTR_DATA; //CAN_RTR_REMOTE
	TxHeader.IDE = CAN_ID_STD;   // CAN_ID_EXT
	TxHeader.DLC = len;
	TxHeader.TransmitGlobalTime = 0;

	HAL_CAN_AddTxMessage(&hcan, &TxHeader, data, &TxMailbox);
}

static int can_write_callback(void *arg) {
	uint8_t data[8];
	uint32_t id = 0;
	uint32_t pos = 0;
	if (!ushell_parse_uint32(arg, &id, &pos))
		return -1;
	arg += pos;
	if (!ushell_parse_bytes(arg, data, 8))
		return -1;

	can_write(id, data, 8);

}

void can_test() {
	TxHeader.StdId = 121;
	TxHeader.ExtId = 0;
	TxHeader.RTR = CAN_RTR_DATA; //CAN_RTR_REMOTE
	TxHeader.IDE = CAN_ID_STD;   // CAN_ID_EXT
	TxHeader.DLC = 8;
	TxHeader.TransmitGlobalTime = 0;

	for (uint8_t i = 0; i < 8; i++) {
		TxData[i] = (i + 10);
	}
	HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);

}

void USB_LP_CAN1_RX0_IRQHandler(void) {
	HAL_CAN_IRQHandler(&hcan);
}

void CAN1_RX1_IRQHandler(void) {
	HAL_CAN_IRQHandler(&hcan);
}

void USB_HP_CAN1_TX_IRQHandler(void) {
	HAL_CAN_IRQHandler(&hcan);
}

void CAN1_SCE_IRQHandler(void) {
	HAL_CAN_IRQHandler(&hcan);
}

