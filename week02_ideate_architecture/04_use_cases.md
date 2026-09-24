# 4. Use Cases

## UC01
- Actor: Hệ thống (Edge Device / ESP32)
- Trigger: Cảm biến độ ẩm đất ghi nhận giá trị giảm xuống dưới ngưỡng cài đặt (ngưỡng đất khô).

- Main flow:. 
1. Vi điều khiển ESP32 đọc định kỳ giá trị độ ẩm từ cảm biến đất.
2. ESP32 phát hiện độ ẩm dưới ngưỡng cài đặt.
3. ESP32 kích hoạt rơ-le bật máy bơm mini tưới nhỏ giọt vào chậu cây, đồng thời cập nhật trạng thái bơm đang bật lên ứng dụng Blynk.
4. ESP32 liên tục giám sát độ ẩm đất; khi độ ẩm đạt ngưỡng đủ ẩm cài đặt hoặc chạm thời gian tưới an toàn tối đa, ESP32 ngắt rơ-le tắt máy bơm.
5. ESP32 cập nhật trạng thái chu kỳ tưới hoàn thành lên Blynk App.

- Expected result: Cây được cấp nước kịp thời, không bị héo vì thiếu nước và người dùng theo dõi được trên app Blynk.


## UC02
- Actor:  Hệ thống (Edge Device / ESP32).

- Trigger: Cảm biến nhiệt độ ghi nhận nhiệt độ môi trường ban công vượt ngưỡng nắng nóng cài đặt.

- Main flow:
1. Cảm biến DHT định kỳ gửi dữ liệu nhiệt độ và độ ẩm không khí về ESP32.
2. ESP32 phát hiện nhiệt độ vượt ngưỡng an toàn cài đặt cho cây trồng ban công.
3. ESP32 phát xung điều khiển 1 động cơ (servo/động cơ bước) quay kéo màn chắn vào che phủ phía trên giàn cây.
4. Hệ thống gửi thông báo sự kiện (Blynk Notification) về điện thoại thông báo màn chắn đã được kéo vào để che nắng cho cây.
5. Khi nhiệt độ hạ xuống dưới ngưỡng cảnh báo và thời tiết dịu mát trở lại, ESP32 tự động điều khiển động cơ quay ngược lại để thu màn chắn, giúp cây tiếp tục đón ánh sáng tự nhiên.

- Expected result: Giàn cây ban công được che chắn kịp thời trong các đợt nắng nóng gay gắt, ngăn ngừa cháy lá khi chủ vắng nhà.


## UC03
- Actor: Nhân viên văn phòng (Người dùng).

- Trigger: Người dùng đang ở nơi làm việc mở điện thoại để kiểm tra vườn cây hoặc muốn chủ động tưới/kéo màn chắn thủ công.

- Main flow:
1. Người dùng mở ứng dụng Blynk Mobile trên điện thoại.
2. Ứng dụng kết nối Blynk Cloud hiển thị thông số thời gian thực: nhiệt độ, độ ẩm không khí, độ ẩm đất và trạng thái bơm/màn chắn.
3. Người dùng nhấn nút gạt (Button Widget) để "Tưới ngay" hoặc "Kéo/Thu màn chắn".
4. Blynk Cloud truyền lệnh qua Virtual Pin tương ứng xuống vi điều khiển ESP32.
5. ESP32 nhận lệnh, kích hoạt rơ-le bơm nước hoặc điều khiển động cơ màn chắn theo yêu cầu.
6. ESP32 phản hồi trạng thái thực tế lên Blynk App để đồng bộ giao diện.

- Expected result:Người dùng nắm bắt trực quan tình trạng vườn cây mọi lúc mọi nơi và chủ động can thiệp chăm sóc cây chỉ với 1 chạm từ văn phòng.



