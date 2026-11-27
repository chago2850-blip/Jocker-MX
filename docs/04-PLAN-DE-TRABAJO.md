# 04 · Plan de trabajo de Jocker-MX

**Objetivo:** entregar en **24 meses** una aplicación DJ libre (GPL-3.0-or-later) con paridad funcional con VirtualDJ 2026 —incluidas las funciones que allí requieren licencia PRO/Business— para **Windows, Linux, macOS y Android**.

## 1. Supuestos
- Equipo base de **6-8 personas** (ver §6). Con un equipo menor, los plazos se alargan de forma proporcional; el orden de fases se mantiene.
- Iteraciones (*sprints*) de 2 semanas; versión publicable al final de cada fase.
- Desarrollo abierto en GitHub (`Jocker-MX`), CI en cada *pull request* para las 4 plataformas.

## 2. Hitos de versión
| Versión | Mes | Contenido mínimo |
|---|---|---|
| **0.1 "Primer beat"** (alfa) | 6 | 2-4 decks, mezclador, sync, key lock, cues/loops, biblioteca, skin Esencial, Win/Linux/mac |
| **0.5 "Cabina"** (beta) | 12 | Controladores (25+), FX nativos, sampler, pads, stems en tiempo real, grabación, Android alfa |
| **0.8 "Escenario"** | 17 | Video, visuales, karaoke, automix, editores, sandbox, DVS, broadcast, remote |
| **0.9 RC** | 21 | IA completa (asistente, letras, fluid grid, ShoutOut, AI scripting), Android beta, nube opcional |
| **1.0 "Jocker"** | 24 | Paridad P0+P1, 50+ controladores, manual completo, instaladores firmados para las 4 plataformas |

## 3. Fases
Las fases se solapan; la columna "Meses" indica la ventana principal.

### F0 · Fundaciones (meses 0-2)
- Repositorio, licencia GPL-3.0-or-later, `CONTRIBUTING`, código de conducta, plantilla de *issues*.
- CMake + vcpkg, Qt 6.8, compilación en Windows (MSVC/clang-cl), macOS (universal), Linux (GCC/Clang), Android (NDK).
- CI (GitHub Actions): compilar, pruebas, análisis estático (clang-tidy), *sanitizers*, artefactos nocturnos.
- Capa `jx::audio` con backends WASAPI, CoreAudio, ALSA/PipeWire/JACK, Oboe; prueba de latencia.
- Esqueleto de la app QML y sistema de ajustes/perfiles.
- **Entregable:** app vacía que reproduce un WAV con latencia medida en las 4 plataformas.

### F1 · Motor de audio núcleo (meses 2-6)
- Decodificadores (FFmpeg), lector con *read-ahead*, caché de forma de onda.
- Decks N, transporte, pitch/tempo, *master tempo* (Rubber Band/SoundTouch), *nudge*, scratch básico.
- Análisis: BPM/beatgrid (qm-dsp), tonalidad (KeyFinder), ganancia (R128).
- Sync, cuantización, hot cues, loops, *slip*, beat jump.
- Mezclador: gain, EQ, filtro, crossfader, PFL/cue, limitador.
- **Criterio de aceptación:** 4 decks a 48 kHz, buffer 128, sin *dropouts* en 2 h; error de sync < 1 ms.

### F2 · Biblioteca (meses 4-7)
- SQLite + FTS5, escáner incremental, vigilancia de carpetas, TagLib.
- Búsqueda con alcance, smart/filter folders, playlists, sidelist, historial y sets.
- Compatibilidad armónica/BPM, columnas configurables, portadas.
- Importadores: iTunes/Music, Rekordbox XML, Traktor NML, Serato, Mixxx, VirtualDJ `database.xml`.
- **Criterio:** búsqueda < 50 ms en 100 000 pistas.

### F3 · Interfaz y skins (meses 5-9)
- Motor de skins QML + `skin.json`; skins **Esencial** y **Pro** (Escenario en F7).
- Formas de onda GPU (RGB, zoom, *Rhythm Wave* multidek), vista de pads, navegador con SideView.
- Atajos de teclado, temas, i18n (es/en), accesibilidad, ayuda contextual y tutoriales integrados.
- Diseño UX con pruebas con DJs reales (5 sesiones por iteración de skin).

### F4 · Controladores, scripting y DVS (meses 7-11)
- RtMidi/hidapi, detección automática, LEDs y pantallas de jog.
- JockerScript (verbos) + motor JS para mapeos; editor visual con MIDI learn.
- 25 controladores prioritarios en 0.5 (Pioneer DDJ-FLX4/FLX10/REV, Numark Mixtrack/NS, Hercules Inpulse/DJControl, Denon MC/SC, Reloop, Rane…); 50+ en 1.0; importar mapeos de Mixxx.
- Modos de audio: mezcladora externa, controlador con tarjeta, timecode.
- DVS: decodificación de timecode (vinilo/CD), modos absoluto/relativo/smart, calibración.
- **Criterio:** latencia de jog → audio < 5 ms; prueba con 10 controladores físicos.

### F5 · Efectos, sampler y pads (meses 8-12)
- 60 FX nativos en 0.5, **120+** en 1.0, todos sincronizables al beat; *color FX* por canal.
- Host de plugins CLAP/VST3/LV2/AU con escaneo aislado (proceso separado ante fallos).
- Sampler (bancos, grupos, modos, grabadora), pads ilimitados y editor de pads.
- ScratchSeq (rutinas de scratch por notación) y editor.

### F6 · Stems IA (meses 9-14)
- Integración ONNX Runtime con proveedores GPU por plataforma y *benchmark* automático.
- Modelos libres para calidad alta *offline* + entrenamiento del modelo propio JX-Stems (5 stems) en paralelo.
- Pipeline en tiempo real con *look-ahead*, caché de stems, análisis en lote.
- Modos de knobs (Voz/Instr/Beat; HiHat/Melodía/Bajo), pads Stems y Stems+FX.
- **Criterio:** separación en tiempo real con 2 decks en GPU media y modo rápido en CPU de 4 núcleos; SDR de voz ≥ 7 dB en el *benchmark*.

### F7 · Video, visuales y karaoke (meses 11-16)
- Decodificación por hardware, compositor RHI, crossfade y transiciones, 40+ FX de video.
  - ✅ **0.11.0-dev**: video en los decks sincronizado con el audio (tempo, pausa, cue, scratch), mezcla por crossfader (fundido, barrido, corte) y salida a pantalla del público. Pendiente: aceleración por hardware dedicada, FX de video, capas y visuales.
- Capas: texto, *song titler*, logo propio, cámara, captura de pantalla, imágenes.
- Visualizaciones (projectM + shaders propios), videoskins QML, salida a segunda pantalla, Syphon/Spout.
- Karaoke: CDG/MP3+G, KAR, LRC, video; Karaoke Engine (cola de cantantes, tonos, música de fondo, próximo cantante).
  - ✅ **0.10.0-dev**: CD+G (MP3+G) y LRC, cambio de tono por semitonos, rotación de cantantes con tono por turno y pantalla del público. Pendiente: KAR, música de fondo entre turnos, video.
- Skin **Escenario**.

### F8 · Automix, editores y Sandbox (meses 13-17)
- Automix con detección de estructura (intro/outro/drops) y transiciones por estilo; Event Scheduler.
  - ✅ **0.7.0-dev**: cola de Automix, salto de silencios, salida en compás, sync de tempo y fase, intercambio de graves, fundido para tempos incompatibles, listas de reproducción. 
  - ✅ **0.8.0-dev**: detección de estructura (intro/cuerpo/break/drop/outro), Automix AUTO outro→intro, franja de secciones con salto cuantizado. 
  - ✅ **0.9.0-dev**: estilos de transición (mezcla, filtro, eco, corte, auto) y programador de eventos (Event Scheduler).
- Editores: BPM/beatgrid, POI, Automix, Pads, Samples, Tags, Letras, Track Cleaner, Video.
- Sandbox (bus de preescucha futuro) con indicador de posición real.

### F9 · Grabación, transmisión, red y público (meses 15-19)
- Grabación audio/video, *split* automático, cue sheet.
- Icecast/Shoutcast, servidor de radio integrado, RTMP/SRT/WHIP con overlays.
  - ✅ **0.12.0-dev**: transmisión a Icecast 2 (PUT/SOURCE) y SHOUTcast v1/v2 en Opus (Ogg) o MP3, reconexión automática, títulos, estadísticas. Pendiente: servidor de radio integrado, RTMP/SRT/WHIP (video).
- Ableton Link, JACK/PipeWire, micrófono con *talkover*, line-in con análisis en vivo.
- **Jocker Remote** (Android + PWA) y **Pide-MX** (peticiones del público con QR).
- DMX: OS2L, Art-Net, sACN.

### F10 · Inteligencia artificial avanzada (meses 16-21)
- **Asistente Jocker** (AIPrompt): llama.cpp local con RAG sobre la biblioteca, o endpoint compatible OpenAI/Ollama.
- **Letras**: whisper.cpp sobre stem de voz, alineación por palabra, editor; karaoke automático.
- **Fluid BeatGrid** y **BPM Stabilizer**.
- Duplicados por huella (Chromaprint), sugerencias por similitud (embeddings locales).
- **ShoutOut**: TTS local (Piper) + FX; reemplazo de letras experimental.
- **AI Scripting** con validación; **Shaders** por texto; **AiLoops** offline.
- Gestor de modelos: descarga bajo demanda, verificación de hash, tarjetas de modelo y licencias.

### F11 · Android (meses 10-22, en paralelo)
- Mes 10-12: alfa (2 decks, biblioteca local, mezclador, sync).
- Mes 13-18: UI táctil completa, USB MIDI/audio, stems cuantizados, FX, sampler.
- Mes 19-22: beta; variante TV/TV Box; publicación en F-Droid y Google Play.

### F12 · Servicios en línea opcionales (meses 18-22)
- Jocker Cloud autoalojable (Docker): charts, sugerencias comunitarias *opt-in*, podcasts, sincronización.
- Sincronización con WebDAV/Nextcloud/S3.
- Programa de conectores de streaming: solicitar acceso a APIs oficiales (SoundCloud, TIDAL, Beatport/Beatsource, Deezer) y a *record pools*; publicar sólo los autorizados. Conectores CC (Jamendo, FMA, Internet Archive).
- Exportación a reproductores autónomos sólo con especificaciones públicas.

### F13 · Calidad, empaquetado y lanzamiento (continuo; foco meses 20-24)
- Instaladores: Windows **MSI + MSIX** (x64/ARM64), macOS **.dmg** universal firmado y notarizado, Linux **.deb y .rpm** (prioritarios) + AppImage + Flatpak, Android **APK/AAB**.
- Manual de usuario completo (español e inglés) con capturas, tutoriales en video y guía de migración desde VirtualDJ.
- Pruebas: unitarias, de integración de audio (comparación de muestras), de rendimiento, *soak test* 8 h, pruebas con DJs en eventos reales (programa beta).
- Auditoría de licencias (REUSE/SPDX en cada archivo, SBOM).

## 4. Cronograma (Gantt simplificado)
```
Mes:            0    3    6    9    12   15   18   21   24
F0 Fundaciones  ███
F1 Motor          ██████
F2 Biblioteca         ████
F3 UI/Skins             ██████
F4 Control/DVS                ██████
F5 FX/Sampler                  █████
F6 Stems IA                      ███████
F7 Video/Karaoke                      ████████
F8 Automix/Edit                           ██████
F9 Rec/Broadcast                             ██████
F10 IA avanzada                                ████████
F11 Android                         ████████████████
F12 Nube                                          █████
F13 QA/Release  ·····························████████
Versiones:              0.1         0.5       0.8    0.9  1.0
```

## 5. Primeras 12 semanas (detalle)
| Semana | Tareas |
|---|---|
| 1-2 | CMake/vcpkg, Qt 6, CI 4 plataformas, REUSE/SPDX, estructura del repo |
| 3-4 | `jx::audio`: WASAPI, CoreAudio, PipeWire/ALSA, Oboe; herramienta de medición de latencia |
| 5-6 | Decodificador FFmpeg + lector asíncrono; deck único con play/pausa/cue |
| 7-8 | Time-stretch (Rubber Band), pitch, forma de onda básica; 2 decks + crossfader |
| 9-10 | Análisis BPM/tonalidad en hilo aparte; sync y cuantización |
| 11-12 | EQ/filtro, PFL/audífonos, hot cues, loops; demo interna "Primer beat" |

## 6. Equipo propuesto
| Rol | Personas | Responsabilidad |
|---|---|---|
| Líder técnico / arquitecto de audio | 1 | Motor en tiempo real, revisión de código |
| Ingeniero DSP | 1 | FX, time-stretch, scratch, análisis |
| Ingeniero de IA/ML | 1-2 | Stems, letras, LLM, fluid grid, entrenamiento |
| Ingeniero de UI (Qt/QML) | 1-2 | Skins, formas de onda, UX |
| Ingeniero de plataformas | 1 | Android, empaquetado, CI, firma |
| Ingeniero de controladores/video | 1 | MIDI/HID, DVS, video, karaoke |
| Diseño UX/UI + documentación | 1 (parcial) | Skins, iconografía, manual |
| Comunidad | voluntarios | Mapeos de controladores, traducciones, pruebas |

## 7. Riesgos y mitigación
| Riesgo | Impacto | Mitigación |
|---|---|---|
| Stems en tiempo real sin GPU potente | Alto | Modelo propio ligero + caché precalculada + modo rápido |
| Latencia en Android variable por fabricante | Alto | Oboe, lista de dispositivos probados, recomendar USB audio |
| Proveedores de streaming no autorizan software libre | Medio | Arquitectura de plugins; enfocarse en biblioteca local y contenido CC |
| Volumen de mapeos de controladores | Medio | Formato abierto, importador de Mixxx, programa comunitario, MIDI learn |
| Licencias incompatibles en dependencias o modelos | Alto | Revisión en cada PR, SBOM, sólo modelos con licencia compatible |
| Marcas registradas / *trade dress* | Medio | Nombres, iconos, sonidos y skins 100 % propios; no usar "VirtualDJ" salvo descripción de compatibilidad |
| Alcance excesivo | Alto | Prioridades P0/P1/P2 y versiones por hitos |

## 8. Definición de "terminado" (por función)
1. Funciona en Windows, Linux y macOS (y Android si aplica según la matriz).
2. Pruebas automáticas y prueba manual con hardware cuando aplique.
3. Documentada en el manual de usuario (español e inglés).
4. Sin regresiones de rendimiento (latencia y CPU dentro de presupuesto).
5. Licencias de todo código/recurso nuevo registradas (SPDX).

## 9. Métricas de éxito de la versión 1.0
- 100 % de funciones P0 y ≥ 90 % de P1 de la matriz.
- 50+ controladores con mapeo oficial.
- Latencia ≤ 10 ms (ASIO/JACK/CoreAudio), ≤ 20 ms en Android con USB audio.
- *Soak test* de 8 h sin fallos con 4 decks + stems + video 1080p.
- Instaladores para las 4 plataformas publicados en GitHub Releases.
