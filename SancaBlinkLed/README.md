# SancaBlinkLed

Proyecto mínimo para probar el LED embarcado del SancaBot usando ESP32-C3 SuperMini, VSCode, PlatformIO y Arduino Framework.

Este proyecto hace parpadear el LED integrado de la placa, conectado al GPIO 8.

## Objetivo

Verificar el funcionamiento básico de la placa ESP32-C3 SuperMini del SancaBot mediante una prueba simple de salida digital.

El programa enciende y apaga el LED embarcado cada 500 ms.

## Hardware utilizado

* SancaBot
* ESP32-C3 SuperMini
* LED embarcado de la placa
* Cable USB-C para programación

## Software utilizado

* VSCode
* PlatformIO
* Arduino Framework

## Pin utilizado

El LED embarcado se controla desde:

`GPIO 8`

## Funcionamiento del programa

El programa realiza continuamente la siguiente secuencia:

1. Configura el GPIO 8 como salida.
2. Enciende el LED.
3. Espera 500 ms.
4. Apaga el LED.
5. Espera 500 ms.
6. Repite indefinidamente.

## Archivo principal

El código principal se encuentra en:

`src/main.cpp`

## Compilación

Desde la carpeta del proyecto:

`cd SancaBlinkLed`

Compilar:

`platformio run`

## Carga en la placa

Para grabar el programa en la ESP32-C3:

`platformio run --target upload`

## Monitor serial

Este proyecto no imprime mensajes por el monitor serial, pero se puede abrir con:

`platformio device monitor -b 115200`

## Observación sobre el LED embarcado

En algunas placas ESP32-C3 el LED embarcado puede trabajar con lógica invertida. Es decir, puede ocurrir que `LOW` encienda el LED y `HIGH` lo apague.

Si el comportamiento observado parece invertido, basta con intercambiar `HIGH` y `LOW` en el archivo `src/main.cpp`.

## Estado del proyecto

Proyecto funcional.

Este ejemplo sirve como primera prueba antes de avanzar hacia proyectos con Bluetooth Low Energy, control de servomotores y comunicación con aplicaciones web.
