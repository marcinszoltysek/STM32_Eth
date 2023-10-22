/*
 * init_Eth.c
 *
 *  Created on: Oct 20, 2023
 *      Author: marci
 */


#include "main.h"
#include "stm32f4xx_hal.h"
#include "spi.h"

#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include "socket.h"




void W5500_Select(void) {
    HAL_GPIO_WritePin(Eth_CS_GPIO_Port, Eth_CS_Pin, GPIO_PIN_RESET);
}

void W5500_Unselect(void) {
    HAL_GPIO_WritePin(Eth_CS_GPIO_Port, Eth_CS_Pin, GPIO_PIN_SET);
}

void W5500_ReadBuff(uint8_t* buff, uint16_t len) {
    HAL_SPI_Receive(&hspi1, buff, len, HAL_MAX_DELAY);
}

void W5500_WriteBuff(uint8_t* buff, uint16_t len) {
    HAL_SPI_Transmit(&hspi1, buff, len, HAL_MAX_DELAY);
}

uint8_t W5500_ReadByte(void) {
    uint8_t byte;
    W5500_ReadBuff(&byte, sizeof(byte));
    return byte;
}

void W5500_WriteByte(uint8_t byte) {
    W5500_WriteBuff(&byte, sizeof(byte));
}

wiz_NetInfo net_info = {
		.ip = {192,168,0,95},
		.mac = {0xEA, 0x11, 0x22, 0x33, 0x44, 0xEA },
		.sn = {255,255,255,0},
		.gw = {192,168,0,1},
		.dns = {8,8,8,8},
		.dhcp = NETINFO_STATIC
};

uint8_t mySocket;
uint8_t serverStatus;
uint8_t state = 0;
uint8_t rxBuff[100];
uint8_t txBuff[100];
int32_t dataSize = 0;
uint16_t port;

void IP_set() {
    uint8_t rx_tx_buff_sizes[] = {2, 2, 2, 2, 2, 2, 2, 2}; //2 bytes TX RX in any buffer

    reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);  //CS Pin UP function
    reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte); //Read 8bit function
    reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff); //Read register function
    wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);




    wizchip_setnetinfo(&net_info); //Send information to module W5500
}





void socket_init () {
	mySocket = socket(1, Sn_MR_TCP, 502, 0);
	if(mySocket != 1){	// Error in socket creation
	  	while(1);
	}
	listen(mySocket);
}


char echo_server (){
	serverStatus = getSn_SR(mySocket);

 	switch(state){

  	case 0: // Wait for client connections
  		if(serverStatus == SOCK_ESTABLISHED){
  			ctlnetwork(CN_GET_NETINFO, (void *) &net_info);
  			port = getSn_PORT(mySocket);
  			state++;
  		}
  		break;


  	case 1: // Client has been connected
  		if(serverStatus == SOCK_CLOSE_WAIT){ // Check for status
  			close(mySocket);
  			state++;
  		}

  		uint8_t intSocketRegister = getSn_IR(mySocket); // Check for incoming data
  		if(intSocketRegister & (1<<2)){ // Check for socket 1 RECV interrupt
  			dataSize = recv(mySocket, rxBuff, 50);
  			recive_from_mysocket(rxBuff);

  			if(send(mySocket,  (uint8_t *)"Dzieki za dane" , 14) != 14){
  			  break;
  			}

  			// Clear interrupt register
  			intSocketRegister &= (1<<2);
  			setSn_IR(mySocket, intSocketRegister);
  		}
  		break;


  	case 2:
  		socket_init ();
  		break;
  	}
}

recive_from_mysocket(uint8_t  rxBuff[]) {
	char diodeon[] = "Wlacz diode byku";
	char diodeoff[] = "Wylacz diode byku";

	  if (strcmp((char*)rxBuff, diodeon) == 0) {
	        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	    }

	    if (strcmp((char*)rxBuff, diodeoff) == 0) {
	        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
	    }
	}





