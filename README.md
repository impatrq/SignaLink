# SignaLink - Tus manos hablan, SignaLink escucha
**Descripción del proyecto:**
 Nuestro proyecto va a estar equipado con sensores flex y sensores de giroscopio y acelerómetro que detectarán los movimientos de los dedos. Estos datos se expresan en señales analógicas, por eso es necesario un microcontrolador que convierte estas señales analógicas a digitales (ADC). Luego serán transmitidas por BLE (Bluetooth Low Energy) a la Raspberry pi Zero 2W en un módulo separado ubicado en el pecho en forma de riñonera, que así mismo se encargará de procesar estas señales digitales para descubrir que seña se intentó hacer. Una vez realizado este proceso se enviará en forma de texto al módulo reproductor de audio y con salida al parlante también ubicado en el módulo de pecho, permitiendo así que las personas que se comunican mediante la lengua de señas puedan expresar su mensaje de forma audible para que cualquier receptor lo comprenda. 
En paralelo, un micrófono direccional se encargará de captar la voz de la persona que quiera comunicarse, esas palabras serán procesadas por la Raspberry Pi Zero 2W utilizando un modelo de reconocimiento de voz (Vosk -> Offline) y en forma de texto finalmente será mostrado en el LCD
----------------------------
**Nuestro objetivo**
SignaLink tiene como objetivo traducir la lengua de señas a voz y a texto para lograr la inclusión de personas sordo-mudas en el ámbito escolar. 
----------------------------




