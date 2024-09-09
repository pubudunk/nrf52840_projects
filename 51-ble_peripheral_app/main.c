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

#include "ble_advdata.h"
#include "ble_advertising.h"

#include "ble_conn_params.h"

#define APP_BLE_CONN_CFG_TAG      1
#define APP_BLE_OBSERVER_PRIO     3

#define DEVICE_NAME               "NRF52_BLE_APP"

#define MIN_CONN_INTERVAL         MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define MAX_CONN_INTERNAL         MSEC_TO_UNITS(200, UNIT_1_25_MS)
#define SLAVE_LATENCY             0
#define CONN_SUPERVISION_TIMEOUT  MSEC_TO_UNITS(2000, UNIT_10_MS)

#define APP_ADV_INTERVAL          300
#define APP_ADV_DURATION          0   /* Continous Advertising */

#define FIRST_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(5000)
#define NEXT_CONN_PARAMS_UPDATE_DELAY     APP_TIMER_TICKS(30000)
#define MAX_CONN_PARAMS_UPDATE_COUNT      3

#define CHECK_BLE_ADV_ADDR_TIME_INTERVAL  APP_TIMER_TICKS(9000)  /* 9 seconds */   

NRF_BLE_QWR_DEF(m_qwr);   /* Use NRF_BLE_QWRS_DEF if multiple connections are used */
NRF_BLE_GATT_DEF(m_gatt);
BLE_ADVERTISING_DEF(m_advertising);

APP_TIMER_DEF(m_check_ble_id); /* To check BLE address periodically for non-resolvable private addr */

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;


static void check_ble_id_timeout_handler(void *p_context);

/* Step 10.1: Connection parameter event handler */
static void on_conn_params_evt(ble_conn_params_evt_t *p_evt)
{
  ret_code_t err_code = NRF_SUCCESS;

  if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
  {
    err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
    APP_ERROR_CHECK(err_code);
  }

  if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_SUCCEEDED)
  {
    NRF_LOG_INFO("Con params updated...");
  }
}

/* Step 10.2: Connection parameter error handler */
static void conn_params_error_handler(uint32_t nrf_error)
{
  APP_ERROR_HANDLER(nrf_error);
}

/* Step 10: setting connection params */
static void init_conn_params(void)
{
  ret_code_t err_code = NRF_SUCCESS;

  ble_conn_params_init_t cp_init = {0};

  cp_init.p_conn_params = NULL;
  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;

  cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;

  cp_init.disconnect_on_fail = false; /* Not to disconnect if the parameter update failes */

  cp_init.error_handler = conn_params_error_handler;
  cp_init.evt_handler = on_conn_params_evt;

  err_code = ble_conn_params_init(&cp_init);
  APP_ERROR_CHECK(err_code);
}

/* Step 9.1: queue writer error handler */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
  APP_ERROR_HANDLER(nrf_error);
}

/* Step 9: Init Services */
static void init_services(void)
{
  ret_code_t err_code = NRF_SUCCESS;

  nrf_ble_qwr_init_t qwr_init = {0};

  qwr_init.error_handler = nrf_qwr_error_handler;

  err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
  APP_ERROR_CHECK(err_code);
}

/* Step 8.1: Advertising event handler */
static void on_adv_event(ble_adv_evt_t ble_adv_evt)
{
  ret_code_t err_code = NRF_SUCCESS;

  switch(ble_adv_evt) 
  {
    case BLE_ADV_EVT_FAST:
      NRF_LOG_INFO("Fast advertising...");
      err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
      APP_ERROR_CHECK(err_code);
    break;
    case BLE_ADV_EVT_IDLE:
      NRF_LOG_INFO("Advertising event Idle...");
      err_code = bsp_indication_set(BSP_INDICATE_IDLE);
      APP_ERROR_CHECK(err_code);
    break;
    default:
    break;
   }
}

/* Step 8: Init Advertising */
static void init_advertising(void)
{
  ret_code_t err_code = NRF_SUCCESS;

  ble_advertising_init_t init = {0};
  int8_t tx_power_lvl = 0;

  /* Device Name */
  init.advdata.name_type =  BLE_ADVDATA_FULL_NAME;
  /* Device Appearance */
  init.advdata.include_appearance = true; /* We are including appearance and be default it will set to unknown unless we set to a type when setting GAP param */
  /* Flags */
  init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
  /* TX power level */
  init.advdata.p_tx_power_level = &tx_power_lvl; /* This only adds the power level in the adv packet. Not setting the transmitter */

  init.config.ble_adv_fast_enabled = true;
  init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
  init.config.ble_adv_fast_timeout = APP_ADV_DURATION;

  init.evt_handler = on_adv_event;

  err_code = ble_advertising_init(&m_advertising, &init);
  APP_ERROR_CHECK(err_code);

  ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

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

  /* Set device appearance */
  err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_CYCLING);
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

  /* Step 12.4: create timer for ble_id reading */
  err_code = app_timer_create(&m_check_ble_id, APP_TIMER_MODE_REPEATED, check_ble_id_timeout_handler);
  APP_ERROR_CHECK(err_code);
}

/* Step 1: Initialize the logger */
static void log_init(void)
{
  ret_code_t ret_err = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(ret_err);

  NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/* Step 11: Start advertisements */
static void start_advertisments(void)
{
  ret_code_t  ret_code = NRF_SUCCESS;

  /* make sure to use the same mode of advertisement used in advertisement init */
  ret_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
  APP_ERROR_CHECK(ret_code);
}


/* Step 12.1: Set random  static address */
static void set_random_static_addr(void)
{
  ret_code_t err_code = NRF_SUCCESS;

  ble_gap_addr_t ble_addr = {0};

  ble_addr.addr[0] = 0x56;
  ble_addr.addr[1] = 0xCE;
  ble_addr.addr[2] = 0x88;
  ble_addr.addr[3] = 0x99;
  ble_addr.addr[4] = 0x01;
  ble_addr.addr[5] = 0xFF; 

  ble_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;

  err_code = sd_ble_gap_addr_set(&ble_addr);
  if( err_code != NRF_SUCCESS )
  {
    NRF_LOG_INFO("Setting random static addr failed. Error code: %X", err_code);
  }
}

/* Step 12.3: Set non-resolvabile private address */
static void set_non_resolvable_pvt_addr(void)
{
  ret_code_t err_code = 0;

  ble_gap_privacy_params_t ble_addr = {0};

  ble_addr.privacy_mode = BLE_GAP_PRIVACY_MODE_DEVICE_PRIVACY;
  ble_addr.private_addr_type = BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE;
  ble_addr.private_addr_cycle_s = 15; /* 15 seconds */
  ble_addr.p_device_irk = NULL;

  err_code = sd_ble_gap_privacy_set(&ble_addr);
  if( err_code != NRF_SUCCESS )
  {
    NRF_LOG_INFO("Setting random static addr failed. Error code: %X", err_code);
  }
}

/* Step 12.2: Get the device advertising address */
static void get_device_adv_addr(void)
{
  ret_code_t err_code = 0;

  ble_gap_addr_t ble_addr = {0};

  err_code = sd_ble_gap_addr_get(&ble_addr);
  
  if( err_code == NRF_SUCCESS )
  {
    NRF_LOG_INFO("Address Type: %02X", ble_addr.addr_type);
    NRF_LOG_INFO("Device Addr: %02X:%02X:%02X:%02X:%02X:%02X",
                        ble_addr.addr[5], ble_addr.addr[4], ble_addr.addr[3], 
                        ble_addr.addr[2], ble_addr.addr[1], ble_addr.addr[0]);
  }
}

/* Step 12.4: Read ble id in timeout handler */
static void check_ble_id_timeout_handler(void *p_context)
{
  ret_code_t err_code = NRF_SUCCESS;

  ble_gap_addr_t ble_addr = {0};

  err_code = sd_ble_gap_adv_addr_get(m_advertising.adv_handle, &ble_addr);
  if( err_code == NRF_SUCCESS )
  {
    NRF_LOG_INFO("Address Type: %02X", ble_addr.addr_type);
    NRF_LOG_INFO("Device Addr: %02X:%02X:%02X:%02X:%02X:%02X",
                        ble_addr.addr[5], ble_addr.addr[4], ble_addr.addr[3], 
                        ble_addr.addr[2], ble_addr.addr[1], ble_addr.addr[0]);
  }
}


/**@brief Function for application main entry.
 */
int main(void)
{
  ret_code_t ret_code = NRF_SUCCESS;

  log_init();
  timer_init();
  init_leds();

  init_power_management();
  
  /* Init Soft device and parameters */
  init_ble_stack();
  init_gap_params();
  init_gatt();
  init_advertising();
  init_services();
  init_conn_params();

  NRF_LOG_INFO("BLE Base Application started...");

  /* Set device address */
  set_random_static_addr();
  //set_non_resolvable_pvt_addr();

  start_advertisments();

  /* check device addr */
  get_device_adv_addr();
  //ret_code = app_timer_start(m_check_ble_id, CHECK_BLE_ADV_ADDR_TIME_INTERVAL, NULL);
  //APP_ERROR_CHECK(ret_code);

  // Enter main loop.
  for (;;)
  {
    idle_state_handler();
  }
}


/**
 * @}
 */
