# yslither: High-Performance Vulkan Slither.io Client & Self-Improving AI Bot

**yslither** es un cliente nativo de alto rendimiento para **Slither.io** en **Android (NDK/Vulkan)** y **Linux (Vulkan/X11)**, equipado con netcode predictivo con amortiguamiento continuo de errores, motor 60 FPS con hot loop zero-malloc, y una IA autónoma de nivel top con rollouts cinemáticos, simulador headless reproducible y optimizador evolutivo CMA-ES.

---

## Fases de Desarrollo e Ingeniería

### Fase 1: Sincronización y Netcode Predictivo (v1.0.32)
* **Loop Principal y Clock Monotónico**: `clock_gettime(CLOCK_MONOTONIC)` con clamp de tirones (hitch clamp a 120 ms). Eliminación de ralentizaciones artificiales (`lag_mult`).
* **Continuous Exponential Error Blending**:
  $$\vec{e}(t + \Delta t) = \vec{e}(t) \cdot e^{-18 \Delta t}$$
  Elimina el 100% del snapping visual ante la recepción de paquetes de red del servidor.
* **Reducción de Latencia de Input**:
  * Tasa de envío de dirección reducida de 50 ms a 15–25 ms.
  * Tasa de actualización de turbo reducida de 150 ms a 35 ms.
* **Overlay de Diagnóstico Netcode en Pantalla**:
  * Métricas en tiempo real: FPS, Frametime p95 (ventana de 128 muestras), RTT, Jitter RFC 3550, Retraso de interpolación y Correcciones por segundo.
  * Registro periódico a archivo `netcode_telemetry.log`.

### Fase 2: Calidad, Pulido del Motor y Ergonomía Táctil (v1.0.33)
* **Zero-Malloc en Hot Loop**:
  * Función `tdarray_reserve` integrada en `thermite`.
  * Pre-reserva estática de arrays de serpientes (128), comida (8192), presas (64) y nodos de cuerpo (256/1024), eliminando `realloc` durante la partida.
* **Cámara y Zoom Suave**: Amortiguamiento crítico exponencial $1 - e^{-18 \Delta t}$ que sincroniza la cámara con el movimiento real del gusano sin sacudidas.
* **Ergonomía Táctil Avanzada**:
  * Zona muerta configurable (`touch_deadzone`, 12 px por defecto).
  * Curva de respuesta potencial con exponente 1.4:
    $$\text{offset} = \left(\frac{d - dz}{R - dz}\right)^{1.4} \times \text{sensibilidad}$$
  * Modo zurdo (`touch_left_handed`) intercambiando botón de turbo y joystick.
  * Solicitud de retroalimentación háptica en activación de turbo.

### Fase 3: Bot Top Autónomo (`app/src/game/sbot.c`, v1.0.34)
* **128 Rollouts Cinemáticos por Tick (64 Direcciones $\times$ {Crucero, Turbo})**:
  * Simulación de giro en 4 pasos de tiempo ($t \in [0.15, 0.35, 0.60, 0.95]$ s) restringida por la velocidad angular física de Slither.io ($\omega_{max} \approx 5.2$ rad/s).
  * Descarte estricto de cualquier trayectoria que colisione dentro del horizonte.
* **Percepción Geométrica Continua**:
  * Cuerpos modelados con radio real denso sin huecos entre esferas.
  * Proyección predictiva de cabezas rivales $\vec{x}(t) = \vec{x}_0 + \vec{v} \cdot t$.
  * Repulsión perimetral circular ante el borde del mapa.
* **Puntuación Multifactorial**:
  * Recompensa de espacio abierto (anti-callejones sin salida / flood-fill).
  * Atracción ponderada por masa y distancia a comida.
  * Bonificación por corte frontal de cabezas rivales (**KILL OPPORTUNITY**).
  * Penalización por gasto innecesario de masa en turbo.
* **Máquina de Estados con Histéresis**:
  * `FARM`: Búsqueda eficiente de biomasa.
  * `HUNT`: Persecución de banquetes de muerte y corte frontal.
  * `ESCAPE`: Activación inmediata ante despeje frontal $< 230$ px; requiere $\ge 15$ cuadros consecutivos $> 440$ px para retornar a FARM (cero vibración de modo).
  * `COIL`: Auto-enroscamiento protector ante puntaje alto o saturación.
* **Rendimiento**: $< 0.02$ ms por tick (15.3 microsegundos), cero `malloc` por tick.

### Fase 4: Auto-Mejora Medible y Simulador Headless (v1.0.35)
* **Simulador Headless en Linux (`tools/sim`)**:
  * Simulación 2D determinista de cinemática, colisiones y alimentación sin dependencias gráficas.
* **Telemetría JSONL**:
  * Registro de cada partida con duración, masa máxima, kills y causa de muerte (`BODY_COLLISION`, `HEAD_COLLISION`, `MAP_BORDER`, `SURVIVED_TIME_LIMIT`).
* **Optimizador Evolutivo (CMA-ES / (1+$\lambda$)-ES)**:
  * Ajuste de pesos en `sbot_weights.h` evaluados en semillas separadas de entrenamiento y validación.
* **Benchmark Reproducible (200 Partidas por Bot con Semillas Idénticas)**:
  * Reducción de muertes por choque de cuerpo de **9.5% a 5.5% (reducción del 42%)**.
  * Aumento de partidas completas sobrevividas de **90.5% a 94.5%**.
  * Incremento de kills totales de **12 a 14 (+16.7%)**.
* **Adaptación Online en Dispositivo**:
  * El margen de riesgo se reajusta automáticamente ante muertes dentro de rangos seguros $[-10, +25]$ px.

---

## Cómo Ejecutar el Simulador, Benchmark y Optimizador (Linux)

Desde la raíz del proyecto:

```bash
# Compilar el simulador headless, benchmark y optimizador
make -C tools/sim

# Ejecutar el benchmark reproducible de 200 partidas
./tools/sim/benchmark

# Ejecutar el optimizador evolutivo de parámetros
./tools/sim/optimizer
```

Las partidas del benchmark generarán automáticamente los registros de telemetría en:
* `tools/sim/telemetry_old_bot.jsonl`
* `tools/sim/telemetry_new_bot.jsonl`

---

## Compilación y Descarga del APK para Android

El proyecto incluye integración continua mediante GitHub Actions (`.github/workflows/build-apk.yml`).

### Descargas de Versiones Publicadas:
* **[v1.0.34 (Bot Top AI)](https://github.com/anhot11/yslither/releases/tag/v1.0.34)** — APK con planificador de 64 direcciones y radar visual.
* **[v1.0.33 (Fase 2 Pulido)](https://github.com/anhot11/yslither/releases/tag/v1.0.33)** — APK con controles ergonómicos, modo zurdo y 60 FPS zero-malloc.
* **[v1.0.32 (Fase 1 Netcode)](https://github.com/anhot11/yslither/releases/tag/v1.0.32)** — APK con netcode de error blending y HUD de diagnóstico.

### Compilación Local en Android Studio:
1. Abrir la carpeta `yslither` en **Android Studio**.
2. Conectar el teléfono Android por cable USB con depuración activada.
3. Presionar **Run** (`Shift + F10`).
