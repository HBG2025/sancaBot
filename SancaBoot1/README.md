## Fuente base del proyecto

Este repositorio se apoya en pruebas y códigos desarrollados para el SancaBot usando PlatformIO, VSCode y Arduino Framework.

Parte del trabajo toma como referencia el código suministrado desde AIPES para el SancaBot:

`https://aipes.com.br/?id=45eba5ab&lang=pt`

AIPES permite generar, compartir, compilar y grabar proyectos para sistemas embebidos, incluyendo la placa ESP32-C3 Super Mini. En este repositorio, dicho código se usa como punto de partida para estudiar, adaptar y documentar ejemplos propios relacionados con BLE, control de LED, servomotores, sensores y comunicación con interfaces web.


## Aplicación oficial SancaBot

La aplicación oficial del SancaBot se usa desde:

`https://sancabot.com.br/app`

Para que esa aplicación controle el robot completo, se requiere un firmware compatible con su protocolo BLE. En este repositorio se conserva y adapta el trabajo basado en el código compartido desde AIPES:

`https://aipes.com.br/?id=45eba5ab&lang=pt`

Ese firmware interpreta comandos de movimiento, configuración, modos autónomos y notificaciones de estado. Los ejemplos más simples, como el control del LED por BLE, se usan como pasos intermedios para entender la comunicación BLE antes de trabajar con el control completo del robot.
