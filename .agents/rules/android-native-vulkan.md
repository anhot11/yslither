---
description: Reglas y salvaguardas para desarrollo nativo Android NDK con Vulkan y NativeActivity.
globs: ["**/*.c", "**/*.cpp", "**/*.h", "**/CMakeLists.txt", "**/*.gradle"]
---

# Guía de Desarrollo Nativo Android (NDK & Vulkan)

Al desarrollar o portar proyectos de escritorio en C/C++ hacia Android mediante `NativeActivity`:

1. **Gestión de Assets y Sistema de Archivos:**
   - Los archivos dentro del APK (`assets/`) no son accesibles vía `fopen()` o rutas relativas directas de Linux sin empaquetarlos en `src/main/assets/` y extraerlos a `internalDataPath` en el primer arranque.
   - En `android_main()`, invocar `chdir(state->activity->internalDataPath)` para asegurar que las rutas relativas (`fopen("config.dat", ...)`) tengan permisos de lectura y escritura.
   - Nunca utilizar llamadas fatales `exit(-1)` ante fallos de persistencia de configuración; registrar advertencia y continuar con valores en memoria.

2. **Límites de Pila (Stack Size) en Hilos Nativos:**
   - El hilo principal de Android y los hilos creados por `pthread_create` tienen límites de pila reducidos (frecuentemente 1 MB).
   - **Nunca declarar estructuras gigantes (> 256 KB) como variables locales** (por ejemplo `(game_data){...}` de 1.7 MB), pues provoca un desbordamiento inmediato de pila (`SIGSEGV` en el prólogo de la función).
   - Declarar tablas y paletas constantes como `static const` para ubicarlas en `.rodata`, y asignar memoria dinámica en el heap (`malloc` / `memset`).

3. **Vulkan en GPUs Móviles (Mali, Adreno, PowerVR):**
   - En la selección de GPU física (`vkEnumeratePhysicalDevices`), **nunca exigir** `VK_PRESENT_MODE_IMMEDIATE_KHR` como condición obligatoria; la especificación de Vulkan en Android solo garantiza `VK_PRESENT_MODE_FIFO_KHR`.
   - Para la swapchain, verificar `capabilities.supportedCompositeAlpha` y utilizar `VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR` si `OPAQUE` no está soportado.
   - **Rotación y Orientación de Pantalla:** En teléfonos con pantalla nativa vertical (`portrait`) ejecutando en horizontal (`sensorLandscape`), `currentTransform` será `ROTATE_90_BIT_KHR`. A menos que se roten manualmente los shaders y las matrices de proyección, debe configurarse `.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR` para que el compositor del sistema operativo (SurfaceFlinger) muestre la imagen derecha. Además, debe asegurarse que las dimensiones del `swapchain_extent` coincidan con el ancho y alto horizontal.
   - Para compatibilidad con arquitecturas de 32 bits (`armeabi-v7a`), utilizar siempre `VK_NULL_HANDLE` en vez de `NULL` para identificadores no despachables de Vulkan.

4. **Dear ImGui en Android:**
   - Proveer dimensiones iniciales a `io->DisplaySize` y un `io->DeltaTime` positivo antes de la primera llamada a `ImGui::NewFrame()` para prevenir fallos por aserción (`IM_ASSERT`).
   - Redirigir eventos táctiles `AMotionEvent` a `io->MousePos` y `io->MouseDown[0]` para permitir la interacción en menús táctiles.
