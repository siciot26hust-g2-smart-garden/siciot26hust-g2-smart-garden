# TÀI LIỆU WORKFLOW HỆ THỐNG SMART GARDEN (DUAL-BRAIN)

Hệ thống được thiết kế theo cấu trúc **"Hai bộ não" (Dual-Brain)** hoạt động song song để đảm bảo tính ổn định tối đa.

## 1. Bộ Não Cơ Bắp (ESP32 - Phần Cứng & An Toàn)
ESP32 chịu trách nhiệm giao tiếp trực tiếp với linh kiện vật lý và hoạt động như một hệ thống an toàn cấp thấp (Fail-safe).

* **Đọc cảm biến:** Định kỳ đọc nhiệt độ, độ ẩm (DHT11), mực nước (HC-SR04) và độ ẩm đất.
* **Gửi dữ liệu (Telemetry):** Đóng gói toàn bộ dữ liệu thành chuỗi JSON và bắn lên MQTT (topic garden/telemetry) mỗi 3 giây.
* **Bảo vệ độc lập (Servo):** Bất chấp Node-RED có mạng hay không, ESP32 luôn tự quét nhiệt độ:
  * Nếu Nhiệt độ > 32°C ➔ Từ từ mở rèm (Kéo dài 1.8 giây cho 180 độ).
  * Nếu Nhiệt độ < 30°C ➔ Từ từ đóng rèm.
* **Chế độ Failover:** ESP32 liên tục lắng nghe "Nhịp tim" (Handshake) từ Node-RED. Nếu Node-RED bị sập mạng quá 30 giây, ESP32 tự động chuyển sang chế độ ESP32_FAILOVER và tự mình điều khiển Máy Bơm (Bơm khi đất khô).
* **Bảo vệ Tự động:** Nếu ESP32 đang bật Auto, nó sẽ TỪ CHỐI (Block) mọi lệnh điều khiển thủ công từ ngoài vào để chống xung đột, trừ khi nhận được lệnh uto OFF trước.

## 2. Bộ Não Trung Tâm (Node-RED - Điều Khiển & Giao Diện)
Node-RED đóng vai trò là bảng điều khiển và là trung tâm ra quyết định chính khi hệ thống có mạng.

* **Nhịp tim (Handshake):** Bắn liên tục tín hiệu garden/pi/ready xuống ESP32 để giữ ESP32 ở trạng thái ngoan ngoãn phục tùng (PI_MODE).
* **Bơm Tự Động (Auto Controller):**
  * Nhận Telemetry từ ESP32.
  * Nếu Đất < 30% ➔ Gửi lệnh PUMP ON.
  * Nếu Đất >= 60% ➔ Gửi lệnh PUMP OFF.
* **Cơ chế Smart Override 15s (Ghi đè Thông minh):** 
  * Khi người dùng bấm nút Thủ công trên web (Bơm/Rèm).
  * Node-RED lập tức bắn lệnh 1: uto OFF (Hạ khiên bảo vệ của ESP32).
  * Node-RED bắn lệnh 2: [Lệnh của người dùng].
  * Node-RED hẹn giờ 15 giây sau: Bắn lệnh 3: uto ON (Bật lại chế độ tự động).
  * Giúp người dùng luôn can thiệp được bằng tay mà không sợ quên bật lại Auto.
