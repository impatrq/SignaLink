#!/bin/bash

echo "🛠️  Instalando dependencias del sistema..."
sudo apt update
sudo apt install -y \
    python3-dev \
    python3-pip \
    python3-venv \
    portaudio19-dev \
    libffi-dev \
    libssl-dev \
    libi2c-dev \
    libatlas-base-dev \
    libbluetooth-dev \
    bluetooth \
    bluez \
    ffmpeg \
    build-essential

echo "📦 Instalando dependencias de Python..."
pip install --upgrade pip setuptools wheel

if [ -f requirements.txt ]; then
    pip install -r requirements.txt
else
    echo "❌ No se encontró el archivo requirements.txt"
    exit 1
fi

echo "✅ Todo listo. Para empezar a trabajar, corré:"
echo "source signalink/bin/activate"
