# SancaBotBLE — Control de LED embarcado por BLE

Proyecto para controlar el LED embarcado del SancaBot mediante comunicación Bluetooth Low Energy (BLE).

El proyecto está desarrollado para la placa ESP32-C3 SuperMini usando VSCode, PlatformIO y Arduino Framework.

## Objetivo

Controlar el LED embarcado de la placa ESP32-C3 SuperMini del SancaBot mediante comandos enviados por BLE.

Este proyecto sirve como paso intermedio entre una prueba básica de parpadeo del LED y proyectos más avanzados de control del SancaBot por Bluetooth.

## Hardware utilizado

* SancaBot
* ESP32-C3 SuperMini
* LED embarcado de la placa
* Cable USB-C para programación
* Celular o computador con cliente BLE

## Software utilizado

* VSCode
* PlatformIO
* Arduino Framework
* nRF Connect u otra aplicación compatible con BLE

## Pin utilizado

El LED embarcado se controla desde:

`GPIO 8`

## Nombre BLE del dispositivo

Al cargar el programa, la placa se anuncia por BLE con el nombre:

`SancaBot_LED`

## Servicio y característica BLE

Servicio BLE:

`4fafc201-1fb5-459e-8fcc-c5c9c331914b`

Característica BLE:

`beb5483e-36e1-4688-b7f5-ea07361b26a8`

La característica permite lectura, escritura, escritura sin respuesta y notificaciones.

## Comandos BLE aceptados

Los comandos se envían como texto a la característica BLE.

* `ON` enciende el LED.
* `OFF` apaga el LED.
* `1` enciende el LED.
* `0` apaga el LED.
* `T` alterna el estado del LED.

## Respuestas BLE

Si las notificaciones están activadas, la placa puede responder con:

* `LED:ON`
* `LED:OFF`

## Prueba con nRF Connect

1. Grabar el firmware en la ESP32-C3.
2. Abrir nRF Connect en el celular.
3. Buscar el dispositivo `SancaBot_LED`.
4. Conectarse al dispositivo.
5. Entrar al servicio BLE del proyecto.
6. Seleccionar la característica BLE.
7. Activar notificaciones si se desea ver la respuesta.
8. Escribir comandos como texto: `ON`, `OFF`, `1`, `0` o `T`.

## Compilación

Desde la carpeta del proyecto:

`cd SancaBotBLE`

Compilar:

`platformio run`

## Carga en la placa

Para grabar el programa en la ESP32-C3:

`platformio run --target upload`

## Monitor serial

Para observar los mensajes del programa:

`platformio device monitor -b 115200`

En el monitor serial se muestran mensajes como:

* `[BOOT] SancaBot LED BLE`
* `BLE listo.`
* `Nombre BLE: SancaBot_LED`
* `Comando recibido: ON`
* `LED encendido`

## Observación sobre lógica invertida

En algunas placas ESP32-C3 el LED embarcado trabaja con lógica invertida. Esto significa que `LOW` puede encender el LED y `HIGH` puede apagarlo.

Si el LED funciona al revés, se debe modificar la función `applyLedState()` en `src/main.cpp`, intercambiando `HIGH` y `LOW`.

## Estado del proyecto

Proyecto funcional.

Este ejemplo permite verificar la comunicación BLE básica antes de avanzar hacia el control de servomotores, sensores y comunicación con interfaces web más completas.
