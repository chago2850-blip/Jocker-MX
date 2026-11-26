# 06 · Manual de usuario — Jocker-MX 0.4.0-dev

## 1. Primer arranque
1. Abre **Jocker-MX**. En la barra superior verás tu tarjeta de sonido, la frecuencia, la latencia y el número de canales de salida.
2. Pulsa **📁 Carpeta** (abajo) y elige dónde está tu música. Se aceptan **MP3, WAV, FLAC y OGG**.
3. Carga una pista con los botones **A** o **B** de cada fila, con **doble clic** (va al deck libre) o **arrastrándola** sobre un deck. También puedes arrastrar archivos desde el explorador de tu sistema.
4. Al cargar, la pista se **analiza** (BPM, beatgrid y forma de onda). El BPM queda guardado y aparece en el navegador.

## 2. Los decks
| Control | Función |
|---|---|
| **▶ / ❚❚** | Reproducir / pausar |
| **CUE** | Reproduciendo: regresa al punto cue y se detiene. Detenido en otro lugar: fija ahí el cue. Detenido sobre el cue: **mantenlo pulsado** para escuchar y suelta para regresar |
| **SYNC** | Iguala el tempo y alinea los beats con el otro deck |
| **LOCK** | Sync permanente: sigue los cambios de tempo del otro deck |
| **KEY** | *Key lock*: cambia el tempo sin cambiar el tono |
| **Q** | Cuantizar: cues y loops caen exactamente en los beats |
| **±8 %** | Cambia el rango del fader de pitch (8, 16, 25, 50, 100 %) |
| **Fader de pitch** | Arriba = más lento, abajo = más rápido. Doble clic = 0 % |
| **− / +** | Empujón temporal para corregir la fase a mano |
| **Plato** | Arrastra el **centro** para hacer *scratch*; arrastra el **aro** para empujar o frenar; la rueda del ratón salta ¼ de beat |
| **HOT CUE 1-8** | Clic: crea el hot cue (si está vacío) o salta a él. **Clic derecho**: borrarlo. Se guardan por pista |
| **LOOP ½…16** | Loop automático de N beats (otro clic lo quita). **÷2 / ×2** acortan o alargan. **◀ / ▶** saltan 4 beats |
| **Vista general** | Clic o arrastre para ir a cualquier parte de la pista |

Las **formas de onda grandes** (arriba) muestran graves en magenta, medios en oro y agudos en cian, con líneas de beat (más brillantes cada compás), cues y loops. Arrástralas para hacer scratch fino; la rueda del ratón cambia el zoom.

### Tonalidad y mezcla armónica
Junto al BPM aparece la **tonalidad** en notación Camelot (por ejemplo **8A** = La menor). Si una pista ya trae la tonalidad en sus etiquetas («Am», «F#m», «1m», «8A»…), se usa esa; si no, Jocker-MX la detecta al analizarla. El recuadro se pone **verde** cuando combina con la pista del otro deck: misma clave, ±1 en la rueda (8A ↔ 7A/9A) o su relativa (8A ↔ 8B). En la biblioteca, la columna TONO se pone en verde para las pistas que combinan con lo que está sonando.

### Efectos (FX)
Cada deck tiene una fila **FX** con seis efectos, todos sincronizados al beat de la pista:
| Efecto | Qué hace | El botón de beats controla |
|---|---|---|
| **ECO** | Repite el sonido; al apagarlo (o pausar) la cola se desvanece | El tiempo entre repeticiones |
| **REVERB** | Sala/espacio | — (la perilla controla tamaño y mezcla) |
| **FLANGER** | Barrido metálico tipo «avión» | La duración del barrido |
| **PHASER** | Barrido suave de fase | La duración del barrido |
| **CRUSH** | Reduce bits y frecuencia (sonido «retro») | — |
| **GATE** | Corta el sonido en ritmo (trance gate) | El tamaño de cada corte |
Clic en un efecto lo enciende (otro clic lo apaga). El botón numérico cambia la duración (¼, ½, 1, 2, 4, 8 beats; clic derecho para bajar) y la perilla naranja la **cantidad**.

## 3. El mezclador
- **GAIN**: ganancia de entrada (±12 dB).
- **HIGH / MID / LOW**: ecualizador. Al fondo a la izquierda la banda se **elimina por completo** (*kill*, la perilla se pone roja).
- **FILTER**: a la izquierda filtro pasa-bajos, a la derecha pasa-altos. Centro = apagado.
- **🎧**: manda el canal a los audífonos (PFL).
- **Fader de canal** y **crossfader** (doble clic lo centra).
- **MASTER**: volumen general; **CUE/MST** mezcla lo que oyes en audífonos; **PHONES** volumen de audífonos; **LIMIT** se enciende cuando el limitador protege la salida de saturar.
- **Audífonos**: funcionan con tarjetas de **4 salidas** (salidas 3-4) o con **SPLIT** activo en tarjetas estéreo (izquierda = audífonos, derecha = master en mono, con un cable divisor).

## 4. La biblioteca
- **Carpeta…** elige dónde está tu música. Jocker-MX la escanea en segundo plano y la vuelve a revisar cada vez que abres la app (sólo lee archivos nuevos o modificados). **⟳** fuerza un nuevo escaneo.
- Lee las **etiquetas** de tus archivos (título, artista, álbum, género, BPM y tonalidad). Si un archivo no tiene etiquetas, usa el nombre «Artista - Título».
- **Vistas** (barra izquierda): *Toda la música*, *Historial* (lo que has tocado, con fecha y hora), *Más tocadas* y *Agregadas* recientemente.
- **Ordenar**: clic en el encabezado de una columna; otro clic invierte el orden.
- **Buscar**: escribe palabras de título, artista, álbum o género; no importan acentos ni mayúsculas. Filtros especiales: `bpm:120-128` (rango de tempo), `bpm:124` y `key:8A`.
- **⚡ Analizar todo** calcula en segundo plano el BPM, la tonalidad y la duración de las pistas que aún no se han analizado (puedes seguir mezclando; pulsa otra vez para detenerlo).
- El BPM, el beatgrid, el cue y los hot cues de cada pista se guardan en la biblioteca (`biblioteca.sqlite` en la carpeta de datos de la app).

### Corregir el beatgrid
Junto al BPM de cada deck: **½** y **×2** (si se detectó a mitad o al doble), **TAP** (pulsa al ritmo varias veces), **◂ / ▸** (mueve la rejilla 10 ms) y **⌖** (marca un beat justo en la posición actual). Una vez corregido, el beatgrid queda guardado y no se vuelve a analizar.

## 5. Sampler
Abre la pestaña **🥁 Sampler** (abajo, junto a Biblioteca). Hay 8 pads con sonidos incluidos: BOCINA, SIRENA, LÁSER, BOMBO, PALMAS, SUBIDA, IMPACTO y REWIND (creados por Jocker-MX, libres de derechos).
- **Clic** (o teclas `Z X C V B N M ,`) dispara el pad. La barra inferior muestra su avance.
- Botón de **modo** en cada pad: **ONE** (suena completo una vez), **LOOP** (se repite hasta que lo vuelvas a tocar) y **HOLD** (suena mientras lo mantienes presionado).
- **Tu propio sonido**: clic derecho en el pad para elegir un archivo (WAV, MP3, FLAC u OGG) o arrástralo sobre el pad. **↺** regresa al sonido incluido. Se recuerda al volver a abrir la app.
- **Volumen** del sampler, **🎧 Audífonos** para escucharlo también en cue y **Detener todo**.
- Los pads, su volumen y el botón de grabar también se pueden asignar a tu controlador MIDI.

## 6. Grabar tu mezcla
Pulsa **REC** en la barra superior (o `Ctrl+R`). El punto rojo parpadea y verás el tiempo y cuántas pistas van. Al detenerla, en la carpeta **Música/Jocker-MX Grabaciones** quedan tres archivos con la fecha y hora:
- `Mezcla … .wav`: tu mezcla completa en WAV de 24 bits (lo que sale por el master, con efectos y sampler).
- `Mezcla … .txt`: la **lista de canciones** con el minuto en que entró cada una.
- `Mezcla … .cue`: hoja CUE para quemar un CD o dividir la mezcla en pistas con otros programas.
Un WAV de 24 bits ocupa unos 17 MB por minuto (alrededor de 1 GB por hora).

## 7. Controladores MIDI
1. Conecta tu controlador por USB. En la barra superior el botón **MIDI** se pone verde y muestra cuántos dispositivos hay; el punto parpadea con cada mensaje recibido.
2. Pulsa **MIDI** para abrir los ajustes. Junto a cada acción pulsa **Aprender** y mueve el control de tu equipo: queda asignado.
3. Para el **plato**: asigna «Plato (jog)» girándolo y «Toque del plato» tocando su superficie. Con el plato tocado hace *scratch*; girando el aro, empuja o frena. Si el scratch va muy rápido o lento, ajusta **Pasos por vuelta**.
4. El mapeo se guarda automáticamente (`mapeo-midi.json`) y se recuerda al volver a abrir la app.

Próximamente: mapeos listos para los controladores más populares y luces (LED) del controlador.

## 8. Atajos de teclado
| Tecla | Acción |
|---|---|
| `Q` / `P` | Play/pausa deck A / B |
| `W` / `O` | Sync deck A / B |
| `1 2 3 4` | Hot cues 1-4 del deck A |
| `7 8 9 0` | Hot cues 1-4 del deck B |
| `A` / `L` | Loop de 4 beats en A / B |
| `←` / `→` | Mover el crossfader |
| `Z X C V B N M ,` | Pads 1-8 del sampler |
| `Ctrl+R` | Iniciar / detener la grabación |

## 9. Solución de problemas
- **"Sin dispositivo de audio"**: el sistema no encontró tarjeta de sonido; la interfaz funciona pero en silencio. Conecta una salida de audio y reinicia la app.
- **Una pista no carga**: en esta versión se leen MP3, WAV, FLAC y OGG. AAC/M4A y video llegarán con el decodificador FFmpeg.
- **macOS dice que no puede abrir la app**: intenta abrirla una vez, luego ve a *Ajustes del Sistema → Privacidad y seguridad* y pulsa **Abrir de todos modos** (la app aún no está notarizada por Apple).

## 10. Próximamente
Más efectos, stems con IA, AAC/M4A, video y karaoke, grabación y Android. Ver el [plan de trabajo](04-PLAN-DE-TRABAJO.md).
