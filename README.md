# Hệ thống chiếu sáng kho bãi tự động theo khu vực có xe nâng

Tiết kiệm điện cho kho logistics diện tích lớn: chia kho thành các phân khu, đèn chỉ sáng hoàn toàn ở khu có xe nâng đi qua.

- **Cảm biến:** 3 PIR đặt ở các hành lang lối đi.
- **Chấp hành:** 3 module relay đóng ngắt dải đèn LED từng phân khu.
- **Công nghệ:** ESP32, MQTT, Node-RED, Blynk.

```
PIR x3 -> ESP32 -> MQTT broker -> Node-RED -> Blynk App / Dashboard
              |
         Relay x3 -> dải đèn LED (3 phân khu)
```

## Cấu trúc

| Đường dẫn | Nội dung |
|---|---|
| `src/`, `platformio.ini`, `diagram.json`, `wokwi.toml` | Bản A: firmware và mạch mô phỏng Wokwi (ESP32, 3 PIR, 3 relay) |
| `dashboard.html` | Bản A: dashboard web nhận dữ liệu MQTT qua broker công cộng |
| `esp32/` | Bản B: firmware cho ESP32 thật (WiFi, MQTT cục bộ) |
| `node-red/` | Bản B: flow Node-RED, script chạy broker, kiểm thử logic |
| `index.html`, `mqtt.min.js` | Bản B: trang minh họa bản đồ kho, có chế độ mô phỏng và live |
| `docs/CAI-DAT.md` | **Hướng dẫn cài đặt và cấu hình chi tiết** |
| `docs/*.pptx` | Slide thuyết trình |

## Chạy nhanh bản A (Wokwi)

```bash
pip install platformio
pio run                   # lần đầu tải khoảng 4 GB toolchain
# VS Code: F1 -> Wokwi: Start Simulator
# rồi mở dashboard.html bằng trình duyệt
```

Chi tiết, bản B và xử lý sự cố: xem [docs/CAI-DAT.md](docs/CAI-DAT.md).

## Trạng thái

- Logic flow Node-RED có kiểm thử: `node node-red/test.js`.
- Firmware và mô phỏng Wokwi chưa được kiểm chứng chạy đầy đủ trong repo này.
