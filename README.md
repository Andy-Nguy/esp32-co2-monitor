# 🌬️ Hệ Thống Cảnh Báo Chất Lượng Không Khí ESP32

## 📋 Giới Thiệu

Đồ án IoT giả lập hệ thống giám sát và cảnh báo nồng độ CO2 trong không khí sử dụng ESP32. Hệ thống được mô phỏng trên **Wokwi Simulator** và tích hợp với **HiveMQ**, **Node-RED** và **Blynk** để giám sát từ xa.

## 🏗️ Kiến Trúc Hệ Thống

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   ESP32         │     │   HiveMQ        │     │   Node-RED      │
│   (Wokwi)       │────▶│   Broker        │────▶│   Dashboard     │
│                 │     │                 │     │                 │
└─────────────────┘     └────────┬────────┘     └─────────────────┘
                                 │
                                 ▼
                        ┌─────────────────┐
                        │     Blynk       │
                        │   Mobile App    │
                        └─────────────────┘
```

## 🔧 Phần Cứng (Mô Phỏng Trên Wokwi)

| Linh Kiện             | Mô Tả                              | Chân Kết Nối             |
| --------------------- | ---------------------------------- | ------------------------ |
| **ESP32 DevKit C V4** | Vi điều khiển chính                | -                        |
| **Potentiometer**     | Giả lập cảm biến CO2               | GPIO 34 (ADC)            |
| **LCD 16x2**          | Hiển thị nồng độ CO2 và trạng thái | RS:22, E:21, D4-D7:16-19 |
| **LED Xanh**          | Tượng trưng cho quạt thông gió     | GPIO 25                  |
| **LED Đỏ**            | Đèn cảnh báo nguy hiểm             | GPIO 26                  |
| **Buzzer**            | Còi báo động                       | GPIO 27                  |

## 📊 Ngưỡng Cảnh Báo

| Mức CO2 (ppm) | Trạng Thái     | Hành Động                  |
| ------------- | -------------- | -------------------------- |
| 0 - 1499      | ✅ **OK**      | Tất cả thiết bị tắt        |
| 1500 - 2999   | ⚠️ **WARNING** | Bật quạt (LED xanh)        |
| ≥ 3000        | 🚨 **ALARM**   | Bật quạt + LED đỏ + Buzzer |

## 📡 Giao Thức MQTT

### Broker

- **Server:** `broker.hivemq.com`
- **Port:** `1883`

### Topics

| Topic            | Mô Tả               | Kiểu Dữ Liệu              |
| ---------------- | ------------------- | ------------------------- |
| `car/co2/ppm`    | Giá trị nồng độ CO2 | Integer (0-5000)          |
| `car/co2/status` | Trạng thái hệ thống | String (OK/WARNING/ALARM) |

## 🚀 Hướng Dẫn Sử Dụng

### 1. Chạy Trên Wokwi

1. Truy cập [Wokwi Project](https://wokwi.com/projects/449414095413214209)
2. Nhấn **Start Simulation**
3. Xoay **Potentiometer** để thay đổi giá trị CO2 giả lập

### 2. Cấu Hình Node-RED

```json
// Import flow này vào Node-RED
[
  {
    "id": "mqtt_co2_ppm",
    "type": "mqtt in",
    "topic": "car/co2/ppm",
    "broker": "broker.hivemq.com"
  },
  {
    "id": "mqtt_co2_status",
    "type": "mqtt in",
    "topic": "car/co2/status",
    "broker": "broker.hivemq.com"
  }
]
```

**Các node cần thiết:**

- `mqtt in` - Subscribe topic từ HiveMQ
- `gauge` - Hiển thị giá trị CO2
- `text` - Hiển thị trạng thái
- `chart` - Biểu đồ theo thời gian

### 3. Cấu Hình Blynk

1. Tạo project mới trên Blynk
2. Thêm các Widget:
   - **Gauge**: Hiển thị CO2 ppm
   - **LED**: Hiển thị trạng thái
   - **SuperChart**: Biểu đồ lịch sử
3. Sử dụng **Webhook** hoặc **MQTT Integration** để nhận dữ liệu

## 📁 Cấu Trúc Project

```
HeThongCanhBaoKhongKhi/
├── sketch.ino          # Code chính ESP32
├── diagram.json        # Sơ đồ mạch Wokwi
├── libraries.txt       # Thư viện sử dụng
├── wokwi-project.txt   # Link project Wokwi
└── README.md           # Tài liệu hướng dẫn
```

## 📚 Thư Viện Sử Dụng

- **WiFi.h** - Kết nối WiFi cho ESP32
- **PubSubClient** - Client MQTT
- **LiquidCrystal** - Điều khiển LCD 16x2

## 🔄 Luồng Hoạt Động

```
1. ESP32 đọc giá trị từ Potentiometer (giả lập CO2)
         ↓
2. Chuyển đổi ADC (0-4095) → ppm (0-5000)
         ↓
3. Xác định trạng thái (OK/WARNING/ALARM)
         ↓
4. Điều khiển thiết bị đầu ra (LED, Buzzer)
         ↓
5. Hiển thị lên LCD
         ↓
6. Gửi dữ liệu lên HiveMQ qua MQTT
         ↓
7. Node-RED/Blynk nhận và hiển thị Dashboard
```

## ⚙️ Tùy Chỉnh

### Thay Đổi Ngưỡng Cảnh Báo

```cpp
const float WARNING_LEVEL = 1500.0; // Ngưỡng cảnh báo
const float ALARM_LEVEL   = 3000.0; // Ngưỡng báo động
```

### Tắt MQTT (Chạy Offline)

Comment dòng này trong code:

```cpp
// #define USE_MQTT
```

### Thay Đổi Tần Suất Gửi MQTT

```cpp
const unsigned long publishInterval = 2000; // ms (mặc định 2 giây)
```

## 👨‍💻 Tác Giả

**Duy Anh Nguyen**

## 📝 License

MIT License - Tự do sử dụng cho mục đích học tập và nghiên cứu.

---

⭐ **Nếu project hữu ích, hãy cho một Star!**
