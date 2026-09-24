## GET /api/devices

- Purpose: Lấy trạng thái kết nối mạng của thiết bị đầu cuối ESP32 tới Raspberry Pi 4.
- Response: HTTP 200 OK kèm JSON gồm status success, mảng devices chứa device_id, name, location, status (online/offline), firmware_version, last_seen.

## GET /api/measurements

- Purpose: Truy vấn dữ liệu đo lường lịch sử (độ ẩm đất, nhiệt độ) từ Database (PostgreSQL/InfluxDB trên Pi 4) để vẽ đồ thị giám sát trên Giao diện Web.
- Parameters: device_id (string, bắt buộc), metric (string, tùy chọn: soil_moisture, temperature), from (ISO 8601), to (ISO 8601), limit (integer, mặc định 50).
- Response: HTTP 200 OK kèm JSON chứa device_id, total và mảng data các bản ghi mẫu (soil_moisture, temperature) gồm id, timestamp, metric, value, unit.

## GET /api/events

- Purpose: Lấy lịch sử các sự kiện kích hoạt tự động (đất < 30% ra lệnh bơm, nhiệt độ > 32°C ra lệnh mở rèm) và các lần điều khiển thủ công.
- Parameters: device_id (string, bắt buộc), severity (string, tùy chọn: INFO, WARNING, CRITICAL), status (string, tùy chọn: ACTIVE, RESOLVED), limit (integer).
- Response: HTTP 200 OK kèm JSON chứa mảng events: event_type "EXTREME_HEAT", severity "WARNING", message "Nhiệt độ ban công vượt ngưỡng 32°C! Hệ thống đã kích hoạt kéo rèm che nắng."

## POST /api/commands

- Purpose: Tiếp nhận lệnh điều khiển từ 2 nút bấm trên Giao diện Web ("Tưới nước", "Đóng/Mở rèm") chuyển tiếp tới Node-RED để publish MQTT xuống ESP32.
- Request: JSON chứa device_id, command (CONTROL_PUMP, CONTROL_CURTAIN), requested_state (ON, OFF, OPEN, CLOSE), parameters (duration_seconds).
- Response: HTTP 202 Accepted kèm JSON chứa command_id, device_id, command, requested_state, actual_state (PENDING), created_at, message xác nhận đã đẩy lệnh vào hàng đợi MQTT.