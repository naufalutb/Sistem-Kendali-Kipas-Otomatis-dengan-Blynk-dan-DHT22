# Sistem Kendali Kipas Otomatis dengan Blynk dan DHT22

Proyek Akhir mata kuliah **Sistem Mikrokontroler** — sistem pemantauan suhu ruangan
berbasis **ESP32** yang mengontrol kipas secara otomatis, memberi peringatan bertingkat
(NORMAL / WARNING / DANGER), dan dapat dipantau serta dikontrol dari jarak jauh lewat
aplikasi **Blynk IoT**.

- **Nama / NIM:** Muhammad Naufal Hanif · 25552012005
- **Kelas:** TIF PK 23 CID — Departemen Informatika, UTB
- **Dosen Pengampu:** Muchamad Rusdan, S.T., M.T.
- **Simulator:** [Wokwi](https://wokwi.com/projects/466367151242034177)

---

## Deskripsi

Perangkat elektronik dan ruangan mudah mengalami *overheat*; jika dibiarkan, suhu tinggi
menurunkan kinerja dan merusak komponen. Pemantauan manual tidak praktis karena tidak
real-time. Sistem ini membaca suhu & kelembapan dari **DHT22** dan beban panas dari
**potentiometer** (simulasi), lalu menentukan status dan mengeksekusi aktuator secara
otomatis. Semua data dikirim ke Blynk sehingga dapat dipantau dan dikontrol dari jauh.

## Fitur

- Pembacaan suhu & kelembapan (DHT22) dan beban panas (potentiometer / ADC)
- Tiga tingkat status: **NORMAL**, **WARNING**, **DANGER**
- Kontrol kipas otomatis via relay berdasarkan status
- Indikator visual 3 LED (hijau/kuning/merah) + buzzer alarm saat DANGER
- Tampilan lokal LCD 16x2 (I2C): suhu, kelembapan, status, kondisi relay
- Integrasi Blynk IoT: monitoring real-time + kontrol jarak jauh
  - Atur ambang suhu dari dashboard (slider V9)
  - Aktif/matikan buzzer dari dashboard (switch V10)
  - Notifikasi event otomatis saat kondisi DANGER (dengan cooldown 30 detik)

## Perangkat Keras & Pemetaan Pin

| Komponen              | Pin ESP32           | Keterangan                        |
|-----------------------|---------------------|-----------------------------------|
| DHT22 (suhu/lembap)   | GPIO 15             | `DHT_PIN`                         |
| Potentiometer (beban) | GPIO 34 (ADC)       | `HEAT_LOAD_PIN`, input analog     |
| Relay → Kipas         | GPIO 13             | `COOLING_RELAY_PIN`               |
| Buzzer (piezo)        | GPIO 32             | `BUZZER_PIN`, PWM `ledcWriteTone` |
| LED NORMAL (hijau)    | GPIO 25             | `NORMAL_LED_PIN` + resistor 220Ω  |
| LED WARNING (kuning)  | GPIO 26             | `WARNING_LED_PIN` + resistor 220Ω |
| LED DANGER (merah)    | GPIO 27             | `DANGER_LED_PIN` + resistor 220Ω  |
| LCD 16x2              | I2C — SDA 21, SCL 22| alamat `0x27`                     |

## Library

Terdaftar di `libraries.txt` (auto-detect Wokwi dari `#include`):

- **DHT sensor library** — baca DHT22
- **LiquidCrystal I2C** — LCD 16x2 via I2C
- **Blynk** — konektivitas IoT (`BlynkSimpleEsp32`)

## Logika Status Kontrol

Diimplementasikan di fungsi `calculateStatus()` (`sketch.ino`):

```cpp
String calculateStatus(float temperature, int heatLoad) {
  bool temperatureDanger = temperature >= DANGER_TEMPERATURE_THRESHOLD; // >= 40
  bool heatDanger        = heatLoad    >= HEAT_LOAD_THRESHOLD;          // >= 2500
  if (temperatureDanger || heatDanger) return "DANGER";
  if (temperature >= temperatureThreshold) return "WARNING";           // default 30
  return "NORMAL";
}
```

| Status  | Kondisi                                          | Relay/Kipas | LED    | Buzzer                  |
|---------|--------------------------------------------------|-------------|--------|-------------------------|
| NORMAL  | suhu < ambang **dan** beban < 2500               | OFF         | Hijau  | OFF                     |
| WARNING | suhu ≥ ambang (default 20) tapi belum DANGER     | ON          | Kuning | OFF                     |
| DANGER  | suhu ≥ ambang **atau** beban panas ≥ 2500        | ON          | Merah  | ON (jika `buzzerEnabled`) |

Eksekusi aktuator ada di `setOutputs()`; relay aktif saat WARNING atau DANGER
(`coolingActive = isDanger || isWarning`).

## Konstanta Utama (`sketch.ino`)

| Konstanta                       | Nilai   | Arti                              |
|---------------------------------|---------|-----------------------------------|
| `DEFAULT_TEMPERATURE_THRESHOLD` | 30      | Ambang WARNING (bisa diubah 20–60)|
| `DANGER_TEMPERATURE_THRESHOLD`  | 40      | Ambang DANGER (suhu)              |
| `HEAT_LOAD_THRESHOLD`           | 2500    | Ambang DANGER (beban panas ADC)   |
| `SENSOR_INTERVAL_MS`            | 2000    | Interval baca sensor              |
| `LCD_INTERVAL_MS`               | 1000    | Interval refresh LCD              |
| `ALERT_COOLDOWN_MS`             | 30000   | Jeda minimal antar notifikasi     |

## Virtual Pin Blynk

| Virtual Pin | Nama                        | Arah          | Fungsi                          |
|-------------|-----------------------------|---------------|---------------------------------|
| V1          | `VPIN_TEMPERATURE`          | Device → App  | Suhu (°C)                       |
| V2          | `VPIN_HUMIDITY`             | Device → App  | Kelembapan (%)                  |
| V3          | `VPIN_HEAT_LOAD`            | Device → App  | Beban panas (nilai ADC)         |
| V5          | `VPIN_COOLING_STATUS`       | Device → App  | Status kipas (1/0)              |
| V6          | `VPIN_NORMAL_STATUS`        | Device → App  | Indikator NORMAL (1/0)          |
| V7          | `VPIN_WARNING_STATUS`       | Device → App  | Indikator WARNING (1/0)         |
| V8          | `VPIN_SYSTEM_STATUS`        | Device → App  | Status sistem (teks)            |
| V9          | `VPIN_TEMPERATURE_THRESHOLD`| App → Device  | Atur ambang suhu (slider, 20–60)|
| V10         | `VPIN_BUZZER_ENABLE`        | App → Device  | Aktif/matikan buzzer (switch)   |

## Cara Menjalankan (Wokwi)

1. Buka proyek: <https://wokwi.com/projects/466367151242034177>
2. Klik **Start Simulation**.
3. Amati LCD dan LED: kondisi awal **NORMAL** (LED hijau).
4. Putar **potentiometer** untuk menaikkan beban panas hingga status berpindah:
   - lewat ambang 30 → **WARNING** (LED kuning, kipas jalan)
   - hingga beban ≥ 2500 atau suhu ≥ 40 → **DANGER** (LED merah, kipas penuh, buzzer bunyi)
5. Ubah suhu DHT22 lewat panel sensor untuk menguji ambang suhu.

> Wokwi memakai WiFi virtual `Wokwi-GUEST` (tanpa password) untuk koneksi Blynk.

## Struktur File

```
UAS WOKWI/
├── sketch.ino          # Program utama ESP32 (Arduino/C++)
├── diagram.json        # Definisi rangkaian & wiring Wokwi
├── libraries.txt       # Daftar library
└── wokwi-project.txt    # Metadata proyek Wokwi
```

## Alur Program

```
setup(): init serial, DHT, pin, buzzer PWM, LCD → connectWifi() → connectBlynk()
         → jadwalkan readSensors() tiap 2s & updateLcd() tiap 1s

loop(): Blynk.run() + timer.run()

readSensors(): baca DHT22 & potentiometer
             → calculateStatus() → setOutputs() → publishToBlynk() → sendDangerEvent()
```

---

Dibuat untuk memenuhi tugas Proyek Akhir Sistem Mikrokontroler, Universitas Teknologi
Bandung (UTB), Genap 2026.
