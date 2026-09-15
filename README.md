vibration-monitoring/
├── data/
│   ├── raw/              # downloaded dataset goes here (gitignored)
│   └── processed/        # extracted feature vectors, cached
├── notebooks/
│   └── eda.ipynb         # dataset exploration
├── src/
│   ├── features.py       # RMS, dominant freq, spectral energy, kurtosis — shared by training AND live inference
│   ├── calibration.py    # the novelty: per-deployment baseline + z-score normalization
│   ├── dataset.py        # dataset loader → windows + labels
│   ├── model.py           # classifier definition, save/load
│   ├── train.py           # offline training script
│   └── ablation.py        # fixed-baseline vs adaptive-baseline comparison
│
├── results/
│   ├── eda/               # class balance plot, sample waveform/FFT plots, feature distribution plots
│   ├── training/           # confusion matrix, train/test metrics.json, feature importance if any
│   └── ablation/            # fixed-vs-adaptive comparison table (csv) + accuracy/AUC bar chart
├── serving/
│   ├── app.py              # Flask API (Render-deployed)
│   ├── model_store/         # saved model + baseline artifacts
│   └── serial_reader.py      # reads Arduino serial, feeds the API
├── dashboard/
│   ├── templates/index.html
│   └── static/dashboard.js
├── firmware/
│   └── vibration_monitor.ino
└── requirements.txt