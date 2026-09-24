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
| Sensing | Cảm biến độ ẩm đất (Soil Moisture Sensor), Cảm biến nhiệt độ & độ ẩm không khí (DHT11/DHT22).| Thu thập liên tục các đại lượng vật lý môi trường khu vườn ban công và mức nước bình chứa.|
| Edge | Vi điều khiển ESP32, Module Rơ-le điều khiển máy bơm mini, Module điều khiển động cơ (Servo/Stepper) kéo màn chắn nắng, Mạch nguồn hạ áp DC. | Đọc cảm biến, thực thi thuật toán tự động tưới và kéo màn chắn cục bộ tại biên, xử lý truyền thông đồng bộ với Blynk Cloud. |
| Network | Mạng Wi-Fi 2.4 GHz gia đình (802.11 b/g/n), Giao thức truyền thông Blynk Protocol (WebSockets / TCP) và HTTP/HTTPS. | Cầu nối truyền tải hai chiều dữ liệu cảm biến và lệnh điều khiển giữa vi điều khiển ESP32 và Blynk Cloud với độ trễ thấp. |
| Backend | Nền tảng Blynk Cloud (Blynk IoT Platform). | Quản lý xác thực thiết bị, điều phối dữ liệu, xử lý sự kiện cảnh báo và gửi thông báo đẩy về smartphone. |
| Database | Cơ sở dữ liệu đám mây Blynk Cloud. | Lưu trữ dữ liệu lịch sử các Virtual Pins (nhiệt độ, độ ẩm đất, mức nước), nhật ký kích hoạt bơm và trạng thái màn chắn. |
| Application | Ứng dụng Blynk Mobile Dashboard (trên iOS/Android) và Blynk Web Console. | Cung cấp giao diện trực quan cho nhân viên văn phòng giám sát biểu đồ môi trường thời gian thực, xem cảnh báo và gửi lệnh điều khiển từ xa. |
