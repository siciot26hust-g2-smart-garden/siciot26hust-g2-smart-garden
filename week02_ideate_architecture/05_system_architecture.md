# 5. System Architecture

## Kiến trúc end-to-end
```text
Physical World
      ↓
Sensor
      ↓
Edge Device
      ↓
Network
      ↓
Backend / IoT Platform
      ↓
Database
      ↓
Application / Dashboard
      ↓
User
```

## Nếu có điều khiển
```text
User / Rule
    ↓
Command
    ↓
Network
    ↓
Edge
    ↓
Actuator
    ↓
Physical World
```

## Thành phần
| Layer | Thành phần | Vai trò |
|---|---|---|
| **Sensing** | Cảm biến độ ẩm đất, cảm biến nhiệt độ | Đo lường độ ẩm của đất và nhiệt độ môi trường giàn cây ban công |
| **Edge Device (Đầu cuối)** | Vi điều khiển ESP32, bơm nước mini, động cơ Servo | Thu thập dữ liệu cảm biến, nhận lệnh điều khiển từ Gateway để bật/tắt bơm và quay góc Servo đóng/mở rèm |
| **Network** | Router Wi-Fi gia đình, giao thức MQTT (Publish/Subscribe) | Cầu nối truyền tải dữ liệu hai chiều giữa ESP32 và Raspberry Pi |
| **Processing Center (Trung tâm xử lý)** | Raspberry Pi 4 tích hợp: MQTT Broker (Mosquitto), Engine xử lý Node-RED / Python, Web Server | Điều phối bản tin MQTT, thực thi thuật logic (<30% bơm, >32°C mở rèm), phục vụ Web Server HTTP/WebSocket |
| **Database** | PostgreSQL / InfluxDB (chạy trên Raspberry Pi 4) | Lưu trữ lịch sử dữ liệu đo lường và nhật ký điều khiển |
| **Application (Client)** | Giao diện Web (PC / Điện thoại) kết nối qua HTTP / WebSocket | Hiển thị đồ thị thời gian thực và cung cấp nút điều khiển: Tưới nước, Đóng/Mở rèm |