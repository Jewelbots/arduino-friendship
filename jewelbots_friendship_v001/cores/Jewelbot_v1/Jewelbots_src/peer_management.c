#include <stdint.h>
#include "app_error.h"
#include "app_timer.h"
#include "ble_conn_state.h"
#include "ble_db_discovery.h"
#include "fds.h"
#include "friend_data_storage.h"
#include "peer_manager.h"

#include "fsm.h"
#include "jewelbot_service.h"
#include "peer_management.h"

#define SEC_PARAM_MIN_KEY_SIZE    7
#define SEC_PARAM_MAX_KEY_SIZE    16


void peer_management_init(bool erase_bonds, bool app_dfu) {
  uint32_t err_code;
  ble_gap_sec_params_t sec_param;
  
  err_code = pm_init();
  APP_ERROR_CHECK(err_code);
  if (erase_bonds)
  {
			NRF_LOG_DEBUG("calling pm peers delete\r\n");
      err_code = pm_peers_delete();
      APP_ERROR_CHECK(err_code);
  }
  
  memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));
  
  sec_param.bond = 1;  //perform bonding
  sec_param.mitm = 0;  //no MITM protection
  sec_param.lesc = 0;  //no LE secure connections
  sec_param.keypress = 0; //no keypress notifications
  sec_param.io_caps = BLE_GAP_IO_CAPS_NONE;
  sec_param.oob = 0;
  sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
  sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
  sec_param.kdist_own.enc = 1;
  sec_param.kdist_peer.enc = 1;
  sec_param.kdist_peer.id = 1;
  
  err_code = pm_sec_params_set(&sec_param);
  APP_ERROR_CHECK(err_code);
  
  err_code = pm_register(pm_evt_handler);
  APP_ERROR_CHECK(err_code);
  
  //err_code = fds_register(jwb_fds_evt_handler);
  //APP_ERROR_CHECK(err_code);
	
	// Send SC indication if new application was loaded by bootloader. 
  if(app_dfu)pm_local_database_has_changed();
  
}

void pm_evt_handler(pm_evt_t const * p_evt)
{
    ret_code_t err_code;

    switch(p_evt->evt_id)
    {
        case PM_EVT_BONDED_PEER_CONNECTED:
            err_code = pm_peer_rank_highest(p_evt->peer_id);
            if(err_code != NRF_ERROR_BUSY)
            {
                APP_ERROR_CHECK(err_code);
            }
            NRF_LOG_DEBUG("PM_EVT_BONDED_PEER_CONNECTED\r\n");
            break;//PM_EVT_BONDED_PEER_CONNECTED

        case PM_EVT_CONN_SEC_START:
            NRF_LOG_DEBUG("PM_EVT_CONN_SEC_START\r\n");
            break;//PM_EVT_CONN_SEC_START

        case PM_EVT_CONN_SEC_SUCCEEDED:
        {
            NRF_LOG_PRINTF_DEBUG("Link secured. Role: %d. conn_handle: %d, Procedure: %d\r\n",
                           ble_conn_state_role(p_evt->conn_handle),
                           p_evt->conn_handle,
                           p_evt->params.conn_sec_succeeded.procedure);
            err_code = pm_peer_rank_highest(p_evt->peer_id);
            
            if(err_code != NRF_ERROR_BUSY)
            {
                APP_ERROR_CHECK(err_code);
            }
            NRF_LOG_DEBUG("PM_EVT_CONN_SEC_SUCCEEDED\r\n");
            if ((get_current_event()->sig == BOND_WITH_MASTER_DEVICE_SIG) || (get_current_event()->sig == JEWELBOT_CONNECTED_TO_MASTER_SIG)) {
              set_jwb_event_signal(BONDED_TO_MASTER_DEVICE_SIG);
              NRF_LOG_DEBUG("BONDED!\r\n");
            }
        }break;//PM_EVT_CONN_SEC_SUCCEEDED

        case PM_EVT_CONN_SEC_FAILED:
        {
            /** In some cases, when securing fails, it can be restarted directly. Sometimes it can
             *  be restarted, but only after changing some Security Parameters. Sometimes, it cannot
             *  be restarted until the link is disconnected and reconnected. Sometimes it is
             *  impossible, to secure the link, or the peer device does not support it. How to
             *  handle this error is highly application dependent. */
            switch (p_evt->params.conn_sec_failed.error)
            {
                case PM_CONN_SEC_ERROR_PIN_OR_KEY_MISSING:
                    // Rebond if one party has lost its keys.
                    err_code = pm_conn_secure(p_evt->conn_handle, true);
                    if (err_code != NRF_ERROR_INVALID_STATE)
                    {
                        APP_ERROR_CHECK(err_code);
                    }
                    break;//PM_CONN_SEC_ERROR_PIN_OR_KEY_MISSING

                default:
                    break;
            }
            NRF_LOG_DEBUG("PM_EVT_CONN_SEC_FAILED\r\n");
        }break;//PM_EVT_CONN_SEC_FAILED

        case PM_EVT_CONN_SEC_CONFIG_REQ:
        {
            // Reject pairing request from an already bonded peer.
            pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
            pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
            NRF_LOG_DEBUG("PM_EVT_CONN_SEC_CONFIG_REQ\r\n");
        }break;//PM_EVT_CONN_SEC_CONFIG_REQ

        case PM_EVT_STORAGE_FULL:
        {
            // Run garbage collection on the flash.
            err_code = fds_gc();
            if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
            {
                // Retry.
            }
            else
            {
                APP_ERROR_CHECK(err_code);
            }
         NRF_LOG_DEBUG("PM_EVT_STORAGE_FULL\r\n");
        }break;//PM_EVT_STORAGE_FULL

        case PM_EVT_ERROR_UNEXPECTED:
            // Assert.
            NRF_LOG_DEBUG("PM_EVT_ERROR_UNEXPECTED\r\n");
            APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
            break;//PM_EVT_ERROR_UNEXPECTED

        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
            NRF_LOG_DEBUG("PM_EVT_PEER_DATA_UPDATE_SUCCEEDED\r\n");
            break;//PM_EVT_PEER_DATA_UPDATE_SUCCEEDED

        case PM_EVT_PEER_DATA_UPDATE_FAILED:
            // Assert.
            NRF_LOG_DEBUG("PM_EVT_PEER_DATA_UPDATE_FAILED\r\n");
            APP_ERROR_CHECK_BOOL(false);
            break;//PM_EVT_PEER_DATA_UPDATE_FAILED

        case PM_EVT_PEER_DELETE_SUCCEEDED:
            NRF_LOG_DEBUG("PM_EVT_PEER_DELETE_SUCCEEDED\r\n");
            break;//PM_EVT_PEER_DELETE_SUCCEEDED

        case PM_EVT_PEER_DELETE_FAILED:
            // Assert.
            NRF_LOG_DEBUG("PM_EVT_PEER_DELETE_FAILED\r\n");
            APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
            break;//PM_EVT_PEER_DELETE_FAILED

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            NRF_LOG_DEBUG("PM_EVT_PEERS_DELETE_SUCCEEDED\r\n");
            break;//PM_EVT_PEERS_DELETE_SUCCEEDED

        case PM_EVT_PEERS_DELETE_FAILED:
            // Assert.
            NRF_LOG_DEBUG("PM_EVT_PEERS_DELETE_FAILED\r\n");
            APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
            break;//PM_EVT_PEERS_DELETE_FAILED

        case PM_EVT_LOCAL_DB_CACHE_APPLIED:
            NRF_LOG_DEBUG("PM_EVT_LOCAL_DB_CACHE_APPLIED\r\n");
            break;//PM_EVT_LOCAL_DB_CACHE_APPLIED

        case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
            // The local database has likely changed, send service changed indications.
            NRF_LOG_DEBUG("PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED\r\n");
            pm_local_database_has_changed();
            break;//PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED

        case PM_EVT_SERVICE_CHANGED_IND_SENT:
            NRF_LOG_DEBUG("PM_EVT_SERVICE_CHANGED_IND_SENT\r\n");
            break;//PM_EVT_SERVICE_CHANGED_IND_SENT

        case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
            NRF_LOG_DEBUG("PM_EVT_SERVICE_CHANGED_IND_CONFIRMED\r\n");
            break;//PM_EVT_SERVICE_CHANGED_IND_CONFIRMED

        default:
            // No implementation needed.
            break;
    }
}
