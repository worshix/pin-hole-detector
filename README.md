# PIN HOLE DETECTOR

A Django dashboard and ESP32-CAM firmware starter for capturing high-quality plastic roll images and storing them for future defect-model training.

## Firmware

Open `firmware/pinhole-detector-espcam/pinhole-detector-espcam.ino` in Arduino IDE.

Required Arduino libraries:

- ESP32 board package
- PubSubClient

Use board settings for an AI Thinker ESP32-CAM style module. The sketch connects to Wi-Fi `pinhole-detector`, listens to MQTT topic `pinhole/control`, and uploads JPEG frames to `http://192.168.137.1:8000/api/images/receive/` once per second after receiving `start`.

## Django app

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
python manage.py migrate
python manage.py runserver 0.0.0.0:8000
```

Open `http://127.0.0.1:8000/` for the dashboard.

## MQTT

The dashboard publishes `start` and `stop` to topic `pinhole/control` on broker `192.168.137.1:1883`. Make sure Mosquitto is running and reachable from both the PC and ESP32-CAM.

## Image upload API

Endpoint: `/api/images/receive/`

The ESP32-CAM can post raw JPEG bytes with `Content-Type: image/jpeg`. Uploaded files are stored under `media/captures/` and shown on the dashboard/history pages.
