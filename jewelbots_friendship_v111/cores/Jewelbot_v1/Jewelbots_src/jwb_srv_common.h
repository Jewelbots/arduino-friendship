#ifndef JWB_SRV_COMMON_H_
#define JWB_SRV_COMMON_H_
/*
  Jewelbot BLE Services defines and important pieces.
*/
#define JWB_BLE_UUID_ANNOTATED_ALERT_SERVICE 0x0001
#define JWB_BLE_UUID_ANNOTATED_ALERT_CHAR_TX 0x0002
#define JWB_BLE_UUID_ANNOTATED_ALERT_CHAR_RX 0x0003
#define JWB_BLE_UUID_AAS_BASE                                                  \
  {                                                                            \
    {                                                                          \
      0xA1, 0x54, 0x42, 0xD1, 0x4B, 0x84, 0x53, 0x0A, 0x04, 0x57, 0x1E, 0x19,  \
          0x00, 0x00, 0x40, 0x6E                                               \
    }                                                                          \
  }
// 0xa15442d14b84530a04571e194af176e2
#define JWB_BLE_UUID_JCS_BASE {{0xA1, 0x54, 0x42, 0xD1, 0x4B, 0x84, 0x53, 0x0A, 0x04, 0x57, 0x1E, 0x1A, 0x00, 0x00, 0x40, 0x63}}
#define JWB_BLE_UUID_FRIENDS_LIST_SERVICE 0x0001
#define JWB_BLE_UUID_FRIENDS_LIST_CHAR_TX 0x0002
#define JWB_BLE_UUID_FRIENDS_LIST_CHAR_RX 0x0003
#define JWB_BLE_UUID_DFU_UPDATE_CHAR_RX   0x0004


#endif
