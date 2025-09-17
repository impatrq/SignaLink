#!/bin/bash

# Cambiar a WiFi con internet
sudo systemctl stop hostapd
sudo systemctl stop dnsmasq
sudo systemctl restart wpa_supplicant
sudo systemctl restart NetworkManager

# Esperar conexión
sleep 10

# Hacer lo que quieras (ejemplo: update y git push)
cd /home/fr4nco/Documents/SignaLink/rpizero2W && git add . && git commit -m "feat: microfono asociado con MQTT y readme de mqtt 
Co-authored-by: Thiago Agustin Albornoz <tatotatuaje099@gmail.com>
Co-authored-by: Valentin Franco <valentinfranco2506@gmail.com>" && git push

# Volver a modo AP
sudo systemctl stop wpa_supplicant
sudo systemctl stop NetworkManager
sudo systemctl start hostapd
sudo systemctl start dnsmasq
