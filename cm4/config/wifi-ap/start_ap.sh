#!/bin/bash

# Deter servicios que pueden interferir
sudo systemctl stop wpa_supplicant
sudo systemctl stop NetworkManager

# Desabilitar servicios para el inicio
sudo systemctl disable wpa_supplicant
sudo systemctl disable NetworkManager

# Incializar servicios de red local
sudo systemctl start hostapd
sudo systemctl start dnsmasq
