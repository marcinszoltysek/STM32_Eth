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
#include "dhcp.h"
#include "dns.h"


#define DHCP_SOCKET     0
#define DNS_SOCKET      1
#define HTTP_SOCKET     2

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

volatile bool ip_assigned = false;

void Callback_IPAssigned(void) {
    ip_assigned = true;
}

void Callback_IPConflict(void) {
}

// 1K should be enough, see https://forum.wiznet.io/t/topic/1612/2
uint8_t dhcp_buffer[1024];
// 1K seems to be enough for this buffer as well
uint8_t dns_buffer[1024];

void initEth() {

    reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
    reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
    reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);

    uint8_t rx_tx_buff_sizes[] = {2, 2, 2, 2, 2, 2, 2, 2};
    wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);

    wiz_NetInfo net_info = {
        .mac  = { 0xEA, 0x11, 0x22, 0x33, 0x44, 0xEA },
        .dhcp = NETINFO_DHCP
    };
    // set MAC address before using DHCP
    setSHAR(net_info.mac);
    DHCP_init(DHCP_SOCKET, dhcp_buffer);

    reg_dhcp_cbfunc(
        Callback_IPAssigned,
        Callback_IPAssigned,
        Callback_IPConflict
    );

    // actually should be called in a loop, e.g. by timer
    uint32_t ctr = 10000;
    while((!ip_assigned) && (ctr > 0)) {
        DHCP_run();
        ctr--;
    }
    if(!ip_assigned) {
        return;
    }

    getIPfromDHCP(net_info.ip);
    getGWfromDHCP(net_info.gw);
    getSNfromDHCP(net_info.sn);

    uint8_t dns[4];
    getDNSfromDHCP(dns);


    wizchip_setnetinfo(&net_info);
    DNS_init(DNS_SOCKET, dns_buffer);

    uint8_t addr[4];
    {
        char domain_name[] = "eax.me";
        int8_t res = DNS_run(dns, (uint8_t*)&domain_name, addr);
        if(res != 1) {
            return;
        }
    }

    uint8_t http_socket = HTTP_SOCKET;
    uint8_t code = socket(http_socket, Sn_MR_TCP, 10888, 0);
    if(code != http_socket) {
        return;
    }

    code = connect(http_socket, addr, 80);
    if(code != SOCK_OK) {
        close(http_socket);
        return;
    }


    {
        char req[] = "GET / HTTP/1.0\r\nHost: eax.me\r\n\r\n";
        uint16_t len = sizeof(req) - 1;
        uint8_t* buff = (uint8_t*)&req;
        while(len > 0) {
;
            int32_t nbytes = send(http_socket, buff, len);
            if(nbytes <= 0) {
                close(http_socket);
                return;
            }

            len -= nbytes;
        }
    }


    {
        char buff[32];
        for(;;) {
            int32_t nbytes = recv(http_socket, (uint8_t*)&buff, sizeof(buff)-1);
            if(nbytes == SOCKERR_SOCKSTATUS) {
                break;
            }

            if(nbytes <= 0) {
                break;
            }

            buff[nbytes] = '\0';
        }
    }

    close(http_socket);
}

void loop() {
    HAL_Delay(1000);
}
