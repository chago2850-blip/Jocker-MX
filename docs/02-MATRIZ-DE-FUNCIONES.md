# 02 · Matriz de paridad de funciones VirtualDJ → Jocker-MX

Leyenda de prioridad: **P0** imprescindible para la primera versión utilizable · **P1** necesaria para la paridad 1.0 · **P2** posterior a 1.0 o dependiente de terceros.
Columna "Lic. VDJ": nivel de licencia que exige VirtualDJ (Free / Home / **PRO** / Business / 3ros = suscripción a terceros). En Jocker-MX **todo es libre**.

## A. Decks y motor de audio
| ID | Función VirtualDJ | Implementación Jocker-MX | Tecnología | Fase | Prio | Lic. VDJ |
|---|---|---|---|---|---|---|
| A01 | Decks 2 a 99 | Motor N-decks (1-99), plantillas 2/4/6 | C++20, motor propio | F1 | P0 | Free |
| A02 | Play/Cue/Stutter, CUP | Transporte completo, modos de cue (CDJ, Numark, Denon) | Motor | F1 | P0 | Free |
| A03 | Pitch/tempo con rangos | ±6/8/10/16/25/50/100 %, *pitch bend*, *nudge* | Motor | F1 | P0 | Free |
| A04 | Master tempo (key lock) | Estiramiento de tiempo de alta calidad + modo económico | Rubber Band (GPL2+), SoundTouch (LGPL) | F1 | P0 | Free |
| A05 | Sync inteligente | Sync por fase/tempo, líder automático, *half/double* | Motor | F1 | P0 | Free |
| A06 | Beatgrid y análisis BPM | Detección de beat, rejilla editable | qm-dsp (GPL2+), análisis propio | F1 | P0 | Free |
| A07 | Detección de tonalidad | Notación clásica, Camelot y Open Key | libKeyFinder (GPL3) | F1 | P0 | Free |
| A08 | Hot cues / cues cuantizados | 16+ hot cues con color y nombre | Motor + BD | F1 | P0 | Free |
| A09 | Loops, auto-loop, roll, move | Loops cuantizados, *slip mode*, *beat jump* | Motor | F1 | P0 | Free |
| A10 | POI (puntos de interés) | Cues, loops guardados, puntos automix, acciones con script | BD + scripts | F8 | P1 | Free |
| A11 | Sandbox | Bus de preescucha "fantasma" con indicador de posición real | Motor (bus paralelo) | F8 | P1 | Free |
| A12 | ScratchDNA | **ScratchSeq**: notación de rutinas de scratch, pads y editor | Motor + editor | F5 | P1 | Free |
| A13 | Slicer, beat jump, reverse, brake/backspin | Incluidos como acciones y pads | Motor | F5 | P0 | Free |
| A14 | Fluid BeatGrid | Rejilla de tempo variable por segmentos | IA/DSP (modelo de beat tracking propio) | F10 | P1 | Free/PRO |
| A15 | BPM Stabilizer | Corrección de tempo en tiempo real durante la transición | Motor + mapa de tempo | F10 | P1 | Free/PRO |
| A16 | Line-In con BPM/forma de onda en vivo | Decks de entrada en vivo | Motor + análisis en streaming | F9 | P1 | Free |

## B. Mezclador
| ID | Función | Implementación | Tecnología | Fase | Prio | Lic. VDJ |
|---|---|---|---|---|---|---|
| B01 | Gain, EQ 3/4 bandas, kill | EQ Linkwitz-Riley / bell / iso-kill | DSP propio | F1 | P0 | Free |
| B02 | Filtro de un knob, color FX | HPF/LPF resonante + color FX | DSP propio | F1 | P0 | Free |
| B03 | Crossfader con curvas | Curvas, *hamster*, cut | Motor | F1 | P0 | Free |
| B04 | PFL / audífonos / split cue | Mezcla cue/master, split mono | Motor | F1 | P0 | Free |
| B05 | Limitador maestro | Limitador *look-ahead* | DSP propio | F1 | P0 | Free |
| B06 | Micrófono con talkover | Entrada de micro, ducking, EQ, FX | Motor | F9 | P1 | Free |
| B07 | Modo mezcladora externa | Salidas por deck | Ruteo de audio | F4 | P0 | Home/PRO |

## C. Stems (IA)
| ID | Función | Implementación | Tecnología | Fase | Prio | Lic. VDJ |
|---|---|---|---|---|---|---|
| C01 | Separación en tiempo real (voz, melodía, bajo, kick, hi-hat) | 5 stems + agrupaciones (drums, instrumental) | ONNX Runtime (MIT) con CUDA/DirectML/CoreML/ROCm/NNAPI; modelo **propio entrenado** + modelos libres (Demucs MIT, Open-Unmix MIT) | F6 | P0 | Free |
| C02 | Modo rápido CPU / alta calidad GPU | Selección automática por perfil de hardware | Benchmark al arrancar | F6 | P0 | Free |
| C03 | Precálculo y caché de stems | Análisis en lote, caché comprimida (FLAC/Opus multicanal) | Worker de análisis | F6 | P0 | Free |
| C04 | EZRemix / ModernEQ / stems en EQ | Modos de knobs: Voz-Instr-Beat; HiHat-Melodía-Bajo | Mezclador | F6 | P0 | Free |
| C05 | Pads Stems y Stems+FX | Páginas de pads dedicadas | UI + motor | F6 | P1 | Free |
| C06 | Exportar acapella / instrumental | Exportación a archivo | Motor | F6 | P2 | Free |

## D. Efectos, sampler y pads
| ID | Función | Implementación | Tecnología | Fase | Prio | Lic. VDJ |
|---|---|---|---|---|---|---|
| D01 | 122+ FX nativos | Biblioteca **120+ FX** propios sincronizados al beat (eco, delay, reverb, flanger, phaser, gate, bitcrush, vinyl brake, backspin, roll, *color FX* estilo mezcladora…) | DSP propio | F5 | P0/P1 | Free |
| D02 | Plugins VST/VSTi | **CLAP, VST3, LV2, AU** | CLAP (MIT), VST3 SDK (MIT), LV2 (ISC), AU (macOS) | F5 | P1 | Free |
| D03 | SDK de efectos | SDK de plugins Jocker (C API estable + CLAP) | C ABI | F5 | P2 | Free |
| D04 | Sampler ilimitado (audio, video, imagen) | Bancos, grupos, colores, modos, grabadora | Motor + video | F5/F7 | P0 | Free |
| D05 | Pads ilimitados + editor | Páginas: hotcue, loop, roll, sampler, slicer, scratch, stems, FX, custom | UI + scripts | F5 | P0 | Free |

## E. Video / VJ y visuales
| ID | Función | Implementación | Tecnología | Fase | Prio | Lic. VDJ |
|---|---|---|---|---|---|---|
| E01 | Reproducción de video hasta 4K | Decodificación por hardware | FFmpeg (LGPL/GPL), VA-API/VideoToolbox/D3D11VA/MediaCodec | F7 | P0 | Free |
| E02 | Mezcla y transiciones de video | Compositor GPU, transiciones y FX | Qt RHI (Vulkan/Metal/D3D/OpenGL), shaders GLSL→SPIR-V | F7 | P0 | Free |
| E03 | Texto, *song titler*, logo, cámara, captura de pantalla | Capas del compositor | Qt Multimedia / FFmpeg | F7 | P1 | Free (logo propio = PRO) |
| E04 | Visualizaciones audio-reactivas | 200+ presets propios + compatibilidad con presets **Milkdrop** | projectM (LGPL) + shaders propios | F7 | P1 | Free |
| E05 | Videoskins | Escenas QML para pantallas/proyectores | QML | F7 | P1 | Free |
| E06 | Shaders generados por IA | LLM genera GLSL validado en *sandbox* | LLM local/externo | F10 | P2 | Free |
| E07 | AiLoops (loops de video IA al beat) | Generación offline de loops con modelos de difusión libres | stable-diffusion.cpp (MIT) + modelos con licencia permisiva | F10 | P2 | Free |
| E08 | Salida a pantalla secundaria / NDI / Syphon / Spout | Salidas de video externas | Syphon (BSD), Spout (BSD), NDI opcional (SDK propietario → plugin aparte) | F7 | P1 | — |

## F. Karaoke
| ID | Función | Implementación | Tecnología | Fase | Prio | Lic. VDJ |
|---|---|---|---|---|---|---|
| F01 | Formatos karaoke (MP3+G/CDG, ZIP, video, KAR) | Lector CDG propio, MIDI-KAR, LRC/ELRC | Propio | F7 | P0 | Free |
| F02 | Karaoke Engine (cantantes, tonos, local) | Cola de cantantes, rotación, historial por cantante | BD + UI | F7 | P1 | Free |
| F03 | Próximo cantante en pantalla, música de fondo | Capas de video + automix de relleno | Compositor | F7 | P1 | Free |
| F04 | Karaoke automático con cualquier pista | Stems (sin voz) + letras sincronizadas por IA | C01 + J02 | F10 | P1 | Free |

## G. Biblioteca, listas y descubrimiento
| ID | Función | Implementación | Tecnología | Fase | Prio | Lic. VDJ |
|---|---|---|---|---|---|---|
| G01 | Base de datos y búsqueda instantánea | SQLite + FTS5, búsqueda con alcance (`bpm:120-128 key:8A`) | SQLite (dominio público) | F2 | P0 | Free |
| G02 | Lectura/edición de etiquetas | ID3, Vorbis, MP4, APE, portadas | TagLib (LGPL/MPL) | F2 | P0 | Free |
| G03 | Smart/Filter folders | Lenguaje de filtros + editor visual | Propio | F2 | P0 | Free |
| G04 | Playlists, sidelist, historial, sets | Incluidos; exportación M3U8/CSV/cue | Propio | F2 | P0 | Free |
| G05 | Compatibilidad por tonalidad/BPM | Filtros y sugerencias armónicas | Propio | F2 | P0 | Free |
| G06 | Duplicados (IA) | Huella acústica + similitud | Chromaprint (LGPL) | F10 | P1 | Free |
| G07 | GeniusDJ / Live feedback | Sugerencias locales (similitud de audio, historial) + red comunitaria opcional con *opt-in* | Embeddings de audio locales + Jocker Cloud | F10/F12 | P1/P2 | Free |
| G08 | Charts comunitarios | Charts federados en Jocker Cloud (autoalojable) | Servidor libre | F12 | P2 | Free |
| G09 | CloudDrive | Sincronización con **WebDAV / Nextcloud / S3 / carpeta sincronizada** | Propio | F12 | P2 | Free |
| G10 | Exportación para reproductores tipo CDJ (OneLibrary) | Exportación USB sólo con especificaciones públicas / interoperabilidad legal | Investigación | F12 | P2 | Free |
| G11 | Importar bibliotecas | iTunes/Music XML, Rekordbox XML, Traktor NML, Serato crates, Mixxx, **VirtualDJ database.xml** | Importadores | F2 | P1 | — |

## H. Automix y editores
| ID | Función | Implementación | Fase | Prio |
|---|---|---|---|---|
| H01 | Automix con estructura y transiciones inteligentes | Detección de intro/outro/drops, transiciones por estilo | F8 | P0 |
| H02 | Editor Automix | Ajuste de puntos de mezcla por pista | F8 | P1 |
| H03 | Editor BPM / beatgrid | Rejilla manual, *tap*, segmentos fluidos | F8 | P0 |
| H04 | Editor POI, Pads, Scratch, Samples, Tags | Editores dedicados | F8 | P1 |
| H05 | Editor de letras | Sincronía por línea/palabra, importación LRC | F8 | P1 |
| H06 | Track Cleaner (limpiar o extender pistas) | Editor no destructivo por regiones (cortar, repetir, silenciar) + render | F8 | P1 |
| H07 | Editor de video | Recorte, offset A/V, portada de video | F8 | P2 |
| H08 | Event Scheduler | Programador de eventos (iniciar automix, playlists, anuncios por hora) | F8 | P1 |

## I. Hardware, control y personalización
| ID | Función | Implementación | Tecnología | Fase | Prio | Lic. VDJ |
|---|---|---|---|---|---|---|
| I01 | 300+ controladores plug-and-play | Mapeos en JSON+JS; meta: 50 en 1.0, 300+ con la comunidad; importar mapeos de Mixxx (GPL2+) | PortMidi/RtMidi (MIT), hidapi (BSD) | F4 | P0 | **Home/PRO** |
| I02 | Mapper + MIDI learn | Editor visual de mapeos | UI | F4 | P0 | Free |
| I03 | Definiciones personalizadas | Libres, sin restricciones | — | F4 | P0 | **PRO** |
| I04 | VDJScript | **JockerScript**: lenguaje de verbos propio + JavaScript (QJSEngine) | Propio | F4 | P0 | Free |
| I05 | AI Scripting | Lenguaje natural → JockerScript con validación y vista previa | LLM | F10 | P2 | PRO |
| I06 | DVS / timecode (smart, relativo, absoluto) | Decodificación de timecode de vinilo/CD estándar abiertos y propios | xwax timecoder (GPL) | F4 | P1 | PRO |
| I07 | Skins e interfaces tipo hardware | Motor de skins QML + JSON, 3 skins oficiales | Qt Quick (LGPL3) | F3 | P0 | Free |
| I08 | Atajos de teclado | Totalmente configurables | — | F3 | P0 | Free |
| I09 | Remote (iOS/Android) | **Jocker Remote**: app Android + web app (PWA) por Wi-Fi/USB | Qt/QML + WebSocket | F9 | P1 | Free |
| I10 | Ableton Link | Sincronía de tempo en red | Ableton Link (GPL2+) | F9 | P1 | Free |
| I11 | ReWire (descontinuado por su fabricante) | Sustituido por JACK/PipeWire y dispositivos de audio virtuales | JACK, PipeWire | F9 | P2 | Free |
| I12 | DMX / OS2L | OS2L + Art-Net + sACN nativos, escenas al beat | Propio / OLA (LGPL) | F9 | P2 | Free |
| I13 | Sin logo/audio drop, uso profesional, multiusuario | **Siempre**: sin marcas, uso profesional permitido, perfiles multiusuario | — | F0 | P0 | **PRO/Business** |

## J. Funciones de IA
| ID | Función | Implementación | Tecnología | Fase | Prio | Lic. VDJ |
|---|---|---|---|---|---|---|
| J01 | AIPrompt | **Asistente Jocker**: LLM local o endpoint compatible OpenAI/Ollama configurado por el usuario; responde con pistas de *tu* biblioteca | llama.cpp (MIT), RAG sobre la BD | F10 | P1 | **PRO** |
| J02 | Letras por IA (cualquier canción) | Transcripción local multilingüe con marcas de tiempo por palabra, sobre stem de voz | whisper.cpp (MIT) | F10 | P1 | **PRO** (completo) |
| J03 | ShoutOut (voces generativas) | TTS local con voces libres + efectos; reemplazo de letras experimental | Piper (GPL3) u otro TTS libre | F10 | P2 | Free/PRO |
| J04 | Visuals IA | Ver E06/E07 | — | F10 | P2 | Free |
| J05 | Duplicados IA | Ver G06 | — | F10 | P1 | Free |

## K. Grabación, transmisión y público
| ID | Función | Implementación | Tecnología | Fase | Prio | Lic. VDJ |
|---|---|---|---|---|---|---|
| K01 | Grabación audio/video | WAV/FLAC/MP3/Opus/AAC, MP4/MKV, split por pista, cue sheet | FFmpeg, LAME (LGPL) | F9 | P0 | Free |
| K02 | Shoutcast/Icecast con metadatos | Transmisión con títulos automáticos | libshout (LGPL) | F9 | P1 | Free |
| K03 | Servidor de radio PC-a-PC | Servidor HTTP de streaming integrado | Propio | F9 | P2 | Free |
| K04 | Podcasts | Publicación de grabaciones en almacenamiento propio (RSS) | Propio | F12 | P2 | **PRO** |
| K05 | Transmisión de video a redes sociales | RTMP/SRT/WHIP con overlays | FFmpeg | F9 | P1 | Free |
| K06 | Ask The DJ | **Pide-MX**: web app autoalojable de peticiones/mensajes, QR, propinas mediante enlaces externos | Servidor ligero | F9 | P2 | Free |

## L. Contenido en línea (requiere cuentas de terceros)
| ID | Servicio VDJ | Estrategia Jocker-MX | Fase | Prio |
|---|---|---|---|---|
| L01 | SoundCloud, TIDAL, Deezer, Beatport, Beatsource | Plugins conectores **sólo con API oficial y acuerdo del proveedor**; el usuario usa su propia suscripción. Si un proveedor no autoriza a software libre, el conector no se publica. | F12 | P2 |
| L02 | iDJPool, Digitrax, Sunfly, VJ-Pro | Igual: conector por acuerdo; mientras tanto, soporte excelente de **descargas locales** de *record pools* | F12 | P2 |
| L03 | Contenido libre | Conectores a Jamendo / Free Music Archive / Internet Archive (licencias CC) | F12 | P2 |

## M. Plataformas (más allá de VirtualDJ)
| ID | Plataforma | Notas |
|---|---|---|
| M01 | Windows 10/11 x64 y ARM64 | WASAPI, **ASIO (SDK GPLv3 desde oct-2025)**, MSIX/MSI |
| M02 | macOS 12+ (Apple Silicon e Intel) | CoreAudio, AU, Metal, .dmg firmado/notarizado |
| M03 | Linux x86_64/ARM64 | ALSA, PipeWire, JACK; **.deb y .rpm** prioritarios, AppImage y Flatpak |
| M04 | Android 9+ (teléfono, tableta, TV/TV Box) | Oboe/AAudio, USB MIDI/Audio class-compliant, UI táctil; APK, AAB y F-Droid |
