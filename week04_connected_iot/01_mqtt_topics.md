# MQTT Topic Plan

| **Topic** | **Publisher** | **Subscriber** | **QoS** | **Payload** |
|---|---|---|---:|---|
| `garden/device01/telemetry` | Thiết bị đầu cuối (ESP32) | Mosquitto Broker (Pi 4) / Node-RED | 1 | JSON (`soil_moisture`, `temperature`, `timestamp`) |
| `garden/device01/state` | Thiết bị đầu cuối (ESP32) | Mosquitto (Pi 4) / Node-RED / Web Server | 1 | JSON (`pump_state`, `curtain_state`, `wifi_rssi`, `uptime`) |
| `garden/device01/alert` | Node-RED (Pi 4) / ESP32 | Web Server / Dịch vụ cảnh báo | 1 | JSON (`event_id`, `alert_type`, `severity`, `message`: < 30% bơm, > 32°C mở rèm) |
| `garden/device01/command` | Node-RED / Web Server (Pi 4) | Thiết bị đầu cuối (ESP32) | 1 | JSON (`command_id`, `action`: `PUMP/SERVO`, `requested_state`, `duration`) |

## Lưu ý

- Quy định **QoS 1** bảo đảm độ tin cậy truyền tin (**at least once**).
- **Cơ chế chống trùng lặp (Deduplication):** Sử dụng cặp định danh `device_id + message_id/event_id` với bộ nhớ đệm kiểm tra tại Backend để loại bỏ các bản tin lặp.
- Cấu hình cờ **Retain** đối với topic `garden/device01/state` để Giao diện Web ngay khi kết nối lại có thể đọc ngay trạng thái hoạt động thực tế của bơm và rèm.
- **Vai trò điều phối:** Mosquitto Broker và Node-RED vận hành trực tiếp trên Raspberry Pi 4, chịu trách nhiệm điều phối toàn bộ bản tin giữa ESP32 và hệ thống Backend, đồng thời thực hiện cơ chế chống trùng lặp.