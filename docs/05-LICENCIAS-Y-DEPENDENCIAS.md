# 05 · Licencias, dependencias y funciones "con licencia"

## 1. Licencia del proyecto
Jocker-MX se distribuye bajo **GNU General Public License v3.0 o posterior** (`GPL-3.0-or-later`). Todo archivo fuente llevará la cabecera SPDX:
```
// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
```

## 2. Cómo se resuelven las funciones que en VirtualDJ requieren licencia
| Función con licencia en VirtualDJ | Situación en Jocker-MX |
|---|---|
| Uso profesional (cobrar por tocar) | Permitido siempre (la GPL no restringe el uso) |
| Cualquier controlador / definiciones personalizadas | Libre, sin activación ni límite de dispositivos |
| Quitar/cambiar logo en video, sin *audio drop* | Nunca hay marca de agua ni *drop*; logo personalizable |
| IA avanzada (asistente, letras de cualquier canción) | Libre, ejecutada localmente con modelos abiertos |
| Almacenamiento de podcasts | Autoalojado (Jocker Cloud) o el servicio que elija el usuario |
| Licencias multiusuario (Business) | Perfiles ilimitados, instalación en cualquier número de equipos |
| Soporte prioritario | Comunidad + posibilidad de soporte comercial ofrecido por terceros (permitido por la GPL) |
| Catálogos y streaming (iDJPool, Digitrax, VJ-Pro, TIDAL, Deezer, SoundCloud, Beatport, Beatsource) | **No pueden regalarse**: son contenidos de terceros. Se ofrecen como conectores con API oficial cuando el proveedor lo autorice, usando la suscripción del propio usuario |

## 3. Dependencias previstas y compatibilidad con GPL-3.0+
| Componente | Licencia | ¿Compatible? | Notas |
|---|---|---|---|
| Qt 6 | LGPL-3.0 / GPL-3.0 | Sí | Al ser Jocker-MX GPL-3.0+, puede usarse la opción GPL de Qt en todas las plataformas (incluida Android) |
| FFmpeg | LGPL-2.1+ / GPL-2.0+ | Sí | Puede compilarse en modo GPL; evitar `--enable-nonfree` |
| Rubber Band (incluido en `third_party/`) | GPL-2.0+ | Sí | Key lock desde la versión 0.1 |
| SoundTouch | LGPL-2.1 | Sí | |
| qm-dsp | GPL-2.0+ | Sí | |
| libKeyFinder | GPL-3.0 | Sí | |
| Chromaprint | LGPL-2.1 | Sí | |
| libebur128 | MIT | Sí | |
| ONNX Runtime | MIT | Sí | Proveedores CUDA/TensorRT usan bibliotecas propietarias de NVIDIA: opcionales, cargadas en tiempo de ejecución, nunca incluidas |
| llama.cpp / whisper.cpp | MIT | Sí | |
| Piper TTS | GPL-3.0 | Sí | Verificar licencia de cada voz |
| SQLite | Dominio público | Sí | |
| TagLib | LGPL-2.1 / MPL-1.1 | Sí | |
| RtMidi / hidapi | MIT / BSD | Sí | |
| miniaudio (incluido en `third_party/`) | Dominio público / MIT-0 | Sí | E/S de audio y decodificación MP3/WAV/FLAC desde la versión 0.1 |
| Ableton Link | GPL-2.0+ (o comercial) | Sí | |
| libshout | LGPL-2.0+ | Sí | |
| projectM | LGPL-2.1 | Sí | Revisar licencia de cada paquete de presets |
| Oboe | Apache-2.0 | Sí (con GPL-3.0) | |
| CLAP | MIT | Sí | |
| VST3 SDK | MIT (desde v3.8, oct-2025) | Sí | |
| ASIO SDK | GPL-3.0 (opción abierta desde oct-2025) | Sí | Seguir las pautas de marca "ASIO" |
| LV2 / lilv | ISC | Sí | |
| Decodificador de timecode (xwax) | GPL | Sí | |
| Mapeos de Mixxx (importación) | GPL-2.0+ | Sí | Conservar créditos |
| NDI SDK | Propietario | **No** | Sólo como plugin separado opcional, nunca en la distribución principal |
| Bibliotecas de conectores de streaming propietarias | Propietario | **No** | Sólo APIs HTTP públicas/autorizadas |

## 4. Modelos de IA y datos
- Sólo se distribuyen pesos con licencia compatible (MIT, Apache-2.0, BSD, CC-BY, CC0, GPL). Modelos con cláusulas **no comerciales** (p. ej. CC-BY-NC) **no** se incluyen.
- Cada modelo tendrá una "tarjeta de modelo" en `models/` con origen, datos de entrenamiento, licencia, hash y métricas.
- Datos de entrenamiento propios: conjuntos con licencia que permita el uso, contenido CC y donaciones con consentimiento.

## 5. Marcas y "clean room"
- "VirtualDJ", "VDJScript", "Stems 2.0", "ScratchDNA", "AIPrompt", "ShoutOut" y demás son marcas o nombres de Atomix Productions; Jocker-MX usa nombres propios (JockerScript, ScratchSeq, Asistente Jocker, etc.).
- No se copian código, binarios, skins, gráficos, sonidos, efectos, textos ni manuales de VirtualDJ. La compatibilidad (p. ej. importar `database.xml`) se basa en interoperabilidad de formatos de datos del usuario.
- Los nombres de marcas de hardware sólo se usan para indicar compatibilidad.

## 6. Herramientas de cumplimiento
- `reuse lint` en CI (especificación REUSE).
- SBOM en formato SPDX/CycloneDX generado en cada versión.
- Archivo `THIRD_PARTY_NOTICES.md` en cada instalador.
