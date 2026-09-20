# yslither (Android Vulkan Slither Client & Survival Bot)

**yslither** es un cliente nativo de alto rendimiento para Slither.io para Android y Linux, basado en el motor C/Vulkan de Vlither, libre de anuncios, sin límites de tiempo y con un algoritmo de bot completamente reconstruido para **supervivencia y juego defensivo**.

---

## Características Principales

* **Sin Anuncios ni Bloqueos:** Acceso directo sin temporizadores de 24 horas ni publicidad de terceros.
* **Algoritmo de Bot "Supervivencia Extrema":**
  * **Radar de 32 Sectores:** Mayor resolución angular (11.25° por sector frente a los 22.5° originales) para detectar corredores estrechos entre cuerpos.
  * **Evasión Tangencial Suave:** Elimina los giros bruscos de 180° que provocan que la serpiente colisione contra su propio cuello o cuerpo.
  * **Puntuación de Comida con Matriz de Riesgo:** Penaliza u omite comida que esté dentro de la zona de peligro o cerca de cabezas de serpientes enemigas más grandes.
  * **Auto-Enrollamiento Preventivo:** Al superar cierta longitud o al detectar saturación de amenazas, la serpiente se enrolla automáticamente sobre su propio cuerpo para proteger la cabeza.
  * **Gestión de Turbo de Emergencia:** Ahorro estricto de masa; el turbo solo se enciende si una cabeza enemiga acelera en trayectoria de impacto directo.
* **Controles Táctiles Nativos para Android:**
  * **Joystick Virtual Flotante:** Control analógico dinámico que aparece donde coloques el pulgar izquierdo.
  * **Modo Puntero Directo:** La serpiente sigue la posición exacta donde tocas en la pantalla.
  * **Botón Turbo Táctil:** Botón ergonómico en la esquina inferior derecha.
* **Integración Continua con GitHub Actions:**
  * Cada `push` a la rama `main` compila automáticamente el APK en la nube de GitHub usando el Android NDK y CMake.
  * El archivo `.apk` compilado queda disponible para descarga en la pestaña **Actions > Artifacts**.
  * Si creas un tag (`v1.0.0`), GitHub publica automáticamente la Release con el APK adjunto.

---

## Estructura del Proyecto

```
yslither/
├── .github/workflows/
│   └── build-apk.yml           # Flujo de CI para compilar el APK en GitHub Actions
├── app/
│   ├── src/
│   │   ├── android_main.c      # Punto de entrada NativeActivity para Android
│   │   ├── main.c              # Punto de entrada de escritorio
│   │   ├── game/
│   │   │   ├── sbot.c / .h     # Motor del bot defensivo de 32 sectores
│   │   │   ├── touch_input.c   # Gestor de joystick y controles táctiles
│   │   │   └── ...             # Lógica del juego
│   │   ├── network/            # WebSockets y protocolo de red (Mongoose)
│   │   └── ui/                 # Interfaz con Dear ImGui
│   ├── build.gradle            # Configuración de Gradle para Android
│   └── src/main/
│       └── AndroidManifest.xml # Manifiesto nativo con Vulkan y permisos
├── thermite/                   # Motor ligero en C con backend Vulkan
├── CMakeLists.txt              # Script de construcción multiplataforma (Android NDK + PC)
└── gradlew                     # Wrapper de Gradle
```

---

## Cómo subir el repositorio a GitHub y generar tu APK

1. **Crea un repositorio vacío en tu cuenta de GitHub** (por ejemplo, con el nombre `yslither`).
2. **Conecta tu repositorio local con GitHub:**
   ```bash
   cd /root/.gemini/antigravity-cli/scratch/yslither
   git remote add origin https://github.com/TU_USUARIO/yslither.git
   git add .
   git commit -m "Initial commit: yslither Android Vulkan port & survival bot"
   git push -u origin main
   ```
3. **Descarga el APK:**
   * Entra a tu repositorio en GitHub y ve a la pestaña **Actions**.
   * Verás el flujo **"Build yslither Android APK"** ejecutándose.
   * Una vez completado en verde (unos 2-3 minutos), haz clic en el workflow y en la sección **Artifacts** descarga `yslither-debug-apk.zip`.
   * Descomprime e instala el archivo `.apk` directamente en tu teléfono Android.

---

## Compilación Local en Android Studio

Si prefieres compilar localmente en tu computadora:
1. Abre **Android Studio**.
2. Selecciona **Open** y escoge la carpeta `yslither/`.
3. Conecta tu celular por USB (con depuración USB activada) y presiona **Run** (`Shift + F10`).
4. Android Studio compilará el código C con CMake/NDK y lo instalará directamente en tu dispositivo.

---

## Créditos
* Basado en el cliente original [for-loop9/vlither](https://github.com/for-loop9/vlither) por **ignite**.
* Bot de supervivencia y adaptación Android desarrollado para **yslither**.
