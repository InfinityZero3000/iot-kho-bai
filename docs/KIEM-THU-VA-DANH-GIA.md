# Kiểm thử và đánh giá

Ngày kiểm: 18/11/2026. Cột "Bằng chứng" ghi rõ đã chạy thật hay chỉ tính toán. Mục chưa kiểm chứng được liệt kê riêng, không ghi là đã đạt.

## 1. Kết quả kiểm thử

| # | Kịch bản | Kết quả mong đợi | Kết quả thực tế | Bằng chứng |
|---|---|---|---|---|
| 1 | Build firmware bản A (`pio run`) | Biên dịch thành công | Đạt. RAM 14,4%, Flash 73,0% | Chạy thật, log `SUCCESS` |
| 2 | Build firmware bản B (`pio run -d esp32`) | Biên dịch thành công | Đạt. RAM 14,4%, Flash 73,0% | Chạy thật |
| 3 | Logic flow Node-RED (`node node-red/test.js`) | In `OK` | Đạt, gồm cả ca cầu nối Wokwi | Chạy thật |
| 4 | Mạng tới `broker.hivemq.com` cổng 1883 và 8884 | Mở được | Đạt cả hai | Chạy thật (`nc -z`) |
| 5 | Wokwi → HiveMQ → Node-RED: khu 2 có xe (`khu2/pir=1`, `khu2/den=ON`, `trangthai=online`) | Broker cục bộ có `kho/zone/B = {"pir":1,"on":1}` và `kho/status = online` | Đạt | Chạy Node-RED thật + HiveMQ thật, gửi đúng topic/payload của firmware |
| 6 | Node-RED → Wokwi: đặt `kho/cmd/mode = off` | Gửi `OFF` tới `khu1..3/lenh` | Đạt, nhận đủ 3 lệnh | Như trên |
| 7 | Wokwi online thì xe nâng giả tắt | Không ghi đè dữ liệu thật | Đạt ở mức logic | `test.js` |

Ghi chú ca 5 và 6: dùng một client giả đóng vai firmware, không phải Wokwi thật.

## 2. Chưa kiểm chứng được

| Mục | Lý do |
|---|---|
| ESP32 trong Wokwi vào WiFi `Wokwi-GUEST` và kết nối MQTT | Wokwi cần tài khoản và license, chỉ chạy được qua VS Code của người dùng. Cần xem Serial Monitor có dòng `MQTT da ket noi`. |
| PIR bấm trong Wokwi làm relay đóng, LED sáng, giữ 5 giây | Như trên |
| Gửi lệnh về ESP32 làm đổi chế độ | Phần broker đã đạt (ca 6), phần firmware nhận lệnh chưa chạy trong mô phỏng |
| Hiển thị trên `index.html`, `dashboard.html`, Node-RED `/ui`, Blynk | Chưa mở bằng trình duyệt hay app. Blynk cần `BLYNK_TOKEN` |
| Chạy trên ESP32 thật | Chưa có phần cứng |

## 3. Mức tiết kiệm điện (tính toán, chưa đo)

Giả định: 3 khu, mỗi khu 150 W (`LAMP_W`), mỗi lúc có một xe nâng đứng trong đúng một khu, thời gian giữ đèn không đáng kể.

- Đèn luôn bật: 3 × 150 W = 450 W.
- Đèn theo xe nâng: 1 × 150 W = 150 W.
- Tiết kiệm: (450 − 150) / 450 = **66,7%**.

Số liệu thật lấy từ Node-RED Dashboard (ô "Điện năng") hoặc `index.html` sau vài phút demo, rồi thay vào đây. Thời gian giữ đèn 5 giây (`HOLD_MS`) chỉ để demo. Kho thật cần 30 đến 60 giây, và khi đó mức tiết kiệm thấp hơn con số trên.

## 4. Bảng linh kiện (BOM)

Giá tham khảo, cần kiểm tra lại khi mua.

| Linh kiện | SL | Ghi chú |
|---|---|---|
| ESP32 DevKit | 1 | WiFi, 3 chân đọc PIR, 3 chân điều khiển relay |
| Cảm biến PIR HC-SR501 | 3 | Mỗi hành lang một cái |
| Module relay 1 kênh 5V | 3 | Đóng ngắt từng dải đèn |
| Dải đèn LED | 3 | Theo phân khu |
| Nguồn 5V, dây, hộp | 1 bộ | |

## 5. Hạn chế

- **Broker công cộng không xác thực.** Bản A dùng `broker.hivemq.com`, ai đoán được `PREFIX` đều đọc và gửi lệnh được. Chỉ phù hợp demo.
- **Không mã hóa.** MQTT chạy cổng 1883, không TLS.
- **Token Blynk** nằm trong `node-red/.env`, không được đưa lên Git (đã có trong `.gitignore`).
- **Cổng WiFi Wokwi bị giám sát**, không gửi dữ liệu riêng tư.
- **Hai bản triển khai** dùng hai bộ topic khác nhau, được nối bằng cầu nối trong Node-RED (xem `docs/CAI-DAT.md` mục 3.6).
- **Relay chỉ bật hoặc tắt**, khu không có xe tối hẳn. Với kho thật cần mức sáng tối thiểu để an toàn.
- **PIR** có tầm và góc hạn chế, phản ứng chậm với xe chạy nhanh.

## 6. Hướng phát triển

- Bật trước khu kế bên khi xe nâng sắp tới, để an toàn hơn.
- Cảm biến ánh sáng (LDR) để ban ngày không bật đèn.
- Điều chỉnh độ sáng bằng PWM: khu không có xe sáng mờ, khu có xe sáng đủ.
- MQTT có TLS và tài khoản riêng, cập nhật firmware qua mạng (OTA).
- Nhận diện xe nâng bằng RFID thay cho PIR, giảm báo nhầm.
