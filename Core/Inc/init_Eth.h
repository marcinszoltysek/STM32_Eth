/*
 * init_Eth.h
 *
 *  Created on: Oct 20, 2023
 *      Author: marci
 */

#ifndef INC_INIT_ETH_H_
#define INC_INIT_ETH_H_


void W5500_Select(void);
void W5500_Unselect(void);
void W5500_ReadBuff(uint8_t* buff, uint16_t len);
void W5500_WriteBuff(uint8_t* buff, uint16_t len);
uint8_t W5500_ReadByte(void);

void IP_set();
void socket_init ();
char echo_server ();
recive_from_mysocket(uint8_t rxBuff);

#endif /* INC_INIT_ETH_H_ */
