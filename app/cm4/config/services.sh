#!/bin/bash

# Activar Bluetooth
sleep 2
bluetoothctl << EOF
power on
agent NoInputNoOutput
default-agent
discoverable on
pairable on
EOF

# Activar entorno virtual
source /home/signalink/SignaLink/app/cm4/venv/bin/activate

# Iniciar el reconocimiento de voz
cd /home/signalink/SignaLink/app/cm4/code/microphone
python3 recognizer.py &

# Iniciar el broker MQTT con IA
cd /home/signalink/SignaLink/app/cm4/code/mqtt_broker
python3 mqtt_tflite.py
