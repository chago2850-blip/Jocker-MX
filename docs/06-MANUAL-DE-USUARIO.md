# 06 · Manual de usuario — Jocker-MX 0.2.0-dev

## 1. Primer arranque
1. Abre **Jocker-MX**. En la barra superior verás tu tarjeta de sonido, la frecuencia, la latencia y el número de canales de salida.
2. Pulsa **📁 Carpeta** (abajo) y elige dónde está tu música. Se aceptan **MP3, WAV y FLAC**.
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
- El BPM, el beatgrid, el cue y los hot cues de cada pista se guardan en la biblioteca (`biblioteca.sqlite` en la carpeta de datos de la app).

### Corregir el beatgrid
Junto al BPM de cada deck: **½** y **×2** (si se detectó a mitad o al doble), **TAP** (pulsa al ritmo varias veces), **◂ / ▸** (mueve la rejilla 10 ms) y **⌖** (marca un beat justo en la posición actual). Una vez corregido, el beatgrid queda guardado y no se vuelve a analizar.

## 5. Controladores MIDI
1. Conecta tu controlador por USB. En la barra superior el botón **MIDI** se pone verde y muestra cuántos dispositivos hay; el punto parpadea con cada mensaje recibido.
2. Pulsa **MIDI** para abrir los ajustes. Junto a cada acción pulsa **Aprender** y mueve el control de tu equipo: queda asignado.
3. Para el **plato**: asigna «Plato (jog)» girándolo y «Toque del plato» tocando su superficie. Con el plato tocado hace *scratch*; girando el aro, empuja o frena. Si el scratch va muy rápido o lento, ajusta **Pasos por vuelta**.
4. El mapeo se guarda automáticamente (`mapeo-midi.json`) y se recuerda al volver a abrir la app.

Próximamente: mapeos listos para los controladores más populares y luces (LED) del controlador.

## 6. Atajos de teclado
| Tecla | Acción |
|---|---|
| `Q` / `P` | Play/pausa deck A / B |
| `W` / `O` | Sync deck A / B |
| `1 2 3 4` | Hot cues 1-4 del deck A |
| `7 8 9 0` | Hot cues 1-4 del deck B |
| `A` / `L` | Loop de 4 beats en A / B |
| `←` / `→` | Mover el crossfader |

## 7. Solución de problemas
- **"Sin dispositivo de audio"**: el sistema no encontró tarjeta de sonido; la interfaz funciona pero en silencio. Conecta una salida de audio y reinicia la app.
- **Una pista no carga**: en esta versión sólo se leen MP3, WAV y FLAC. Otros formatos (AAC/M4A, OGG, video) llegarán con el decodificador FFmpeg.
- **macOS dice que no puede abrir la app**: intenta abrirla una vez, luego ve a *Ajustes del Sistema → Privacidad y seguridad* y pulsa **Abrir de todos modos** (la app aún no está notarizada por Apple).

## 8. Próximamente
Efectos, sampler, stems con IA, video y karaoke, grabación y Android. Ver el [plan de trabajo](04-PLAN-DE-TRABAJO.md).
