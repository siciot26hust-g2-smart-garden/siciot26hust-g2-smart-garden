# Hardware Plan

## BOM
| STT | Thiết bị | Số lượng | Vai trò trong | Ghi chú |
|---:|---|---:|---|---|
| 1 | Vi điều khiển ESP32 | 01 | Thiết bị đầu cuối (End Device): Đọc cảm biến và xuất lệnh điều khiển chấp hành | Tích hợp Wi-Fi 2.4GHz kết nối Router |
| 2 | Cảm biến độ ẩm đất | 01 | INPUT: Đo độ ẩm thực tế của đất trong chậu cây | Đầu ra Analog kết nối chân ADC của ESP32 |
| 3 | Cảm biến nhiệt độ | 01 | INPUT: Đo nhiệt độ môi trường giàn cây ban công | Cảm biến nhiệt độ đo lường nhiệt độ môi trường |
| 4 | Bơm nước mini | 01 | OUTPUT: Động cơ/cơ cấu chấp hành cấp nước tưới cho cây | Điều khiển Bật/Tắt (thông qua mạch Relay cách ly) |
| 5 | Động cơ Servo | 01 | OUTPUT: Động cơ/cơ cấu chấp hành kéo rèm che nắng ban công | Điều khiển góc quay bằng tín hiệu xung PWM |
| 6 | Máy tính nhúng Raspberry Pi 4 | 01 | Trung tâm xử lý (Processing Center): Vận hành Mosquitto Broker, Node-RED/Python, Database, Web Server | Máy chủ Gateway cục bộ |
| 7 | Router Wi-Fi | 01 | Hạ tầng truyền thông mạng nội bộ | Cầu nối truyền thông Wi-Fi/IP giữa ESP32 và Raspberry Pi 4 |
| 8 | Thẻ nhớ MicroSD | 01 | Lưu trữ hệ điều hành và phần mềm trên Raspberry Pi 4 | Class 10 / U3 tốc độ cao |
| 9 | Nguồn cấp | 02 | Cung cấp nguồn độc lập ổn định cho Raspberry Pi 4 và cụm ESP32/cơ cấu chấp hành | Nguồn Type-C 5V/3A cho Pi 4 và Adapter 5V cho mạch ESP32 |
| 10 | Bo test & Dây nối (Jumper Wires) | 01 bộ | Đấu nối và kết nối tín hiệu mạch thử nghiệm | Dây đực-đực, đực-cái, bo mạch breadboard |
## Wiring
- Sensor:
  - Cảm biến độ ẩm đất: VCC -> 3.3V, GND -> GND, chân tín hiệu Analog (AOUT) -> GPIO 34 của ESP32.
  - Cảm biến nhiệt độ: VCC -> 3.3V/5V, GND -> GND, chân tín hiệu DATA -> GPIO 4 của ESP32.
- GPIO:
  - GPIO 34 (Analog Input): Đọc tín hiệu điện áp từ Cảm biến độ ẩm đất.
  - GPIO 4 (Digital Input): Đọc dữ liệu từ Cảm biến nhiệt độ.
  - GPIO 23 (Digital Output): Kích hoạt đóng/ngắt Module Relay để điều khiển Bật/Tắt Bơm nước mini.
  - GPIO 19 (PWM Output): Xuất xung PWM điều khiển góc quay Động cơ Servo đóng/mở rèm che nắng.
- Power:
  - Nguồn vi điều khiển ESP32: Cấp nguồn 5V qua cổng microUSB hoặc chân 5V/VIN, bộ ổn áp trên bo mạch hạ áp xuống 3.3V cấp cho vi điều khiển và các cảm biến.
  - Nguồn cơ cấu chấp hành (Bơm & Servo): Sử dụng nguồn cấp 5V riêng biệt có dòng tải tối thiểu 2A, nối chung cực âm (GND) với ESP32 để tránh hiện tượng sụt áp gây reset vi điều khiển khi động cơ khởi động.
  - Nguồn máy tính nhúng Raspberry Pi 4: Cấp nguồn độc lập chuẩn 5V/3A qua cổng Type-C, đảm bảo trung tâm xử lý Gateway hoạt động ổn định và liên tục.
- Actuator:
  - Bơm nước mini: Điều khiển Bật/Tắt qua tiếp điểm thường mở (NO) của Module Relay kết nối chân kích GPIO 23 của ESP32.
  - Động cơ Servo: Điều khiển Góc quay qua chân tín hiệu PWM kết nối GPIO 19 của ESP32, nhận nguồn 5V và mass GND chung với hệ thống.
- Safety notes:
  - Bố trí tụ lọc nguồn (1000µF trên đường cấp nguồn động cơ) và diode dập xung ngược để triệt tiêu xung nhiễu điện áp do tải cảm ứng sinh ra.
  - Toàn bộ bo mạch ESP32, module relay và các mối nối dây được đặt trong hộp bảo vệ cách điện, chống ẩm và nước mưa hắt ngoài ban công.
  - Cài đặt thời gian ngắt tối đa (timeout) cho bơm nước trong phần mềm để chống tràn nước và chống cháy máy bơm khi cạn nguồn nước.
  - Giới hạn hành trình góc quay của động cơ servo bằng phần mềm để bảo vệ cơ cấu rèm, chống kẹt cơ khí.
