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
| `docs/KIEM-THU-VA-DANH-GIA.md` | Kết quả kiểm thử, mức tiết kiệm điện, BOM, hạn chế, hướng phát triển |
| `docs/*.pptx` | Slide thuyết trình |

## Cài đặt và chạy (macOS và Windows)

### Cần cài trước

| Công cụ | macOS | Windows |
|---|---|---|
| VS Code + extension **Wokwi Simulator** (đăng nhập, lấy license miễn phí) | tải từ code.visualstudio.com | như macOS |
| Python 3.9+ | `brew install python` | python.org, tick **Add python.exe to PATH** |
| PlatformIO CLI | `pip3 install platformio` | `pip install platformio` |
| Node.js 18+ (chỉ cho Node-RED) | `brew install node` | nodejs.org, bản LTS |

Kiểm tra: `python3 --version` / `node -v` (macOS), `python --version` / `node -v` (Windows). Nếu gõ `pio` báo không tìm thấy, dùng `python -m platformio` (Windows) hoặc `python3 -m platformio` (macOS) thay cho `pio`.

### Bước 1. Build firmware và chạy Wokwi (bản A)

Giống nhau trên cả hai hệ điều hành, chạy trong thư mục dự án:

```bash
pio run          # lần đầu tải khoảng 4 GB toolchain
```

Trong VS Code: `F1` (macOS: `Cmd+Shift+P`, Windows: `Ctrl+Shift+P`) → **Wokwi: Start Simulator**. Bấm vào một PIR → **Simulate Motion**. Serial Monitor sẽ có dòng `MQTT da ket noi`.

### Bước 2. Mở giao diện

Mở `index.html` (bản đồ kho, bấm **Live MQTT**) hoặc `dashboard.html` bằng trình duyệt. Mac: `open index.html`. Windows: `start index.html`. Cả hai nối thẳng broker công cộng HiveMQ nên không cần cài gì thêm.

### Bước 3. Node-RED Dashboard và Blynk (tùy chọn)

**macOS** (Terminal):

```bash
cd node-red
cp .env.example .env                              # sửa BLYNK_TOKEN nếu dùng Blynk
mkdir -p .data && (cd .data && npm install node-red-dashboard)   # lần đầu
sh start.sh                                       # broker :1883 + Node-RED :1880
```

**Windows** (PowerShell; `start.sh` không chạy được, nên chạy tay hai cửa sổ):

```powershell
cd node-red
New-Item -ItemType Directory -Force .data | Out-Null
cd .data; npm install node-red-dashboard; cd ..   # lần đầu

# Cửa sổ PowerShell 1: broker MQTT
npx -y aedes-cli start --port 1883

# Cửa sổ PowerShell 2: Node-RED
cd node-red
$env:BLYNK_TOKEN = "dan_token_vao_day"            # bỏ qua nếu không dùng Blynk
$env:LAMP_W = "150"
npx -y node-red --userDir "$PWD\.data" flows.json
```

Rồi mở http://localhost:1880/ui. Dữ liệu từ Wokwi vào Node-RED qua cầu nối HiveMQ (xem [docs/CAI-DAT.md](docs/CAI-DAT.md) mục 3.6). Chạy kiểm thử logic: `node node-red/test.js`.

### Lỗi thường gặp

| Triệu chứng | Cách xử lý |
|---|---|
| Wokwi báo `firmware.bin not found` | Chạy `pio run` rồi khởi động lại mô phỏng |
| `pio` không nhận (Windows) | Mở lại terminal sau khi cài, hoặc dùng `python -m platformio` |
| PowerShell báo không cho chạy script | `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` |
| `port in use` khi chạy Node-RED | Đang có Node-RED hoặc broker cũ chạy. Tắt rồi chạy lại để nạp flow mới |
| Dashboard hiện "Đang kết nối broker..." mãi | Kiểm tra Internet; mạng trường/công ty có thể chặn cổng 8884 |

Chi tiết, bản B (ESP32 thật) và xử lý sự cố khác: xem [docs/CAI-DAT.md](docs/CAI-DAT.md).

## Trạng thái

- Logic flow Node-RED có kiểm thử: `node node-red/test.js`.
- Cả hai firmware build thành công; cầu nối Wokwi ↔ Node-RED đã thử với HiveMQ và Node-RED thật.
- Mô phỏng Wokwi, Blynk và hiển thị trên giao diện chưa được kiểm chứng. Chi tiết: [docs/KIEM-THU-VA-DANH-GIA.md](docs/KIEM-THU-VA-DANH-GIA.md).
