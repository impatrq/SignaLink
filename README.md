# SignaLink - Tus manos hablan, SignaLink escucha.
# Objetivo de Proyecto:
SignaLink tiene como objetivo principal traducir la lengua de señas para lograr la inclusión de personas sordo-mudas en el ámbito escolar.
-------------------------------
# Descripción del proyecto:
Nuestro proyecto va a estar equipado con sensores flex y sensores de giroscopio y acelerómetro que detectarán los movimientos de los dedos. Estos datos se expresan en señales analógicas, por eso es necesario un microcontrolador que convierte estas señales analógicas a digitales (ADC). Luego serán transmitidas por BLE (Bluetooth Low Energy) a la Raspberry pi Zero 2W en un módulo separado ubicado en el pecho en forma de riñonera, que así mismo se encargará de procesar estas señales digitales para descubrir que seña se intentó hacer. Una vez realizado este proceso se enviará en forma de texto al módulo reproductor de audio y con salida al parlante, también ubicado en el módulo de pecho, permitiendo así que las personas que se comunican mediante la lengua de señas puedan expresar su mensaje de forma audible para que cualquier receptor lo comprenda. 
En paralelo, un micrófono direccional se encargará de captar la voz de la persona que quiera comunicarse, esas palabras serán procesadas por el microcontrolador utilizando un modelo de reconocimiento de voz (Vosk -> Offline) y en forma de texto finalmente será mostrado en el LCD.
----------------------------
# Componentes a utilizar:

| Componentes | Imagen |
|---|---|
| Rasberry Pi Zero 2W | [Rasberry Pi Zero 2 W - Mercado Libre](https://github.com/carenalgas/popochiu/releases/download/v2.0.0/popochiu-v2.0.0.zip) |
| ESP32-C3 Mini | [ESP32-C3 Mini - Mercado Libre](https://articulo.mercadolibre.com.ar/MLA-1933180704-placa-desarrollo-esp32-c3-super-mini-wifi-bluetooth-sgk-_JM#polycard_client=search-nordic&position=18&search_layout=grid&type=item&tracking_id=a6179d80-9c2f-448f-931d-3f6b0744610e&wid=MLA1933180704&sid=search) |
| Sensores Flex | [Sensores Flex - Mercado Libre](https://articulo.mercadolibre.com.ar/MLA-621168012-flex-sensor-45-o-degrees-10-30-kohms-sensor-reflectivo-_JM#polycard_client=search-nordic&position=22&search_layout=stack&type=item&tracking_id=d09a116e-7fa9-4e61-b811-829b67d77fb1&wid=MLA621168012&sid=search) |
| Reproductor      | [DFPlayer Mini](https://listado.mercadolibre.com.ar/dfplayer-mini#D[A:DFPLAYER%20MINI])   |
| Parlante         | [Parlante]() |
| Microfono        | [Microfono](https://listado.mercadolibre.com.ar/max9814#D[A:MAX9814]) |
| Baterias Lipo 3.7V | [Baterias Lipo]() |