# 01 · Investigación: funcionamiento y capacidades de VirtualDJ

> Documento de referencia funcional para **Jocker-MX**. Describe *qué hace* VirtualDJ (Atomix Productions) para poder reimplementarlo desde cero ("clean room"): no se copia código, gráficos, sonidos, skins, marcas ni textos del producto original.
>
> Fecha de la investigación: 24 de noviembre de 2026 · Versión analizada: **VirtualDJ 2026** (Partes 1, 2 y 3).

---

## 1. Visión general

| Aspecto | VirtualDJ 2026 |
|---|---|
| Desarrollador | Atomix Productions (Miami, EE. UU.) |
| Tipo | Software DJ de audio + video + karaoke con IA |
| Plataformas | Windows y macOS (macOS 10.15+). **No existe versión nativa para Android ni Linux**; en móviles sólo hay apps de *control remoto*. |
| Modelo de negocio | Gratis para uso no profesional sin hardware; licencias de pago para controladores, uso profesional e IA avanzada |
| Público | Desde principiantes hasta DJs de club, bodas, radio, VJs y karaoke |
| Diferenciadores | Separación de *stems* en tiempo real (desde 2020), 2 a 99 decks, video y karaoke integrados, VDJScript, soporte de 300+ controladores, asistente IA |

## 2. Historial reciente de versiones (lo más relevante)

- **VirtualDJ 2021** – primera separación de stems en tiempo real: modos *EZRemix* (voces / instrumentos / beat) y *ModernEQ* (hi-hat / melodía / bajo), hasta 5 stems mapeables a controladores.
- **VirtualDJ 2023 – Stems 2.0** – motor de redes neuronales profundas; acapellas e instrumentales limpios; aislamiento de voces, melodía, bajo, kick, hi-hat; aceleración por GPU (NVIDIA / Apple Silicon) y modo rápido de menor calidad por CPU; preparación previa de stems para archivos.
- **VirtualDJ 2026 Parte 1 (dic-2025)** – letras extraídas por IA mostradas dentro de la forma de onda; carpeta **AIPrompt** (asistente LLM que recomienda pistas arrastrables al deck, requiere PRO); motor de FX renovado con **más de 122 efectos** (incluye equivalentes de los FX de mezcladoras Pioneer y Rane); plugin de video de letras; karaoke automático con cualquier pista; carpeta de duplicados con IA.
- **VirtualDJ 2026 Parte 2 (feb-2026)** – **Visuals** con IA cuando se reproduce sólo audio: *Shaders* audio-reactivos generables por texto y *AiLoops* (loops de video generados y sincronizados al beat); **Fluid BeatGrid** (rejilla para tempos variables: rock antiguo, disco, vinilos, megamixes); **BPM Stabilizer** durante transiciones; mejor detección multilingüe de letras.
- **VirtualDJ 2026 Parte 3 (sep-2026)** – **ShoutOut** (voces generativas para drops, anuncios y reemplazo de letras en vivo); **AI Scripting** (describir en lenguaje natural el comportamiento deseado y la IA escribe el script); nuevos controladores y catálogos.

## 3. Mapa funcional completo

### 3.1 Interfaz (según la estructura del manual oficial)

**Sección superior**
- Controles de la aplicación: diseño (*layout*) y modo de forma de onda, **Sandbox**, **ShoutOut**, **Event Scheduler** (programador de eventos).
- **Rhythm Wave**: vista rítmica/forma de onda de todos los decks alineada.

**Decks** (2 a 99)
- Información de la pista (título, artista, BPM, tonalidad, tiempo, portada, letras).
- Controles básicos: play/pausa, cue, sync, pitch/tempo con rango configurable, *master tempo* (key lock), *nudge*, jog wheel virtual.
- Controles avanzados: efectos por deck, loops (manuales, auto, *roll*, *move*, cuantizados), **pads** de rendimiento y **botones personalizados**.

**Mezclador**
- Audio: ganancia, EQ de 3/4 bandas con *kill*, filtro de un solo knob, faders, crossfader con curvas, VU-metros, PFL/cue de audífonos.
- Video: crossfader de video, transiciones y efectos de video, **Visuals**.
- Scratch: controles de scratch/vinilo.
- Master: volumen maestro, limitador, grabación, *broadcast*.

**Navegador (Browser)**
- Barra lateral, lista de carpetas, lista de archivos, información de archivo.
- **SideView**: *Sidelist* (lista de espera), *Remixes*, *Sampler*, *Automix*, *Karaoke*, *Clone*.

**Base de datos**
- Búsqueda con parámetros de alcance, análisis de pistas (BPM, tonalidad, beatgrid, volumen, stems, letras).
- **Smart Folders / Filter Folders** con sintaxis de filtros.
- **Online Music**: catálogos en línea, catálogos alternos y **CloudDrive**.
- Playlists, **Historial** y *Sets* por fecha.
- **GeniusDJ** + *Live Feedback*: sugerencias según lo que tocan otros DJs.
- **Ask The DJ**: peticiones y mensajes del público.
- **Charts**: listas de la comunidad y "lo más tocado".
- **OneLibrary**: exportación de biblioteca para hardware (reproductores tipo CDJ).
- **Stems** y **AI Prompt**.

### 3.2 Motor de audio y rendimiento
- Estiramiento de tiempo y cambio de tono de alta calidad (*master tempo*), alta frecuencia de muestreo, limitador, ruteo avanzado.
- **Sync** inteligente, beatgrid, cuantización, *slip mode*.
- **Hot cues**, cues cuantizados, **POI** (*Points of Interest*: cues, loops guardados, puntos de automix, acciones).
- Loops sin cortes, *loop roll*, *slicer*, *beat jump*.
- **Sandbox**: previsualizar en audífonos partes posteriores de la mezcla sin afectar lo que escucha el público, con indicador de la posición real del master.
- **ScratchDNA**: rutinas de scratch automáticas escritas con notación de letras, pads de scratch y editor DNA.
- **Pads**: páginas ilimitadas (hotcues, loops, sampler, slicer, loop roll, scratch, stems, stems+FX) y editor de pads.

### 3.3 Stems (separación en tiempo real)
- Componentes: voces, melodía/instrumentos, bajo, kick, hi-hat (y drums agrupados).
- Modos: *EZRemix*, *ModernEQ*, stems en EQ, pads de stems, página Stems+FX (4 pads de stems + 4 de efectos para voz/instrumentos).
- Acapella e instrumental instantáneos, mashups en vivo.
- GPU (NVIDIA, Apple Silicon) o CPU; modo rápido de calidad reducida; precálculo y caché de stems.

### 3.4 Efectos y muestras
- 122+ efectos nativos de audio (eco, flanger, filtros, reverb, *brake*, *backspin*, *beat grid*, *color FX* estilo mezcladora, etc.), con sincronía al beat.
- Plugins **VST / VSTi** y SDK propio para efectos.
- **Sampler**: bancos y slots ilimitados, grupos con colores e iconos, modos (stutter, loop, one-shot…), muestras de audio, video e imagen fija, editor de muestras, grabadora, transparencia.

### 3.5 Video / VJ
- Formatos principales hasta **4K**, previsualizaciones, crossfades de video inteligentes, efectos y transiciones.
- Texto a pantalla, *song titler*, cámara, captura de pantalla, superposiciones (logos).
- **Visualizaciones** (miles: patrones, túneles, ecualizadores) automáticas en pistas sólo-audio, sincronizadas al beat.
- **Videoskins** para pantallas o proyectores (loops, efectos, pista actual y siguiente, túneles al beat), personalizables.
- Novedades IA: Shaders por texto, AiLoops, plugin de letras en video.

### 3.6 Karaoke
- Reproduce los formatos de karaoke populares (p. ej. MP3+G/CDG y video) con letras en pantalla, en cada deck o mediante el **Karaoke Engine**.
- Lista de cantantes (nombres, tonalidad por canción, nombre del local), lista "próximo cantante" en pantalla, música de fondo automática, gráficos entre canciones.
- **Karaoke automático** con IA sobre cualquier pista (stems + letras detectadas).

### 3.7 Biblioteca y listas
- Búsqueda rápida, carpetas inteligentes, árbol de carpetas, arrastrar y soltar, lectura y edición de etiquetas.
- Listas, ideas y filtros: compatibilidad por **tonalidad y BPM**, búsqueda de duplicados, *sidelist*.
- Historial automático por fecha, compartir listas en línea y redes sociales.
- Charts comunitarios (públicos/privados) y feeds de servicios externos.
- **CloudDrive**: sincronizar listas y archivos, streaming de tu propia música desde cualquier lugar, respaldo.

### 3.8 Automix y editores
- **Automix** con detección de estructura de la canción y beats, transiciones inteligentes, tiempo restante, eliminación de pistas tocadas, reordenar arrastrando.
- **Editores**: Automix, BPM, DNA Scratch, Pads (y Scratch Bank), POI, **Letras**, Muestras, Etiquetas, **Track Cleaner** (limpiar o extender pistas), Video.

### 3.9 Hardware, control y personalización
- **300+ controladores y mezcladoras** plug-and-play (Pioneer DJ, Denon DJ, Numark, Reloop, Hercules, Rane…).
- *Mapper*, *MIDI learn*, definiciones personalizadas de controladores (HID/MIDI).
- **DVS / timecode**: vinilos y CDJ en modos *smart*, relativo y absoluto, calibración automática, compatibilidad con timecode de terceros.
- **VDJScript**: lenguaje de verbos que controla casi todo (mapeos, botones, skins).
- **Skins** (incluye skins que imitan hardware) y SDK de skins; atajos de teclado personalizables.
- **Remote**: apps iOS/Android para teléfono y tableta por Wi-Fi, USB o Bluetooth, con o sin Internet.
- **Ableton Link**, **ReWire** y **line-in** con lectura de BPM y forma de onda en vivo.
- **DMX**: control de luces, escenas y máquinas de humo mediante software compatible con **OS2L**.

### 3.10 Configuración de audio (modos del manual)
Master Out · Master + Audífonos · Mezcladora externa · Controlador · Loopback de grabación · Micrófono · Line-In · Timecode · ReWire.

### 3.11 Grabación y transmisión
- Grabación de audio y video en formatos populares, inicio automático, fuentes externas (loopback, micrófono), división automática de pistas y exportación de *cue sheet* para quemar CD.
- Radio por Internet **Shoutcast / Icecast** con títulos automáticos, servidor de radio PC-a-PC integrado, podcasts.
- Transmisión de video a redes sociales con videoskins, visualizaciones, cámara y logo.

### 3.12 Catálogos de contenido en línea
- Catálogos de suscripción de socios: **iDJPool** (música), **Digitrax / Sunfly** (karaoke), **VJ-Pro** (video); prueba de 30 s.
- Servicios de streaming: **TIDAL, Deezer, SoundCloud, Beatport, Beatsource**; caché sin conexión según el plan del servicio.

### 3.13 Funciones de IA (2026)
| Función | Descripción |
|---|---|
| AIPrompt | Asistente LLM que responde preguntas y propone pistas razonadas listas para cargar |
| Letras por IA | Transcripción automática multilingüe mostrada en la forma de onda y en video |
| Karaoke automático | Quita la voz y muestra letras de cualquier pista |
| Duplicados IA | Detecta duplicados por contenido |
| Visuals: Shaders / AiLoops | Visuales audio-reactivos generados por texto y loops de video IA al beat |
| Fluid BeatGrid + BPM Stabilizer | Rejilla variable y estabilización temporal para mezclar pistas con tempo fluctuante |
| ShoutOut | Voces generativas: drops, anuncios, reemplazo de letras |
| AI Scripting | Lenguaje natural → scripts de control |

## 4. Licenciamiento de VirtualDJ (lo que Jocker-MX ofrecerá libre)

| Nivel | Precio | Qué habilita / restringe |
|---|---|---|
| **Free** | 0 | Uso no profesional y sin hardware DJ profesional |
| **Home** | US$4/mes | Sólo controladores de gama de entrada; mantiene la marca VirtualDJ (logo en video y *audio drop*) |
| **Pro** | US$19/mes (o Pro Infinity de pago único, aún disponible pero sin algunas funciones IA nuevas) | Uso profesional (cobrar por tocar), **cualquier controlador**, definiciones de controlador personalizadas, quitar/cambiar logo de video, sin *audio drop*, **IA avanzada** (AIPrompt, letras completas de cualquier canción), almacenamiento de podcasts |
| **Business / Mission Critical** | US$99/mes | Todo Pro + soporte empresarial, concierge, SMS/llamada, personalización profesional, **licencias multiusuario** |

Además, el contenido en línea (iDJPool, Digitrax, VJ-Pro, TIDAL, Deezer, SoundCloud, Beatport, Beatsource) requiere **suscripciones a terceros**.

**En Jocker-MX todas las funciones "PRO / Business" serán gratuitas y libres** (GPL-3.0-or-later): cualquier controlador, uso profesional, sin marcas de agua ni *audio drops*, IA completa, multiusuario. Lo único que no puede regalarse es el **contenido de terceros**: los conectores de streaming se implementarán sólo con APIs oficiales y requerirán la cuenta/suscripción del propio usuario (ver documento 05).

## 5. Puntos débiles detectados (oportunidades para Jocker-MX)
1. No hay versión nativa para **Linux** ni **Android**.
2. Modelo de licencias confuso (Home vs Pro vs Infinity; licencias atadas a un controlador).
3. Funciones IA dependen de la nube y de suscripción; Jocker-MX las ejecutará **localmente** (privacidad, sin Internet en el evento) con opción de servicios externos.
4. Formato propietario de scripts y skins; Jocker-MX usará estándares abiertos (QML, JavaScript, JSON, CLAP/LV2/VST3).

## Fuentes
- Funciones: https://virtualdj.com/products/virtualdj/features.html
- Manual: https://virtualdj.com/manuals/virtualdj/
- Novedades 2026: https://virtualdj.com/forums/266309/General_Discussion/VirtualDJ_2026.html
- Precios y comparativa: https://virtualdj.com/products/virtualdj/price.html · https://virtualdj.com/buy/
- Stems: https://virtualdj.com/help/stems.html · https://virtualdj.com/manuals/virtualdj/interface/stems2.html
- Catálogos: https://virtualdj.com/products/contentcatalogs.html · https://virtualdj.com/manuals/virtualdj/interface/database/onlinemusic/altcatalogs.html
- App Store (requisitos macOS): https://apps.apple.com/mx/app/virtualdj/id1061963639
- Reseñas: Digital DJ Tips, DJ TechTools, gearnews (2020-2026)
