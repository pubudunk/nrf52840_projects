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

#include "nrf_ble_gatt.h"

#define APP_BLE_CONN_CFG_TAG      1
#define APP_BLE_OBSERVER_PRIO     3

#define DEVICE_NAME               "NRF52_BLE_APP"

#define MIN_CONN_INTERVAL         MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define MAX_CONN_INTERNAL         MSEC_TO_UNITS(200, UNIT_1_25_MS)
#define SLAVE_LATENCY             0
#define CONN_SUPERVISION_TIMEOUT  MSEC_TO_UNITS(2000, UNIT_10_MS)


NRF_BLE_QWR_DEF(m_qwr);   /* Use NRF_BLE_QWRS_DEF if multiple connections are used */
NRF_BLE_GATT_DEF(m_gatt);

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;


/* Step 7: Init GATT */
static void init_gatt(void)
{
  ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
  APP_ERROR_CHECK(err_code);
}

/* Step 6: Init GAP */
static void init_gap_params(void)
{
  ret_code_t err_code = NRF_SUCCESS;

  ble_gap_conn_params_t     gap_conn_params = {0};
  ble_gap_conn_sec_mode_t   sec_mode = {0};

  /* Setting basic (no) security */
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
  APP_ERROR_CHECK(err_code);

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERNAL;
  gap_conn_params.slave_latency = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout = CONN_SUPERVISION_TIMEOUT;

  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  APP_ERROR_CHECK(err_code);
}


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

  init_power_management();
  
  /* Init Soft device and parameters */
  init_ble_stack();
  init_gap_params();
  init_gatt();

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
