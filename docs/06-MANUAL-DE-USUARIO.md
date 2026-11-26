# 06 · Manual de usuario — Jocker-MX 0.1.0-dev

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

## 4. Atajos de teclado
| Tecla | Acción |
|---|---|
| `Q` / `P` | Play/pausa deck A / B |
| `W` / `O` | Sync deck A / B |
| `1 2 3 4` | Hot cues 1-4 del deck A |
| `7 8 9 0` | Hot cues 1-4 del deck B |
| `A` / `L` | Loop de 4 beats en A / B |
| `←` / `→` | Mover el crossfader |

## 5. Solución de problemas
- **"Sin dispositivo de audio"**: el sistema no encontró tarjeta de sonido; la interfaz funciona pero en silencio. Conecta una salida de audio y reinicia la app.
- **Una pista no carga**: en esta versión sólo se leen MP3, WAV y FLAC. Otros formatos (AAC/M4A, OGG, video) llegarán con el decodificador FFmpeg.
- **macOS dice que no puede abrir la app**: intenta abrirla una vez, luego ve a *Ajustes del Sistema → Privacidad y seguridad* y pulsa **Abrir de todos modos** (la app aún no está notarizada por Apple).

## 6. Próximamente
Controladores MIDI, efectos, sampler, stems con IA, video y karaoke, grabación y Android. Ver el [plan de trabajo](04-PLAN-DE-TRABAJO.md).
