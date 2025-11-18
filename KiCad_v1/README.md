# Carpeta de Trabajo - Diseño de Placas 

Utilizamos **KiCad** para el diseño de circuitos impresos (PCB). Llevamos con esta aplicacion la creacion de dos PCBs:

## Módulo Muñeca

El diseño de la placa está optimizado en un formato compacto de 30 mm por 50 mm, lo que permite reducir al máximo su tamaño sin afectar el rendimiento. Su objetivo principal es garantizar un sensado preciso de los gestos de la mano, combinando comodidad de uso con eficiencia en la interpretación de movimientos. 

El módulo integra un ESP32-C3 Super Mini como microcontrolador central, acompañado por el chip MPU6050 para la detección de aceleración y giro, una pantalla OLED SSD1306 para la visualización de información en tiempo real, sensores flexibles que registran la flexión de los dedos y un módulo de carga USB que facilita la autonomía energética y la recarga de Batería Lipo. Gracias a esta integración, el sistema se presenta como una solución ligera, compacta y confiable para la captura e interpretación de gestos manuales.

<h3>Mano Izquierda</h3>

- Diseño 3D:
<div align="center">
<img src="Modulo_Muñeca/Mano_izquierda/Diseño_3D.png" alt="" width= "70%" height= "50%"/>
</div>

- Esquemático
<div align="center">
<img src="Modulo_Muñeca/Mano_izquierda/Esquematico.png" alt="" width= "70%" height= "50%"/>
</div>

- Diseño de placa sin plano de masa
<div align="center">
<img src="Modulo_Muñeca/Mano_izquierda/Editor_de_placa_sin_plano_de_masa.png" alt="" width= "70%" height= "50%"/>
</div>

- Diseño de placa con plano de masa:
<div align="center">
<img src="Modulo_Muñeca/Mano_izquierda/Editor_de_placa.png" alt="" width= "70%" height= "50%"/>
</div>


<h3>Mano Derecha</h3>

- Diseño 3D:
<div align="center">
<img src="Modulo_Muñeca/Mano_derecha/Diseño_3D.png" alt="" width= "70%" height= "50%"/>
</div>

- Esquemático
<div align="center">
<img src="Modulo_Muñeca/Mano_derecha/Esquematico.png" alt="" width= "70%" height= "50%"/>
</div>

- Diseño de placa sin plano de masa
<div align="center">
<img src="Modulo_Muñeca/Mano_derecha/Editor_de_placa_sin_plano_de_masa.png" alt="" width= "70%" height= "50%"/>
</div>

- Diseño de placa con plano de masa:
<div align="center">
<img src="Modulo_Muñeca/Mano_derecha/Editor_de_placa.png" alt="" width= "70%" height= "50%"/>
</div>




## Modulo Pecho: 

El módulo del pecho constituye el núcleo de procesamiento de SignaLink. En su diseño se integra una Raspberry Pi Compute Module 4 (RCM4), que actúa como el procesador principal encargado de coordinar y analizar la información proveniente de los demás periféricos. Esta capacidad de cómputo permite realizar el filtrado de datos en tiempo real y gestionar la comunicación con el resto del sistema a través del protocolo MQTT, asegurando una transmisión eficiente y confiable.

Además de la RCM4, la placa incorpora un DAC con amplificador PAM junto con los conectores para parlante, lo que posibilita la conversión de gestos a audio de manera clara y directa. Gracias a esta integración, el sistema no solo traduce los movimientos capturados en datos procesables, sino que también ofrece la salida de voz que convierte las señas en un medio de comunicación audible.

De esta forma, el módulo del pecho se consolida como el cerebro y la voz de SignaLink, combinando procesamiento, conectividad y salida de audio en una sola unidad compacta y eficiente.

- Diseño 3D:
<div align="center">
<img src="Modulo_Pecho/Diseño_3D_derecho.png" alt="" width= "70%" height= "50%"/>
</div>

- Diseño 3D:
<div align="center">
<img src="Modulo_Pecho/Diseño_3D_reves.png" alt="" width= "70%" height= "50%"/>
</div>

- Esquemático
<div align="center">
<img src="Modulo_Pecho/Esquematico.png" alt="" width= "70%" height= "50%"/>
</div>

- Diseño de placa sin plano de masa
<div align="center">
<img src="Modulo_Pecho/Editor_de_placa.png" alt="" width= "70%" height= "50%"/>
</div>