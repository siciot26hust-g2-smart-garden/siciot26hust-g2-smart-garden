# Device Test

| Test ID | Nội dung | Expected | Actual | PASS/FAIL |
| :-: | :--- | :--- | :--- | :-: |
| D01 | Sensor read | ESP32 đọc giá trị từ Cảm biến độ ẩm đất và Cảm biến nhiệt độ liên tục, không lỗi kết nối | Đọc ổn định, dữ liệu độ ẩm đất và nhiệt độ hiển thị chính xác | PASS |
| D02 | Stable reading | Giá trị đo từ cảm biến ổn định qua nhiều chu kỳ, không bị nhảy giá trị đột ngột hoặc nhiễu | Dữ liệu lấy mẫu và lọc trung bình ổn định qua 30 chu kỳ, sai số < 3% | PASS |
| D03 | Actuator | ESP32 điều khiển chính xác cơ cấu chấp hành: kích hoạt Relay Bật/Tắt bơm nước mini và xuất xung PWM điều khiển góc quay Động cơ Servo đóng/mở rèm | Relay đóng ngắt bơm nước dứt khoát; Động cơ Servo quay đúng góc mở/đóng rèm che nắng | PASS |
| D04 | Error handling | Hệ thống có cơ chế bảo vệ an toàn: tự ngắt bơm khi quá thời gian timeout cài đặt và tự động kết nối lại khi mất mạng | Bơm tự động ngắt an toàn khi hết thời gian cho phép; ESP32 tự động reconnect khi khôi phục kết nối Wi-Fi/MQTT | PASS |

## Evidence
- Screenshot: Ảnh chụp màn hình Serial Monitor hiển thị dữ liệu đo cảm biến, trạng thái đóng ngắt bơm và góc quay servo; ảnh chụp Giao diện Web hiển thị đồ thị và nút điều khiển
- Video: Clip quay lại quá trình hoạt động thực tế của phần cứng: đọc cảm biến độ ẩm đất, cảm biến nhiệt độ, bơm mini tưới nước và động cơ servo kéo rèm che nắng.
- Raw data: Bảng dữ liệu trích xuất giá trị cảm biến theo thời gian (Timestamp, Soil Moisture, Temperature) và trạng thái hoạt động thực tế của bơm (Pump State), rèm (Curtain State).