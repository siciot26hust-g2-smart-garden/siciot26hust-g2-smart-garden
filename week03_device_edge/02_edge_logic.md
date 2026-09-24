# 2. Edge & Gateway Logic

## Input

- **Cảm biến độ ẩm đất:** Cung cấp giá trị đo độ ẩm đất liên tục tới ESP32.
- **Cảm biến nhiệt độ:** Cung cấp giá trị đo nhiệt độ môi trường tới ESP32.
- **Lệnh điều khiển thủ công:** Phát sinh từ người dùng thao tác trên Giao diện Web (PC / Điện thoại)

## Processing (Xử lý hệ thống theo kiến trúc sơ đồ)

### Tại Thiết bị đầu cuối (ESP32)

- Đọc tín hiệu từ **Cảm biến độ ẩm đất** và **Cảm biến nhiệt độ** định kỳ.
- Đóng gói dữ liệu thành bản tin JSON đo lường (telemetry).
- Kết nối Wi-Fi tới Router Wi-Fi và gửi bản tin qua giao thức MQTT (Publish) lên Trung tâm xử lý Raspberry Pi 4.
- Nhận bản tin điều khiển qua MQTT (Subscribe) từ Trung tâm xử lý để thực thi:
  - Xuất tín hiệu GPIO 23 Bật/Tắt **Bơm nước mini**.
  - Xuất tín hiệu PWM GPIO 19 điều khiển **Góc quay Động cơ Servo**.

### Tại Trung tâm xử lý (Raspberry Pi 4)

- **MQTT Broker (Mosquitto):**
  - Tiếp nhận các bản tin MQTT Publish từ ESP32.
  - Điều phối truyền tải bản tin điều khiển xuống ESP32.

- **Node-RED / Python:**
  - Tiếp nhận dữ liệu đo lường.
  - Đánh giá luật tự động (**Ẩm < 30%, Nhiệt độ > 32°C**).
  - Lưu lịch sử vào Database.
  - Giao tiếp với Web Server.

- **Database (PostgreSQL / InfluxDB):**
  - Lưu trữ dữ liệu lịch sử đo lường.
  - Lưu trữ nhật ký kích hoạt.

- **Web Server (Giao diện Web):**
  - Phục vụ giao diện giám sát và điều khiển qua HTTP / WebSocket.

### Tại Giao diện Web (PC / Điện thoại)

- Hiển thị đồ thị thời gian thực.
- Cung cấp 2 nút điều khiển:
  - **"Tưới nước"**
  - **"Đóng/Mở rèm"**

## Decision (Quy tắc ra quyết định)

- **Luật 1 – Tự động tưới nước:**
  - NẾU **Độ ẩm đất < 30%** → Ra lệnh **Bật máy bơm**.
  - Ngắt bơm khi đất đạt đủ độ ẩm hoặc hết thời gian timeout.

- **Luật 2 – Tự động che nắng:**
  - NẾU **Nhiệt độ > 32°C** → Ra lệnh **Servo mở rèm**.
  - Thu rèm khi nhiệt độ giảm xuống mức mát.

- **Luật 3 – Điều khiển thủ công:**
  - Phản hồi tức thì theo thao tác nút bấm trên Giao diện Web.

## Output

- **Thiết bị đầu cuối (ESP32):**
  - Xuất tín hiệu Digital **GPIO 23** để Bật/Tắt **Bơm nước mini**.
  - Xuất xung PWM **GPIO 19** để điều khiển **Góc quay Động cơ Servo** đóng/mở rèm.

- **Trung tâm xử lý (Raspberry Pi 4):**
  - Ghi log lịch sử vào Database (**PostgreSQL / InfluxDB**).
  - Cập nhật đồ thị dữ liệu thời gian thực lên Giao diện Web (PC / Điện thoại) qua **HTTP / WebSocket**.

## Pseudocode
```text
read input
validate
process
make decision
publish/store/control
```
