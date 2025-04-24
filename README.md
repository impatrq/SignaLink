<div align="center">
 
<img src="./Images/Logo.png" alt="Logo proyecto" width="30%"/>

</div>

<div align="center">
 
# Tus gestos hablan, SignaLink traduce

</div>

## 🔗 Acceso a pagina web:
https://signalink2025.com

## 🗓️ Objetivo de Proyecto:
 SignaLink tiene como objetivo principal traducir la lengua de señas para lograr la inclusión de personas sordo-mudas en el ámbito escolar. 
 
 ## 🌎 Visión a futuro
 Otro objetivo a largo plazo es que siga creciendo esta idea en futuras generaciones para poder proveerles un mejor manejo en la vida cotidiana para este tipo de personas, agilizando su vida social en casos de que necesiten ayuda de otras personas o simplemente comunicandose con ellos. 

 ## 💭 ¿De dónde surgió la idea?
 Esta idea surgió en una conversación con la bibliotecaria de nuestra escuela que nos dio a conocer cuan reservadas son estas personas en la sociedad y cuantos desafios deben afrontar a diario. A traves de todo esto planteamos posibles soluciones para facilitarle sus vidas inicialmente en las escuelas y asi empezo toda esta aventura.

## 🖐 Descripción del proyecto:
 Nuestro proyecto va a estar equipado con sensores flex y sensores de giroscopio y acelerómetro que detectarán los movimientos de los dedos. Estos datos se expresan en señales analógicas, por eso es necesario un microcontrolador que convierte estas señales analógicas a digitales (ADC). 
    
 Luego serán transmitidas por BLE (Bluetooth Low Energy) a la Raspberry pi Zero 2W en un módulo separado ubicado en el pecho en forma de riñonera, que así mismo se encargará de procesar estas señales digitales para descubrir que gestos se intentó hacer. Una vez realizado este proceso se enviará en forma de texto al módulo reproductor de audio y con salida al parlante, también ubicado en el módulo de pecho, permitiendo así que las personas que se comunican mediante la lengua de señas puedan expresar su mensaje de forma audible para que cualquier receptor lo comprenda. 
    
 En paralelo, un micrófono direccional se encargará de captar la voz de la persona que quiera comunicarse, esas palabras serán procesadas por el microcontrolador utilizando un modelo de reconocimiento de voz (Vosk -> Offline) y en forma de texto finalmente será mostrado en el LCD.


## 🛠️ Componentes a utilizar:

| Componentes | Links |
|---|---|
| Rasberry Pi Zero 2W | [Raspberry Pi Zero 2 W - Mercado Libre](https://www.mercadolibre.com.ar/raspberry-pi-zero-2-w-64-bits-cortex-a53/p/MLA35340704#polycard_client=search-nordic&searchVariation=MLA35340704&wid=MLA1476733635&position=2&search_layout=grid&type=product&tracking_id=1a2cf9bb-b64c-4103-830b-95cb25e0c878&sid=search) |
| ESP32-C3 Mini | [ESP32-C3 Mini - Mercado Libre](https://articulo.mercadolibre.com.ar/MLA-1933180704-placa-desarrollo-esp32-c3-super-mini-wifi-bluetooth-sgk-_JM#polycard_client=search-nordic&position=18&search_layout=grid&type=item&tracking_id=a6179d80-9c2f-448f-931d-3f6b0744610e&wid=MLA1933180704&sid=search) |
| Sensores Flex | [Sensores Flex - Mercado Libre](https://articulo.mercadolibre.com.ar/MLA-621168012-flex-sensor-45-o-degrees-10-30-kohms-sensor-reflectivo-_JM#polycard_client=search-nordic&position=22&search_layout=stack&type=item&tracking_id=d09a116e-7fa9-4e61-b811-829b67d77fb1&wid=MLA621168012&sid=search) |
| Reproductor      | [DFPlayer Mini - Mercado Libre](https://articulo.mercadolibre.com.ar/MLA-1415876931-modulo-reproductor-audio-hw-247a-musica-dfplayer-mp3-wav-wma-_JM#polycard_client=search-nordic&position=11&search_layout=grid&type=item&tracking_id=ab813d9d-dc9a-42de-8274-9bea4aed94f4&wid=MLA1415876931&sid=search)   |
| Parlante         | [ Mini Parlante- Mercado Libre](https://articulo.mercadolibre.com.ar/MLA-926965993-mini-parlante-mylar-50mm-8-ohms-05w-audio-arduino-nubbeo-_JM#polycard_client=search-nordic&position=8&search_layout=stack&type=item&tracking_id=5afdc171-789b-4858-9908-6e8644b818c2&wid=MLA926965993&sid=search) |
| Microfono        | [ MAX 9814- Mercado Libre](https://www.mercadolibre.com.ar/modulo-microfono-amplificado-arduino-max9814-agc/p/MLA46725329#polycard_client=search-nordic&searchVariation=MLA46725329&wid=MLA2026208850&position=1&search_layout=grid&type=product&tracking_id=300cf118-f249-42f0-8552-88a0b5ccff91&sid=search) |
| Baterias | [ Lipo 3.7V - Mercado Libre](https://articulo.mercadolibre.com.ar/MLA-823943306-bateria-litio-polimero-lipo-37v-1200mah-drones-helicopteros-_JM#polycard_client=search-nordic&position=11&search_layout=stack&type=item&tracking_id=00997a8f-a302-41cd-929e-7f9b96588b73&wid=MLA823943306&sid=search) |
| Sensor Giroscopo y Acelerometro | [MPU6050 - Mercado Libre](https://articulo.mercadolibre.com.ar/MLA-1464073846-acelerometro-giroscopo-mpu6050-6-ejes-gy-521-pic-arduino-_JM#polycard_client=search-nordic&position=4&search_layout=grid&type=item&tracking_id=1592b6fb-67c2-4128-a466-704776d0e915&wid=MLA1464073846&sid=search) |  
|  LCD OLED   | [LCD SSD-1306 - Mercado libre](https://articulo.mercadolibre.com.ar/MLA-832803465-display-oled-091-pulgadas-128x32-ssd1306-i2c-blanco-arduino-_JM#polycard_client=search-nordic&position=6&search_layout=grid&type=item&tracking_id=9fc3ed54-f3c3-4b96-9f93-8db0a3d194bd&wid=MLA832803465&sid=search)  |


## 🧑🏽‍💻 Integrantes

| Integrantes  | Responsabilidades | Tareas Secundarias |
|---|---|---|
| Albornoz Thiago | Gestión de Redes Sociales y Marketing | Desarrollo de Software |
| Erbino Sebastián |  Diseño del Prototipo | Integración de hardware |
| Franco Valentin |  Armado de Hardware | Instalación y conexión de hardware |
| Lesme Franco  | Armado de software | Diseño del prototipo | 
| Poggi Lorenzo | Registro de Seguimiento | Programación del sistema |
| Sarniguette Valentino | Prueba electronica | Armado de circuitos | 

## 🤝 Colaboración:
 Este es el comienzo de un prototipo realizado a base de nuestros conocimientos, si le gustaria formar parte de SignaLink, nos ayudaría su colaboración en:
 - Perfeccionar el código.
 - Mejorar la infraestructura tecnológica.
 - Creación de manuales y realización de pruebas.
 - Apoyo financiero para la investigación y el desarrollo.
 
## 📱 Contactanos: 

 <div style="display: flex; justify-content: space-around;">

<a href="https://mail.google.com/mail/?view=cm&to=signalink2025@gmail.com" target="_blank">
    <img alt="Gmail" src="https://img.shields.io/badge/Gmail-D14836?style=for-the-badge&logo=gmail&logoColor=white" />
</a>
<a href="https://www.instagram.com/signalink_">
    <img alt="Instagram" width="123px" src="https://img.shields.io/badge/Instagram-E4405F?style=for-the-badge&logo=instagram&logoColor=white" />
</a>

## 📱 Contactos personales



### 1. Albornoz Thiago
<a href="https://mail.google.com/mail/?view=cm&to=albornozthiagoagustin@gmail.com" target="_blank">
    <img alt="Gmail" src="https://img.shields.io/badge/Gmail-D14836?style=for-the-badge&logo=gmail&logoColor=white" />
</a>
<a href="https://www.linkedin.com/in/thiago-agustin-albornoz-6956b135b">
    <img alt="LinkedIn" src="https://img.shields.io/badge/LinkedIn-0A66C2?style=for-the-badge&logo=linkedin&logoColor=white" />
</a>


### 2. Erbino Sebastian
<a href="https://mail.google.com/mail/?view=cm&to=serbino25@gmail.com" target="_blank">
    <img alt="Gmail" src="https://img.shields.io/badge/Gmail-D14836?style=for-the-badge&logo=gmail&logoColor=white" />
</a>
<a href="https://www.linkedin.com/in/sebasti%C3%A1n-erbino-25b9792ab?trk=contact-info">
    <img alt="LinkedIn" src="https://img.shields.io/badge/LinkedIn-0A66C2?style=for-the-badge&logo=linkedin&logoColor=white" />
</a>


### 3. Franco Valentin
<a href="https://mail.google.com/mail/?view=cm&to=valentinfranco2506@gmail.com" target="_blank">
    <img alt="Gmail" src="https://img.shields.io/badge/Gmail-D14836?style=for-the-badge&logo=gmail&logoColor=white" />
</a>
<a href="https://www.linkedin.com/in/valentin-franco-174587357?utm_source=share&utm_campaign=share_via&utm_content=profile&utm_medium=ios_app ">
   <img alt="LinkedIn" src="https://img.shields.io/badge/LinkedIn-0A66C2?style=for-the-badge&logo=linkedin&logoColor=white" />
</a>


### 4. Lesme Franco
<a href="https://mail.google.com/mail/?view=cm&to=franco.lesme2006@gmail.com" target="_blank">
    <img alt="Gmail" src="https://img.shields.io/badge/Gmail-D14836?style=for-the-badge&logo=gmail&logoColor=white" />
</a>
<a href="https://www.linkedin.com/in/franco-lesme-25bb4b259/us">
    <img alt="LinkedIn" src="https://img.shields.io/badge/LinkedIn-0A66C2?style=for-the-badge&logo=linkedin&logoColor=white" />
</a>


### 5. Poggi Lorenzo
</a><a href="https://mail.google.com/mail/?view=cm&to=lorenzo.poggijanin@gmail.com" target="_blank">
    <img alt="Gmail" src="https://img.shields.io/badge/Gmail-D14836?style=for-the-badge&logo=gmail&logoColor=white" />
</a>
<a href="https://www.linkedin.com/in/lorenzo-poggi-6b9b5a357?utm_source=share&utm_campaign=share_via&utm_content=profile&utm_medium=ios_app">
    <img alt="LinkedIn" src="https://img.shields.io/badge/LinkedIn-0A66C2?style=for-the-badge&logo=linkedin&logoColor=white" />
</a>


### 6. Sarniguette Valentino
<a href="https://mail.google.com/mail/?view=cm&to=valentinosarniguette@gmail.com" target="_blank">
    <img alt="Gmail" src="https://img.shields.io/badge/Gmail-D14836?style=for-the-badge&logo=gmail&logoColor=white" />
</a>
<a href="https://www.linkedin.com/in/valentino-sarniguette-156175354/">
    <img alt="LinkedIn" src="https://img.shields.io/badge/LinkedIn-0A66C2?style=for-the-badge&logo=linkedin&logoColor=white" />
</a>

-----

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg) ![Status](https://img.shields.io/badge/status-active-brightgreen)
