# 03 · Arquitectura técnica de Jocker-MX

## 1. Principios
1. **Tiempo real primero**: el hilo de audio nunca bloquea, no reserva memoria y no toma *locks*. Comunicación por colas *lock-free* (SPSC/MPSC).
2. **Un solo núcleo, cuatro plataformas**: C++20 + Qt 6 compartido; sólo cambian los *backends* de audio, video y empaquetado.
3. **IA local por defecto**: todo modelo corre en el equipo (CPU/GPU/NPU) mediante ONNX Runtime, llama.cpp y whisper.cpp; servicios en la nube sólo si el usuario los configura.
4. **Abierto y extensible**: skins en QML, mapeos en JSON + JavaScript, plugins CLAP/VST3/LV2/AU, API de scripting documentada.
5. **Clean room**: se reimplementa el comportamiento; nunca se copian código, recursos ni marcas de VirtualDJ.

## 2. Pila tecnológica
| Capa | Elección | Licencia | Motivo |
|---|---|---|---|
| Lenguaje núcleo | C++20 | — | Rendimiento determinista, ecosistema de audio |
| Construcción | CMake + vcpkg/Conan, Ninja | BSD/MIT | Multiplataforma, reproducible |
| UI | Qt 6.8+ (Qt Quick/QML, Qt RHI) | LGPL-3.0 | Escritorio + Android con el mismo código, GPU nativa (Vulkan/Metal/D3D11-12/GL) |
| E/S de audio | Capa propia `jx::audio` sobre: WASAPI, **ASIO** (Win), CoreAudio (mac), ALSA/**PipeWire**/**JACK** (Linux), **Oboe/AAudio** (Android) | ASIO SDK GPLv3, Oboe Apache-2.0 | Latencia < 5 ms donde el hardware lo permita |
| Decodificación | FFmpeg (compilación LGPL o GPL), libFLAC, libsndfile, minimp3 | LGPL/GPL/BSD | Todos los formatos de audio y video |
| Time-stretch | Rubber Band v3 (alta calidad), SoundTouch (bajo consumo) | GPL-2.0+, LGPL-2.1 | Key lock |
| Análisis | qm-dsp (beats), libKeyFinder (tonalidad), Chromaprint (huella), EBU R128 (libebur128) | GPL-2.0+, GPL-3.0, LGPL, MIT | Calidad probada |
| Inferencia IA | ONNX Runtime (CUDA, DirectML, CoreML, ROCm, XNNPACK, NNAPI/QNN) | MIT | Stems y modelos ligeros |
| LLM / voz | llama.cpp (LLM), whisper.cpp (letras), Piper (TTS) | MIT, MIT, GPL-3.0 | Ejecución local |
| Base de datos | SQLite 3 + FTS5 | Dominio público | Biblioteca de 100k+ pistas |
| Etiquetas | TagLib 2 | LGPL-2.1/MPL-1.1 | Metadatos |
| MIDI/HID | RtMidi, hidapi, Android MIDI API | MIT, BSD | Controladores |
| Timecode | Decodificador derivado de xwax | GPL | DVS |
| Red | Ableton Link, libshout, WebSocket (Qt), mDNS | GPL-2.0+, LGPL | Link, radio, remote |
| Visuales | projectM (presets Milkdrop), shaders propios | LGPL-2.1 | Visualizaciones |
| Plugins | CLAP, VST3 SDK, LV2 (lilv), AudioUnit | MIT, MIT, ISC, Apple | Efectos de terceros |
| Pruebas | Catch2/GoogleTest, Qt Test, Squish-libre (Qt Quick Test) | BSL/BSD | QA |

## 3. Diagrama de módulos
```
┌──────────────────────────────────────────────────────────────────────┐
│                     UI (Qt Quick / QML · Skins)                       │
│  Decks · Mixer · Browser · Pads · Editores · Video · Karaoke · Ajustes │
└───────────────▲───────────────────────────────▲──────────────────────┘
                │ Bus de control (comandos/eventos, lock-free)          │
┌───────────────┴──────────────┐   ┌────────────┴────────────────────────┐
│ jx::control                   │   │ jx::script (JockerScript + JS)     │
│ MIDI/HID · Mapper · Teclado   │──▶│ Verbos · Mapeos · AI Scripting      │
│ Remote (WebSocket) · Link     │   └─────────────────────────────────────┘
└───────────────┬──────────────┘
                ▼
┌──────────────────────────────────────────────────────────────────────┐
│ jx::engine  (hilo de audio en tiempo real)                            │
│  Deck[N] → Stems(5) → Key-lock → FX por deck → Canal mezclador         │
│  Sampler · Micrófono · Line-in · Sandbox bus · Master · Cue · Grabación│
└───────▲───────────────────▲──────────────────────────▲────────────────┘
        │ buffers            │ stems                    │ PCM / video clock
┌───────┴──────┐  ┌──────────┴─────────┐  ┌────────────┴──────────────┐
│ jx::io        │  │ jx::ai             │  │ jx::video                  │
│ Decoders      │  │ Stems (ONNX)       │  │ Decod. HW · Compositor RHI │
│ Audio backends│  │ Beat/Key/Fluid grid│  │ Transiciones · Visuales    │
│ Caché         │  │ Whisper · LLM · TTS│  │ Videoskins · Salidas/RTMP  │
└──────────────┘  └────────────────────┘  └────────────────────────────┘
        ▲                    ▲
┌───────┴────────────────────┴─────────────────────────────────────────┐
│ jx::library  SQLite/FTS5 · Analizador en lote · Tags · Smart folders   │
│ Historial · Importadores · Sincronización (WebDAV/S3) · Karaoke queue  │
└──────────────────────────────────────────────────────────────────────┘
        ▲
┌───────┴──────────────────────────────────────────────────────────────┐
│ jx::cloud (opcional, autoalojable): charts, sugerencias, Pide-MX,     │
│ podcasts, conectores de streaming (plugins)                           │
└──────────────────────────────────────────────────────────────────────┘
```

## 4. Diseño del motor de audio
- **Grafo fijo por bloque** (64-512 muestras) con *double buffering* de parámetros; cambios desde UI/MIDI por cola de comandos con *timestamp* para precisión de muestra en MIDI.
- **Deck**: lector con *read-ahead* asíncrono (hilo de E/S) → separador de stems (si activo) → *resampler/time-stretch* → *scratch engine* (modelo físico de plato con inercia) → FX de inserción.
- **Stems en tiempo real**: el modelo procesa ventanas por delante del cabezal de lectura (*look-ahead* de 2-6 s) en un hilo de inferencia; si la GPU no alcanza, cambia a modelo rápido o usa caché precalculada. El hilo de audio sólo mezcla 5 buffers ya separados (costo ~0).
- **Sandbox**: segundo bus que reproduce una "copia" del estado de los decks desde una posición futura, enviado sólo a audífonos; al salir, el estado real nunca fue alterado.
- **Sincronía de video**: el reloj maestro es el audio; el video presenta el *frame* más cercano a la posición del deck (con compensación de latencia de salida).
- Objetivos: latencia de ida y vuelta ≤ 10 ms con ASIO/JACK/CoreAudio, 0 *dropouts* en prueba de 8 h con 4 decks + stems + video 1080p en equipo de gama media.

## 5. Stems: estrategia de modelos
1. **Fase inicial**: modelos libres (Demucs/HTDemucs, Open-Unmix — MIT) exportados a ONNX para análisis *offline* de calidad alta.
2. **Modelo propio "JX-Stems"**: red ligera causal (≈5-15 M parámetros) entrenada con conjuntos de datos de licencia compatible (MUSDB18-HQ para investigación, MoisesDB, contenido CC y donado), 5 salidas (voz, melodía, bajo, kick, hi-hat), cuantización INT8/FP16, objetivo < 30 % de un núcleo en CPU moderna en modo rápido.
3. **Destilación**: el modelo grande enseña al pequeño; *benchmark* público (SDR) en el repositorio.
4. Los pesos se publican con licencia compatible (GPL-3.0+ o CC-BY) y el procedimiento de entrenamiento es reproducible.

## 6. UI y skins
- Skin = carpeta con `skin.json` (metadatos, resoluciones), archivos QML y recursos. Tres skins oficiales: **Esencial** (principiante, 2 decks), **Pro** (4 decks, stems, sampler) y **Escenario** (tipo hardware, alto contraste para cabina oscura).
- Formas de onda renderizadas por GPU (RGB por frecuencia, stems coloreados, letras superpuestas).
- Idiomas: español (base), inglés y más vía Qt Linguist/Weblate.
- Accesibilidad: escalado, alto contraste, lectores de pantalla en vistas de biblioteca.

## 7. Scripting y mapeos
- **JockerScript**: verbos encadenables (`play`, `sync`, `loop 4`, `effect_active "echo"`, `deck 2 stem vocal 0%`, condiciones `? :`), consultables desde skins y pads.
- **Mapeos**: `controller.json` (identificación USB/MIDI, controles, LEDs, pantallas) + `controller.js` para lógica compleja. Editor visual con MIDI learn.
- **AI Scripting**: el LLM sólo produce JockerScript/JS que pasa por validación sintáctica, *dry-run* y vista previa antes de aplicarse.

## 8. Android
- Mismo núcleo C++ compilado con NDK; UI QML adaptada a táctil (modo 2 decks teléfono, 4 decks tableta).
- Audio con Oboe (AAudio, modo *exclusive/low-latency*), soporte de interfaces USB class-compliant y controladores USB-MIDI.
- Stems: modelo cuantizado con NNAPI/GPU; si el dispositivo no alcanza, precálculo en segundo plano.
- Variante **TV/TV Box**: interfaz navegable con control remoto (automix, karaoke, video).

## 9. Seguridad y privacidad
- Sin telemetría por defecto. Funciones comunitarias (charts, sugerencias) sólo con consentimiento explícito.
- Plugins y scripts de IA en *sandbox* (shaders validados, scripts sin acceso a archivos fuera de rutas permitidas).
- Actualizaciones firmadas.

## 10. Estructura del repositorio (objetivo)
```
Jocker-MX/
├── src/
│   ├── engine/      # motor de audio en tiempo real
│   ├── io/          # backends de audio, decodificadores
│   ├── ai/          # stems, análisis, whisper, llm, tts
│   ├── video/       # decodificación, compositor, visuales
│   ├── library/     # base de datos, análisis, importadores
│   ├── control/     # MIDI/HID, mapper, remote, link, dmx
│   ├── script/      # JockerScript + motor JS
│   ├── broadcast/   # grabación, icecast, rtmp
│   └── app/         # arranque, ajustes, perfiles
├── ui/skins/{esencial,pro,escenario}/
├── controllers/     # mapeos de controladores
├── effects/         # FX nativos
├── models/          # tarjetas de modelo + scripts de descarga/entrenamiento
├── android/         # proyecto Gradle, Oboe, manifest
├── packaging/       # wix/msix, dmg, deb, rpm, appimage, flatpak, fdroid
├── server/          # Jocker Cloud y Pide-MX (opcionales)
├── docs/            # documentación y manual de usuario
├── tests/
└── assets/          # logo e iconos
```
