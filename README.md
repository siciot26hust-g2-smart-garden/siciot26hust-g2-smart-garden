# siciot26hust-g2
Dự án Capstone
# TÓM TẮT DỰ ÁN VƯỜN THÔNG MINH

**1. Mô hình hoạt động:** 
Gồm 2 bộ não: **Node-RED** (Giao diện web & Điều khiển trung tâm) và **ESP32** (Đọc cảm biến, chạy động cơ & Bảo vệ phần cứng).

**2. Tính năng Tự động (Mặc định khi bật máy):**
- 💦 **Tưới nước:** Độ ẩm đất < 30% ➔ Tự Bơm | Độ ẩm đất >= 60% ➔ Tự Tắt.
- 🌤️ **Rèm che nắng:** Nhiệt độ > 32°C ➔ Kéo mở rèm | Nhiệt độ < 30°C ➔ Đóng rèm.

**3. Cơ chế Nâng cao:**
- **Chỉnh tay (Ghi đè 15s):** Bấm nút trên Web ➔ Hệ thống lập tức nghe theo ➔ 15 giây sau tự động khôi phục chế độ Auto.
- **Bảo vệ Bơm:** Tự ngắt bơm lập tức nếu Bồn hết nước (cảm biến siêu âm) hoặc bơm kẹt quá 3 phút.
- **Mất mạng (Failover):** Nếu Node-RED/Web bị sập, ESP32 sẽ tự động đứng ra tiếp quản việc tưới cây, đảm bảo cây không bao giờ héo.

**4. Chân cắm ESP32:**
- **D4:** DHT11 (Nhiệt/Ẩm)
- **D34:** Cảm biến Đất
- **D5 / D18:** Siêu âm (TRIG / ECHO)
- **D25:** Rơ-le Máy Bơm (Bắt buộc gạt Jumper sang mức L)
- **D27:** Động cơ Rèm (Servo)
