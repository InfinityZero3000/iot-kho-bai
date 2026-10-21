# Hướng dẫn cài đặt và cấu hình

Dự án: **Hệ thống chiếu sáng kho bãi tự động theo khu vực có xe nâng**
(3 PIR, 3 relay, ESP32, MQTT, Node-RED, Blynk).

Repo có **hai bản triển khai** dùng chung ý tưởng nhưng khác topic MQTT. Chọn một bản để chạy, không trộn lẫn.

| | Bản A: Wokwi + dashboard.html | Bản B: Node-RED + Blynk + index.html |
|---|---|---|
| Thư mục | `src/`, `platformio.ini`, `diagram.json`, `wokwi.toml`, `dashboard.html` | `esp32/`, `node-red/`, `index.html`, `mqtt.min.js` |
| Chạy trên | Mô phỏng Wokwi | ESP32 thật (hoặc Wokwi nếu tự chỉnh) |
| Broker | `broker.hivemq.com` (công cộng, không cần cài) | Broker cục bộ (aedes, cổng 1883) |
| Giao diện | `dashboard.html` | Node-RED Dashboard (`/ui`), Blynk, `index.html` |
| Topic | `huit-khobai-nht/khuN/...` | `kho/zone/A..C`, `kho/status`, `kho/cmd/mode` |

## 1. Công cụ chung

| Công cụ | Dùng cho | Ghi chú |
|---|---|---|
| VS Code | soạn code, chạy Wokwi | |
| PlatformIO (extension **PlatformIO IDE** hoặc CLI) | build firmware ESP32 | CLI: `pip install platformio` |
| Wokwi Simulator (extension **Wokwi for VS Code**) | mô phỏng mạch | cần đăng nhập tài khoản Wokwi để lấy license miễn phí |
| Node.js 18 trở lên | chỉ cho bản B (Node-RED, broker) | kiểm tra: `node -v` |
| Tài khoản Blynk | chỉ cho bản B, tùy chọn | https://blynk.cloud |

**Dung lượng:** lần build đầu tiên PlatformIO tải nền tảng `espressif32` vào `~/.platformio/`, khoảng **4 GB** (toolchain của mọi dòng ESP32). Các lần sau dùng lại, không tải nữa. Muốn xóa: `rm -rf ~/.platformio`.

## 2. Bản A: chạy Wokwi và dashboard.html

### 2.1 Build firmware

```bash
pio run          # tạo .pio/build/esp32dev/firmware.bin và firmware.elf
```

Lỗi `Wokwi: firmware binary .pio/build/esp32dev/firmware.bin not found in workspace` nghĩa là chưa build, hãy chạy lệnh trên trước.

### 2.2 Chạy mô phỏng

1. Mở thư mục dự án trong VS Code.
2. `F1` → **Wokwi: Start Simulator**.
3. ESP32 tự nối WiFi `Wokwi-GUEST` (không mật khẩu, kênh 6) và kết nối `broker.hivemq.com:1883`.
4. Bấm vào một cảm biến PIR trong mô phỏng → **Simulate Motion**. Relay khu đó đóng, LED sáng, giữ 5 giây sau khi hết chuyển động.

Cổng WiFi công cộng của Wokwi miễn phí, nhưng lưu lượng được Wokwi giám sát nên **không gửi dữ liệu riêng tư**.

### 2.3 Mở dashboard

Mở file `dashboard.html` bằng trình duyệt (bấm đúp). Trang kết nối `wss://broker.hivemq.com:8884/mqtt` và hiển thị trạng thái PIR, đèn, chế độ từng khu. Nút **Tự động / Bật / Tắt** gửi lệnh về ESP32.

Cần máy tính có Internet vì trang nạp thư viện `mqtt.js` từ CDN.

### 2.4 Cấu hình cần chú ý

| Mục | File | Ghi chú |
|---|---|---|
| `PREFIX` = `huit-khobai-nht` | `src/main.cpp` | Broker công cộng dùng chung với mọi người, nên đổi thành chuỗi riêng của bạn |
| Ô "Tiền tố topic" | `dashboard.html` (mục Cài đặt kết nối) | Phải trùng `PREFIX` |
| `HOLD_MS` = 5000 | `src/main.cpp` | Thời gian giữ đèn sau khi xe nâng rời khu |
| Chân GPIO | `src/main.cpp`, `diagram.json` | PIR: 13, 14, 27. Relay: 25, 26, 33 |

Topic MQTT của bản A:

| Topic | Hướng | Giá trị |
|---|---|---|
| `huit-khobai-nht/khuN/pir` | ESP32 → broker | `1` hoặc `0` |
| `huit-khobai-nht/khuN/den` | ESP32 → broker | `ON` hoặc `OFF` |
| `huit-khobai-nht/khuN/chedo` | ESP32 → broker | `AUTO`, `ON`, `OFF` |
| `huit-khobai-nht/khuN/lenh` | dashboard → ESP32 | `AUTO`, `ON`, `OFF` |
| `huit-khobai-nht/trangthai` | ESP32 → broker | `online` / `offline` (Last Will) |

## 3. Bản B: ESP32 thật, Node-RED, Blynk

### 3.1 Nối dây

| Linh kiện | Chân ESP32 |
|---|---|
| PIR khu A, B, C (OUT) | GPIO 34, 35, 32 (chân chỉ-input) |
| Relay khu A, B, C (IN) | GPIO 25, 26, 27 |
| PIR, relay VCC | 5V (VIN) |
| GND | GND chung |

PIR cấp 5V và ngõ ra OUT 3,3V nên nối thẳng vào ESP32. Module relay thường kích mức thấp, đã khai báo `RELAY_ACTIVE_LOW = true` trong code; nếu relay của bạn kích mức cao thì đổi thành `false`.

### 3.2 Nạp firmware

Sửa 3 dòng đánh dấu `TODO` đầu file `esp32/src/main.cpp`:

```cpp
const char* WIFI_SSID = "TEN_WIFI";
const char* WIFI_PASS = "MAT_KHAU";
const char* MQTT_HOST = "192.168.1.10";   // IP máy chạy broker + Node-RED
```

Rồi nạp:

```bash
cd esp32
pio run -t upload
pio device monitor        # xem log, tốc độ 115200
```

Đừng commit WiFi và mật khẩu thật lên GitHub.

### 3.3 Broker và Node-RED

```bash
cd node-red
cp .env.example .env      # rồi sửa BLYNK_TOKEN (và LAMP_W nếu cần)
(cd .data && npm install) # lần đầu: cài node-red-dashboard
./start.sh                # chạy broker MQTT (cổng 1883) và Node-RED
```

- Editor Node-RED: http://localhost:1880
- Dashboard: http://localhost:1880/ui
- `LAMP_W` là công suất mỗi dải đèn (W), mặc định 150, dùng để ước tính điện năng tiết kiệm.
- Lần chạy đầu `npx` tải `aedes-cli` và `node-red`, cần Internet.
- Chạy kiểm thử logic của flow: `node node-red/test.js` (in `OK` nếu đạt).

Topic MQTT của bản B:

| Topic | Hướng | Nội dung |
|---|---|---|
| `kho/zone/A`, `B`, `C` | ESP32 → Node-RED | `{"pir":0\|1,"on":0\|1}` (retained) |
| `kho/status` | ESP32 → Node-RED | `online` / `offline` (Last Will) |
| `kho/cmd/mode` | Node-RED → ESP32 | `auto`, `on`, `off` |

### 3.4 Blynk

1. Tạo **Template** và **Device**, chép **Auth Token** vào `node-red/.env` (`BLYNK_TOKEN=...`).
2. Tạo các Datastream:

| Pin | Kiểu | Ý nghĩa |
|---|---|---|
| V0, V1, V2 | Integer 0/1 | Đèn khu A, B, C đang sáng |
| V3 | Integer 0/1 | ESP32 online |
| V4 | Integer | Phần trăm khu đang tắt (mức tiết kiệm) |
| V5 | Double | Điện năng đã dùng (kWh) |
| V6 | Double | Điện năng tiết kiệm (kWh) |
| V10 | String | Lệnh chế độ `auto` / `on` / `off` (Node-RED đọc mỗi 5 giây rồi gửi xuống ESP32) |

3. Trong app Blynk thêm widget LED cho V0 đến V3, Gauge hoặc Value Display cho V4 đến V6, và Segmented Switch cho V10.

Nếu thiếu `BLYNK_TOKEN`, node Blynk trong flow hiện trạng thái đỏ "thiếu BLYNK_TOKEN" và bỏ qua, phần còn lại vẫn chạy.

### 3.5 index.html

Trang minh họa kho bằng bản đồ động, có hai chế độ:

- **Mô phỏng:** kéo xe nâng trên bản đồ, không cần phần cứng hay broker. Mở bằng cách bấm đúp `index.html`.
- **Live MQTT:** nối broker qua **WebSocket** (mặc định `ws://localhost:9001`, đổi trong Cài đặt). `start.sh` hiện chỉ mở TCP 1883, nên muốn dùng chế độ này cần bật thêm cổng WebSocket cho broker (chưa kiểm chứng trong repo này).

## 4. Sự cố thường gặp

| Triệu chứng | Cách xử lý |
|---|---|
| `firmware binary ... not found` trong Wokwi | Chạy `pio run` rồi khởi động lại mô phỏng |
| Dashboard hiện "Đang kết nối broker..." mãi | Kiểm tra Internet; mạng công ty hoặc trường có thể chặn cổng 8884 |
| Dashboard kết nối nhưng không có dữ liệu | `PREFIX` trong code và ô Tiền tố trên dashboard chưa trùng |
| ESP32 trong Wokwi không vào được MQTT cục bộ | Cổng WiFi công cộng không tới được `localhost`; dùng broker công cộng (bản A) hoặc bật Private Gateway (tính phí) |
| Relay đóng ngược (đèn sáng khi không có xe) | Đổi `RELAY_ACTIVE_LOW` |
| PIR báo liên tục lúc mới cấp nguồn | Chờ khoảng 30 đến 60 giây cho PIR ổn định |
| `pio run` rất lâu ở lần đầu | Đang tải khoảng 4 GB toolchain, chờ hoặc xem tiến độ bằng `pio run -v` |
