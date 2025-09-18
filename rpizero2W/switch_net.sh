#!/bin/bash

# Cambiar a WiFi con internet
sudo systemctl stop hostapd
sudo systemctl stop dnsmasq
sudo systemctl restart wpa_supplicant
sudo systemctl restart NetworkManager

# Esperar conexión
sleep 10

# Variables 
Path = ""
Commands = ""

# Input
read -p "Ingresar PATH:" Path
read -p "Ingresar comandos a ejecutar:" Commands
read -p "Continue? (Y/N)" confirm && [[ $confirm == [yY] || $confirm == [yY][eE][sS] ]] || exit 1

# Hacer lo que quieras (ejemplo: update y git push)
cd $PATH && $Commands

# Volver a modo AP
sudo systemctl stop wpa_supplicant
sudo systemctl stop NetworkManager
sudo systemctl start hostapd
sudo systemctl start dnsmasq
