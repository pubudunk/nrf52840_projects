#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"

#include "app_timer.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "nrf_ble_qwr.h"

#define APP_BLE_CONN_CFG_TAG    1
#define APP_BLE_OBSERVER_PRIO   3


NRF_BLE_QWR_DEF(m_qwr);   /* Use NRF_BLE_QWRS_DEF if multiple connections are used */


static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;


/* Step 5.1: BLE Event handler */
static void ble_event_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
  ret_code_t err_code = NRF_SUCCESS;

  switch(p_ble_evt->header.evt_id) {
    
    case BLE_GAP_EVT_DISCONNECTED:
      NRF_LOG_INFO("Device disconnected");
      break;
    case BLE_GAP_EVT_CONNECTED:
      NRF_LOG_INFO("Device Connected");
      
      err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
      APP_ERROR_CHECK(err_code);

      m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
      APP_ERROR_CHECK(err_code);


      break;
    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
      NRF_LOG_INFO("Phy update request");

      ble_gap_phys_t const phys = 
      {
        .rx_phys = BLE_GAP_PHY_AUTO,
        .tx_phys = BLE_GAP_PHY_AUTO,
      };

      err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);   
      APP_ERROR_CHECK(err_code);

     break;
    default:
     break;
  }
}

/* Step 5: Init BLE Stack (Soft Device) */
static void init_ble_stack(void)
{
  ret_code_t err_code = NRF_SUCCESS;
  
  err_code = nrf_sdh_enable_request();
  APP_ERROR_CHECK(err_code);

  uint32_t ram_start = 0;
  err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
  APP_ERROR_CHECK(err_code);

  err_code = nrf_sdh_ble_enable(&ram_start);
  APP_ERROR_CHECK(err_code);

  NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_event_handler, NULL);
}

/* Step 4.1: Initialize Power Managment */
static void idle_state_handler(void)
{
  if( NRF_LOG_PROCESS() == false ) {
    nrf_pwr_mgmt_run();
  }
}

/* Step 4: Initialize Power Managment */
static void init_power_management(void)
{
  ret_code_t err_code = nrf_pwr_mgmt_init();
  APP_ERROR_CHECK(err_code);
}

/* Step 3: Initialize LEDs */
static void init_leds(void)
{
  ret_code_t err_code = bsp_init(BSP_INIT_LEDS, NULL);
  APP_ERROR_CHECK(err_code);
}

/* Step 2: Initialize App Timer */
static void timer_init(void) 
{
  ret_code_t  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
}

/* Step 1: Initialize the logger */
static void log_init(void)
{
  ret_code_t ret_err = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(ret_err);

  NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for application main entry.
 */
int main(void)
{
  log_init();
  timer_init();
  init_leds();

  // Enter main loop.
  for (;;)
  {
    NRF_LOG_DEBUG("this is a debug from nrf");
    NRF_LOG_INFO("this is a info from nrf");
    idle_state_handler();
    
    //nrf_delay_ms(2000);
  }
}


/**
 * @}
 */
