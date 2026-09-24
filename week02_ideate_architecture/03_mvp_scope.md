# 3. MVP Scope

## MUST HAVE
[x] Thiết bị đầu cuối ESP32 thu thập dữ liệu thời gian thực từ Cảm biến độ ẩm đất và Cảm biến nhiệt độ.

[x] Trung tâm xử lý Raspberry Pi 4 vận hành MQTT Broker (Mosquitto), engine xử lý Node-RED / Python, Database (PostgreSQL/InfluxDB) và Web Server.

[x] Tự động kích hoạt Bơm nước mini khi độ ẩm đất dưới mức cài đặt và ngắt khi đạt độ ẩm tiêu chuẩn.

[x] Tự động kích hoạt Động cơ Servo quay mở rèm che nắng khi nhiệt độ vượt mức cài đặt và thu rèm khi nhiệt độ hạ mát.

[x] Giao diện Web (PC / Điện thoại) kết nối qua HTTP / WebSocket hiển thị đồ thị giám sát thời gian thực và cung cấp chức năng đóng, mở lưới và tưới tiêu.

[x] Kết nối truyền thông mạng hai chiều ổn định qua Router Wi-Fi bằng giao thức MQTT (Publish / Subscribe).


## SHOULD HAVE
[x] Lưu trữ đầy đủ dữ liệu đo lường và nhật ký điều khiển vào Database (PostgreSQL/InfluxDB) trên Raspberry Pi 4.

[x] Cho phép tùy biến ngưỡng kích hoạt (30% độ ẩm đất, 32°C nhiệt độ) trực tiếp trên Giao diện Web.


## NICE TO HAVE
- Biểu đồ trực quan hóa dữ liệu lịch sử độ ẩm đất và nhiệt độ chuyên sâu trên Giao diện Web.
- Chế độ "Đi công tác" (Vacation Mode) cấu hình thời gian chạy bơm tối ưu trên Web.
- Tích hợp API dự báo thời tiết OpenWeatherMap hoãn tưới nếu trời sắp có mưa to.


## Out of Scope
- Màn hình hiển thị tại chỗ (LCD) và còi báo động tại chỗ (Buzzer) (toàn bộ giám sát chuyển lên Giao diện Web).
- Cảm biến đo mức nước bình chứa (hệ thống sử dụng bình cấp dung tích lớn người dùng tự kiểm tra định kỳ hoặc cấp nước trực tiếp).
- Hệ thống pha và châm dinh dưỡng/phân bón tự động NPK.
- Camera AI nhận diện sâu bọ và phân tích sức khỏe lá cây.
- Vỏ hộp công nghiệp tiêu chuẩn chống nước IP68 và hệ thống cấp nguồn pin năng lượng mặt trời công suất lớn.


## Scope Freeze
Ngày:
Người xác nhận:
