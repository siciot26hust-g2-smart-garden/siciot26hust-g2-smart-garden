# TÀI LIỆU KIẾN TRÚC VÀ QUY TRÌNH HOẠT ĐỘNG (SYSTEM ARCHITECTURE & WORKFLOW)

Hệ thống Smart Garden được thiết kế theo kiến trúc phân tán (Distributed Architecture) nhằm đảm bảo tính ổn định và khả năng chịu lỗi cao (Fault Tolerance). Hệ thống bao gồm hai thành phần xử lý chính hoạt động song song.

## 1. Node Chấp hành và Dự phòng (ESP32 - Edge Node)
ESP32 đóng vai trò là thiết bị biên (Edge Device), trực tiếp giao tiếp với các linh kiện vật lý và thực thi các lớp bảo vệ phần cứng mức thấp (Hardware Fail-safe).

* **Thu thập dữ liệu:** Định kỳ đọc các thông số từ cảm biến nhiệt độ, độ ẩm (DHT11), mực nước (HC-SR04) và độ ẩm đất.
* **Truyền thông (Telemetry):** Đóng gói dữ liệu thành định dạng JSON và xuất bản (publish) lên MQTT Broker (chủ đề garden/telemetry) với chu kỳ 3 giây/lần.
* **Điều khiển Động cơ Rèm (Servo):** Vận hành độc lập dựa trên cảm biến nhiệt độ cục bộ:
  * Khi nhiệt độ > 32°C: Động cơ servo mở rèm.
  * Khi nhiệt độ < 30°C: Động cơ servo đóng rèm.
  * Quá trình vận hành sử dụng xung PWM phi đồng bộ, điều chỉnh góc quay tuyến tính trong khoảng 1.8 giây nhằm tối ưu hóa chuyển động cơ học.
* **Cơ chế an toàn (Chống cạn nước):** Trực tiếp ngắt máy bơm ngay lập tức nếu cảm biến siêu âm phát hiện bình chứa cạn nước, hoặc nếu thời gian bơm vượt quá 180 giây, nhằm bảo vệ thiết bị.
* **Chế độ Dự phòng (Failover):** ESP32 liên tục giám sát tín hiệu kết nối (Heartbeat) từ Node-RED. Nếu quá thời gian giới hạn (30 giây) không nhận được tín hiệu, ESP32 tự động chuyển sang trạng thái ESP32_FAILOVER. Ở trạng thái này, ESP32 tự quản lý việc tưới tiêu (Bơm khi độ ẩm đất < 30%) để đảm bảo cây trồng không bị thiếu nước khi mất kết nối mạng.

## 2. Node Trung tâm (Node-RED - Central Controller)
Node-RED đóng vai trò là máy chủ ứng dụng, xử lý logic nghiệp vụ chính và cung cấp giao diện tương tác người dùng (Dashboard).

* **Duy trì kết nối (Heartbeat):** Liên tục phát tín hiệu garden/pi/ready đến ESP32 sau mỗi chu kỳ nhận Telemetry để duy trì trạng thái điều khiển trung tâm (PI_MODE).
* **Điều khiển Tưới tiêu Tự động:**
  * Tiếp nhận dữ liệu độ ẩm đất từ ESP32.
  * Khi độ ẩm < 30%: Ban hành lệnh Bật máy bơm.
  * Khi độ ẩm >= 60%: Ban hành lệnh Tắt máy bơm.
* **Giao thức Ưu tiên Điều khiển Thủ công (Manual Override):**
  * Giao diện người dùng (Dashboard) luôn cho phép can thiệp và điều khiển thiết bị trực tiếp ở bất kỳ thời điểm nào, bất kể hệ thống đang ở chế độ Tự động.
  * Khi có tín hiệu điều khiển thủ công, Node-RED lập tức thiết lập chuỗi lệnh ưu tiên: Chuyển đổi trạng thái sang Manual -> Thực thi lệnh điều khiển của người dùng -> Khởi tạo bộ đếm thời gian.
  * Sau 15 giây, hệ thống tự động khôi phục về trạng thái Tự động (Auto Mode). Cơ chế này đảm bảo người dùng có toàn quyền kiểm soát trực tiếp thiết bị theo ý muốn, đồng thời ngăn ngừa rủi ro hệ thống bị treo ở chế độ thủ công do người dùng quên chuyển đổi lại.
