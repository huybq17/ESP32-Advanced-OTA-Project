#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#define OTA_URL "http://192.168.102.250:8000/ota_server/ota_v1.bin"

// Khởi tạo OTA Manager (Lắng nghe sự kiện hệ thống)
void ota_manager_init(void);

#endif // OTA_MANAGER_H
