/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup blinky_example_main main.c
 * @{
 * @ingroup blinky_example
 * @brief Blinky Example Application main file.
 *
 * This file contains the source code for a sample application to blink LEDs.
 *
 */

#include <stdbool.h>
#include "nrf_delay.h"
#include "boards.h"
#include "app_uart.h"
#include "app_error.h"
#include "nrf_uart.h"
#include <stdio.h>
#include "log.h"


#define UART_TX_BUFFER    128
#define UART_RX_BUFFER    128

#define TX_PIN_NUMBER     NRF_GPIO_PIN_MAP(0,13)
#define RX_PIN_NUMBER     NRF_GPIO_PIN_MAP(0,15)

static const app_uart_flow_control_t UART_HWFC = APP_UART_FLOW_CONTROL_DISABLED;

void error_uart_handler(app_uart_evt_t *p) 
{

}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code = NRF_SUCCESS;
    
    /* Configure board LES for debug */
    bsp_board_init(BSP_INIT_LEDS);
    bsp_board_led_on(2);

    app_uart_comm_params_t com_params = {0};
    com_params.rx_pin_no = RX_PIN_NUMBER;
    com_params.tx_pin_no = TX_PIN_NUMBER;
    com_params.flow_control = APP_UART_FLOW_CONTROL_DISABLED;
    com_params.use_parity = false;
    com_params.baud_rate = NRF_UART_BAUDRATE_115200;

    /* Init UART fifo */
    APP_UART_FIFO_INIT(&com_params, 
      UART_RX_BUFFER, 
      UART_TX_BUFFER, 
      error_uart_handler, 
      APP_IRQ_PRIORITY_LOWEST, 
      err_code);
    APP_ERROR_CHECK(err_code);

    UART_LOG("hello world from nrf %02X\n", 0xC);

    while(true) 
    {
        uint8_t cr;
        while(app_uart_get(&cr) != NRF_SUCCESS);

        if( cr == 'n' ) {
            bsp_board_leds_on();
            UART_LOG("Turned on leds\n");    
        }

        if( cr == 'f' ) {
            bsp_board_leds_off();
            UART_LOG("Turned off leds\n");    
        }
    }

}

/**
 *@}
 **/
