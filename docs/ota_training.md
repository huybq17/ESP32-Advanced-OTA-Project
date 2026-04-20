# KIẾN THỨC CHUYÊN SÂU VỀ OTA TRÊN ESP32

## 1. Bản chất của OTA trên ESP32 là gì?

OTA không phải là việc ghi đè trực tiếp lên đoạn code đang chạy. Thay vào đó, ESP32 sử dụng cơ chế **A/B Updates** thông qua việc chia bộ nhớ Flash thành nhiều phân vùng (Partitions).

Một Partition Table chuẩn cho OTA thường bao gồm:

* **Factory App**: Chứa firmware gốc ban đầu (tùy chọn).
* **OTA_0 (App Slot A)**: Phân vùng để chứa firmware.
* **OTA_1 (App Slot B)**: Phân vùng chứa firmware dự phòng hoặc bản cập nhật mới.
* **OTA Data**: Kích thước rất nhỏ (2 sectors - 8KB), đóng vai trò là "con trỏ" báo cho Bootloader biết lần khởi động tiếp theo phải chạy ở phân vùng nào (OTA_0 hay OTA_1).

## 2. Quá trình ESP32 cập nhật Firmware

Giả sử ESP32 đang chạy firmware ở phân vùng `OTA_0`:

1. **Kết nối & Tải về:** ESP32 kết nối với server để tải firmware mới.
2. **Ghi vào vùng chờ:** Firmware mới sẽ được ghi dần vào phân vùng đang nghỉ ngơi, tức là `OTA_1`.
3. **Xác thực:** Kiểm tra tính toàn vẹn của image (Magic byte, SHA256 checksum, chữ ký số).
4. **Cập nhật OTA Data:** Nhờ hàm `esp_ota_set_boot_partition()`, OTA Data được cập nhật để trỏ sang `OTA_1`.
5. **Reboot:** Khởi động lại và bootloader bắt đầu chạy firmware mới ở `OTA_1`.

## 3. An toàn ứng dụng (App Rollback & Anti-Rollback)

* **App Rollback:** Nếu firmware mới bị lỗi, bootloader tự động "quay xe" (Rollback) về lại firmware cũ đang hoạt động tốt. Ứng dụng khi mới boot phải chạy hàm tự chẩn đoán và gọi `esp_ota_mark_app_valid_cancel_rollback()` nếu chạy đúng.
* **Anti-Rollback:** Ngăn cản hacker bắt thiết bị tải bản firmware cũ hơn để khai thác lỗ hổng bằng cách so sánh `secure_version` lưu trong eFuse của chip.

## 4. Bảo mật trong OTA (Security)

* **HTTPS (TLS/SSL):** Bắt buộc, mã hóa đường truyền.
* **Server Certificate Validation & Pinning:** Xác thực chứng chỉ của server.
* **Firmware Signing (Secure Boot):** Chống nạp firmware giả mạo bằng chữ ký điện tử.

---

# LỘ TRÌNH 6 BƯỚC TRIỂN KHAI DỰ ÁN OTA

### Bước 1: Setup cơ sở hạ tầng (Partition & Wi-Fi)

Cấu hình bảng phân vùng chứa OTA và cho phép ESP32 kết nối Wi-Fi.

### Bước 2: Setup Server lưu trữ Firmware

Tạo Local Server hoặc dùng Cloud Storage để lưu trữ file `.bin`.

local server link in my laptop: http://192.168.24.102:8000/update.bin

### Bước 3: Viết logic Cập nhật OTA Cơ bản (Sử dụng `esp_https_ota`)

Tải file firmware từ Server về ghi vào Flash và chuyển đổi khởi động.

### Bước 4: Áp dụng HTTPS và Xác thực Server (Server Certificate Validation)

Cấp chứng chỉ và xác thực kết nối.

### Bước 5: Kích hoạt tính năng App Rollback Protection

Bảo vệ thiết bị không bị brick khi update lỗi bằng cách reset về firmware trước đó.

### Bước 6: (Tùy chọn Nâng cao) Secure Boot và Firmware Signing

Bảo vệ phần cứng, yêu cầu firmware phải có chữ ký đi kèm mới được khởi động chân chính.



# BỔ SUNG

* **Nguồn chính:** Espressif đã định nghĩa sẵn các layout partition trong ESP‑IDF. Bạn có thể chọn trong `menuconfig → Partition Table`:

  * **Single app** (chỉ có factory app).
  * **Factory app, two OTA definitions** (có `factory`, `ota_0`, `ota_1`, `otadata`).
  * **Custom partition table (CSV)** (tự viết file như bạn đưa).
* **Kích thước** : phụ thuộc dung lượng flash của module (thường 4 MB cho ESP32‑WROOM). Espressif khuyến nghị mỗi OTA slot ~1 MB để đủ chứa firmware.
* RAM:
* **SRAM tổng:** khoảng **520 KB** (chia thành nhiều vùng).

  * **SRAM0 (Data RAM):** ~320 KB, dùng cho dữ liệu, stack, heap.
  * **SRAM1 (Instruction RAM):** ~192 KB, dùng cho code và dữ liệu.
  * **RTC FAST/SLOW RAM:** ~16 KB, dùng cho dữ liệu giữ lại khi deep sleep.
* **PSRAM (tùy board):** DevKit v1 chuẩn thường  **không có PSRAM** . Một số module ESP32‑WROVER mới hơn có thêm 4 MB hoặc 8 MB PSRAM.
