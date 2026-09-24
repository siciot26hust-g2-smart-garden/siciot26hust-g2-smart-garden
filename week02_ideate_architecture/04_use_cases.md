# 4. Use Cases

## UC01: Tự động tưới nước theo độ ẩm đất 
- Actor: Hệ thống (ESP32 & Node-RED trên Raspberry Pi 4).

- Trigger: Cảm biến độ ẩm đất ghi nhận giá trị giảm xuống dưới ngưỡng cài đặt (ngưỡng đất khô).

- Main flow:. 
1. ESP32 đọc dữ liệu từ cảm biến độ ẩm đất định kỳ.
2. ESP32 publish bản tin đo lường qua Router Wi-Fi lên MQTT Broker (Mosquitto) trên Raspberry Pi 4.
3. Engine Node-RED / Python trên Pi 4 nhận dữ liệu, phát hiện độ ẩm đất < ngưỡng cài đặt và kích hoạt luật ra lệnh Bơm.
4. Node-RED lưu bản ghi vào Database và publish bản tin Command qua MQTT xuống ESP32.
5. ESP32 nhận lệnh, kích hoạt Rơ-le bật máy bơm nước mini tưới cây, đồng thời cập nhật trạng thái lên Giao diện Web.
6. Khi độ ẩm đất đạt ngưỡng an toàn hoặc hết thời gian tưới, Node-RED phát lệnh ngắt bơm.

- Expected result:Cây được cấp nước kịp thời, trạng thái hiển thị trực quan trên Giao diện Web.


## UC02: Tự động mở rèm che nắng khi nhiệt độ cao 

- Actor: Hệ thống (ESP32 & Node-RED trên Raspberry Pi 4).
- Trigger: Cảm biến nhiệt độ ghi nhận nhiệt độ ban công > 32°C.
- Main flow:
1. Cảm biến nhiệt độ gửi dữ liệu về ESP32.
2. ESP32 publish dữ liệu nhiệt độ lên MQTT Broker (Mosquitto) trên Pi 4.
3. Node-RED phát hiện nhiệt độ > 32°C, kích hoạt luật ra lệnh Servo mở rèm che nắng.
4. Node-RED lưu lịch sử vào Database và publish bản tin Command qua MQTT xuống ESP32.
5. ESP32 điều khiển góc quay động cơ Servo để mở rèm che phủ giàn cây, đồng thời cập nhật trạng thái lên Giao diện Web.
6. Khi nhiệt độ hạ xuống dưới ngưỡng an toàn, Node-RED ra lệnh Servo quay đóng/thu rèm lại.
- Expected result: Giàn cây ban công được che chắn tự động, tránh cháy lá khi nắng gắt.


## UC03: Giám sát và điều khiển thủ công từ xa qua Giao diện Web 
- Actor: Nhân viên văn phòng (Người dùng).
- Trigger: Người dùng mở Giao diện Web trên PC hoặc Điện thoại.
- Main flow:
1. Người dùng truy cập Giao diện Web (kết nối Web Server trên Raspberry Pi 4 qua HTTP / WebSocket).
2. Web hiển thị đồ thị nhiệt độ, độ ẩm đất thời gian thực trích xuất từ Database.
3. Người dùng bấm nút "Tưới nước" hoặc "Đóng/Mở rèm" trên giao diện.
4. Web Server chuyển tiếp yêu cầu đến Node-RED / Python.
5. Node-RED publish bản tin Command qua MQTT Broker xuống ESP32.
6. ESP32 nhận lệnh, bật/tắt bơm hoặc quay Servo rèm theo yêu cầu, phản hồi trạng thái thực tế lên Web.
- Expected result: Người dùng hoàn toàn chủ động theo dõi và điều khiển vườn cây mọi lúc mọi nơi qua trình duyệt web.






