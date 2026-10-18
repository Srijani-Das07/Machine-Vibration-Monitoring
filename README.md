# Machine Vibration Monitoring

Predictive machine vibration monitoring using adaptive baseline calibration. Details TBD.

## Project Structure

```
Machine-Vibration-Monitoring/
├── data/
│   ├── raw/                     # MaFaulDa dataset (not tracked, download separately)
│   └── processed/               # extracted features, cached demo windows
├── notebooks/
│   └── eda.ipynb                # exploratory data analysis
├── src/
│   ├── features.py              # RMS, dominant freq, spectral energy, kurtosis
│   ├── calibration.py           # adaptive baseline calibration
│   ├── dataset.py                # MaFaulDa dataset + synthetic dataset loaders
│   ├── model.py                   # classifier definition, save/load
│   ├── train.py                   # offline training script
│   └── ablation.py                # fixed-baseline vs adaptive-baseline comparison
├── results/
│   ├── eda/                     # EDA figures
│   ├── training/                 # metrics, confusion matrix
│   └── ablation/                  # ablation results
├── serving/
│   ├── app.py                    # Flask API
│   ├── build_demo_cache.py        # caches demo windows from MaFaulDa dataset
│   ├── simulate_stream.py          # replays cached windows into the API (no hardware needed)
│   ├── serial_reader.py             # reads Arduino serial, forwards to the API
│   └── model_store/                  # saved classifier + calibration baseline
├── dashboard/
│   ├── templates/
│   │   └── index.html
│   └── static/
│       └── dashboard.js
├── firmware/
│   └── vibration_monitor.ino    # Arduino Uno firmware
├── requirements.txt
├── Procfile                     # Render deployment
└── README.md
```

## Setup
 
```
pip install -r requirements.txt

pip install flask requests pyserial gunicorn
```
 
## Local Demo
 
No physical rig needed. This replays the real MaFaulDa windows into the API.
 
1. Train the classifier (writes `serving/model_store/classifier.joblib`):
```
   cd src
   python train.py
```

2. Build the demo cache (one-time, ~10-20s):
```
   cd ../serving
   python build_demo_cache.py
```
 
3. Start the API (leave this terminal running):
```
    cd ../serving
   python app.py
```
 
4. In a **separate** terminal, stream the demo data:
```
   cd serving
   python simulate_stream.py
```
 
5. Open `http://127.0.0.1:5000` in a browser. It calibrates on real normal windows, then streams normal followed by imbalance windows -- watch the verdict flip from NORMAL to FAULTY live.

## Usage (hardware mode)

1. Upload `firmware/vibration_monitor.ino` to the Arduino Uno with the Arduino IDE. The Serial Monitor (115200 baud) should print `ADXL345 found at 0x53.` Close the Serial Monitor afterwards so the port is free.
2. Start the API:
   cd serving
   python app.py
3. In a separate terminal, start the serial bridge (replace COM3 with the Arduino's port):
   cd serving
   python serial_reader.py --port COM3
4. Open `http://127.0.0.1:5000`. With the machine running normally, press Start Calibration, wait for about 10 windows, then press Stop Calibration. Live classification starts after calibration.