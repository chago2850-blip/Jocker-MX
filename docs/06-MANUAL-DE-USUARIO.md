# 06 · Manual de usuario — Jocker-MX 0.15.0-dev

## 1. Primer arranque
1. Abre **Jocker-MX**. En la barra superior verás tu tarjeta de sonido, la frecuencia, la latencia y el número de canales de salida.
2. Pulsa **📁 Carpeta** (abajo) y elige dónde está tu música. Se aceptan **MP3, WAV, FLAC, OGG, AAC/M4A y AIFF**.
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

### Secciones de la pista (estructura)
Al cargar una pista con BPM, Jocker-MX reconoce sus partes por compases y las dibuja como una franja de colores debajo de la vista general:
**Intro** (cian), **Cuerpo** (azul), **Break** (verde), **Drop** (rosa) y **Outro** (oro). El nombre de la sección que suena aparece junto al indicador de beats.
- **Clic en un color** salta al inicio de esa sección. Si la pista está sonando y la cuantización (**Q**) está activa, conserva la posición dentro del compás, así que no pierdes el ritmo.
- La detección se basa en la energía y en los graves de cada compás, así que funciona mejor con música electrónica, pop y urbana de estructura marcada. Si el beatgrid está mal, las secciones también; corrígelo y se recalculan al instante.

### Tonalidad y mezcla armónica
Junto al BPM aparece la **tonalidad** en notación Camelot (por ejemplo **8A** = La menor). Si una pista ya trae la tonalidad en sus etiquetas («Am», «F#m», «1m», «8A»…), se usa esa; si no, Jocker-MX la detecta al analizarla. El recuadro se pone **verde** cuando combina con la pista del otro deck: misma clave, ±1 en la rueda (8A ↔ 7A/9A) o su relativa (8A ↔ 8B). En la biblioteca, la columna TONO se pone en verde para las pistas que combinan con lo que está sonando.

### Stems: voz, batería, bajo y melodía (IA)
Al cargar una pista, Jocker-MX la **separa en segundo plano** en cuatro partes con inteligencia artificial que corre en tu equipo (sin Internet). En la fila **STEMS** verás «Separando stems con IA… %» y, al terminar, se activan los botones:
| Botón | Qué hace |
|---|---|
| **VOZ / BATERÍA / BAJO / MELODÍA** | Encienden o apagan esa parte de la canción (el cambio es suave, sin chasquidos) |
| **ACAPELLA** | Deja sólo la voz (otro clic regresa todo) |
| **INSTRUMENTAL** | Quita la voz (otro clic regresa todo) |
Ideas: mezcla la voz de una canción sobre el instrumental de otra (mashup), quita la batería para una transición limpia o deja sólo el bajo y la batería para un *breakdown*.
- Tarda aproximadamente entre 1/8 y 1/4 de la duración de la canción (unos 30-60 s para una de 4 minutos, según tu computadora). Puedes tocar la pista mientras tanto; los botones se activan al terminar.
- Si cargas la misma pista en el otro deck, se reutiliza la separación.
- Atajos: `Shift+Q` / `Shift+W` acapella / instrumental del deck A; `Shift+O` / `Shift+P` del deck B.
- Usa bastante memoria (unos 250 MB por pista de 4 minutos). En equipos con poca memoria puedes desactivar la separación automática (próximamente en Ajustes).

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

### Listas de reproducción
- En la barra izquierda, sección **LISTAS**: **+** crea una lista nueva. Clic en una lista la abre; clic derecho (o **⋯**) para **Reproducir en Automix**, **Agregar a la cola**, **Renombrar** o **Borrar**.
- Para agregar pistas: **arrástralas** sobre el nombre de la lista, o clic derecho en la pista → **Agregar a lista ▸**.
- Dentro de una lista el orden es el tuyo: clic derecho en una pista → **Subir**, **Bajar** o **Quitar de la lista**. Ordenar por columna sólo cambia la vista; vuelve a abrir la lista para ver su orden.
- Las listas guardan la ruta de cada archivo, así que sobreviven a los reescaneos de la carpeta.

### Corregir el beatgrid
Junto al BPM de cada deck: **½** y **×2** (si se detectó a mitad o al doble), **TAP** (pulsa al ritmo varias veces), **◂ / ▸** (mueve la rejilla 10 ms) y **⌖** (marca un beat justo en la posición actual). Una vez corregido, el beatgrid queda guardado y no se vuelve a analizar.

## 5. Sampler
Abre la pestaña **🥁 Sampler** (abajo, junto a Biblioteca). Hay 8 pads con sonidos incluidos: BOCINA, SIRENA, LÁSER, BOMBO, PALMAS, SUBIDA, IMPACTO y REWIND (creados por Jocker-MX, libres de derechos).
- **Clic** (o teclas `Z X C V B N M ,`) dispara el pad. La barra inferior muestra su avance.
- Botón de **modo** en cada pad: **ONE** (suena completo una vez), **LOOP** (se repite hasta que lo vuelvas a tocar) y **HOLD** (suena mientras lo mantienes presionado).
- **Tu propio sonido**: clic derecho en el pad para elegir un archivo (WAV, MP3, FLAC u OGG) o arrástralo sobre el pad. **↺** regresa al sonido incluido. Se recuerda al volver a abrir la app.
- **Volumen** del sampler, **🎧 Audífonos** para escucharlo también en cue y **Detener todo**.
- Los pads, su volumen y el botón de grabar también se pueden asignar a tu controlador MIDI.

## 5 bis. Automix
El Automix toca y mezcla solo la música de su **cola**, usando los decks A y B como lo haría un DJ.
1. Llena la cola: botón **+** verde de cada pista en la biblioteca, clic derecho → **Agregar a la cola de Automix** / **reproducir a continuación**, arrastrar pistas al panel **🔁 Automix**, **Agregar lo visible** (todo lo que muestra la biblioteca ahora) o **▶ Reproducir en Automix** desde una lista.
2. Pulsa el gran botón **AUTOMIX** (o el indicador **AUTO** de la barra superior, o `Ctrl+M`). Si ya hay algo sonando continúa desde ahí; si no, carga la primera pista.
3. Mientras suena una pista, la siguiente se **precarga** en el otro deck. Llegado el punto de mezcla:
   - salta el silencio del final y del principio de cada pista;
   - la salida empieza **al inicio de un compás** y la entrada en el primer beat;
   - si los BPM son compatibles (±8 %, o medio/doble tiempo) **iguala tempo y fase**, cruza el crossfader e **intercambia los graves** a mitad de la mezcla para que no choquen los bajos;
   - si no son compatibles, hace un **fundido** sin sincronizar.
4. **Duración de la mezcla**: **AUTO** (recomendado) usa la estructura: empieza en la **outro** de la pista que sale y arranca la nueva en el punto exacto de su **intro** para que su cuerpo o drop entre justo cuando la anterior termina (hasta 16 compases). Si alguna pista no tiene intro/outro clara, o los tempos no son compatibles, usa el punto fijo. También puedes fijar 8, 16 o 32 beats. Bajo la pista que sigue verás cómo será la mezcla (p. ej. «outro → intro · 8 compases»). **⏩ Mezclar ya** (o `Ctrl+Intro`) mezcla en el siguiente compás; **Saltar** descarta la pista preparada y prepara la siguiente.
5. **Estilo de transición**:
   - **Auto** (recomendado): *Mezcla* si los tempos se pueden sincronizar; *Eco* si no (evita que choquen dos ritmos distintos).
   - **Mezcla**: las dos suenan juntas, sincronizadas, cruzando el crossfader e intercambiando los graves a la mitad.
   - **Filtro**: como la mezcla, pero la que sale se va quedando sin graves (paso alto) y la que entra empieza apagada (paso bajo) y se abre.
   - **Eco**: la que sale suena con eco durante su último compás, se detiene en el compás exacto y la nueva entra de golpe (en su cuerpo, si tiene intro); la cola del eco se apaga de fondo.
   - **Corte**: cambio seco en el compás exacto, sin solapar.
   Al terminar, el Automix deja el EQ, el filtro y el efecto del deck como estaban.
6. Opciones: **Intercambiar graves**, **Volver al tempo original** (tras una mezcla sincronizada, la pista regresa poco a poco a su tempo, ≈1 % cada 10 s) y **Repetir la cola**.
- En la barra superior, **AUTO** muestra cuánto falta para la siguiente mezcla. Puedes tocar los decks, efectos y sampler mientras el Automix trabaja; si pausas el deck que suena, el Automix espera.
- **💾 Guardar lista** guarda la cola como lista de reproducción. La cola se recuerda al cerrar la app.
- El Automix también se puede asignar al controlador MIDI: encender/apagar, mezclar ya y saltar.

## 5 ter. Programar eventos
En el panel **🔁 Automix**, pestaña **⏰ PROGRAMACIÓN**, puedes dejar acciones que se ejecuten solas a una hora:
- **Encender / Apagar Automix**, **Automix: mezclar ya**.
- **Reproducir lista en Automix**: reemplaza la cola por la lista elegida y enciende el Automix; si ya suena algo, mezcla desde ahí.
- **Empezar a grabar / Detener la grabación**.
- **Bajar volumen y parar**: fundido del volumen maestro (10 s a 2 min), luego apaga el Automix y pausa los decks, y deja el volumen como estaba para la próxima vez.

Cada evento tiene: interruptor de activado, hora (escríbela como `21:30`), acción, lista o duración si aplica, **diario** o **1 vez** (se desactiva solo al cumplirse), **▶** para probarlo ahora y **✕** para borrarlo. Arriba se muestra el próximo evento; abajo, el último que se disparó.
- La app tiene que estar abierta. Si estaba cerrada a la hora del evento, no lo ejecuta tarde (sólo hasta 90 segundos después de la hora).
- Los eventos se guardan al cerrar la app.

## 5 quater. Karaoke
**Archivos.** Jocker-MX usa los formatos de karaoke más comunes, que van **al lado** de la canción y con **el mismo nombre**:
- `Cancion.mp3` + `Cancion.cdg` → gráficos **CD+G** (el formato clásico de los discos de karaoke, «MP3+G»).
- `Cancion.mp3` + `Cancion.lrc` → **letra sincronizada** (también el formato con tiempos por palabra).
En la biblioteca esas canciones llevan 🎤, y en el deck aparece el distintivo 🎤 al cargarlas. Si vienen en un .zip, descomprímelo primero en la carpeta de tu música.

**Panel 🎤 Karaoke.**
- **Vista previa** del deck del cantante (gráficos o letra con barrido dorado).
- **TONO −/+**: sube o baja de −6 a +6 semitonos sin cambiar la velocidad, para acomodar la canción a la voz. Al cargar otra canción el tono vuelve a 0.
- **📺 Pantalla del público**: abre una ventana aparte para la TV o el proyector; arrástrala a esa pantalla y haz **doble clic** para ponerla a pantalla completa (Esc para salir). Muestra los gráficos o la letra en grande, quién canta y quién sigue.
- **Rotación**: agrega cantantes arrastrando una canción al panel o desde la biblioteca (clic derecho → **🎤 Agregar a karaoke…**) y escribe su nombre. Cada turno guarda su **tono**; puedes reordenar (▲▼), cambiar el nombre (doble clic) o quitar.
- **SIGUIENTE CANTANTE** (o `Ctrl+K`): carga su canción en el deck libre **desde el principio** y con su tono, sin cortar al cantante actual. Con **Reproducir al llamar** activado, además mueve el crossfader a ese deck y la arranca.
- La rotación se guarda al cerrar la app.

## 5 quinquies. Video
- **Cargar**: los videos (MP4, M4V, MOV, MKV, WebM, AVI…) aparecen en la biblioteca con 🎬 y se cargan como cualquier pista. El deck muestra 🎬 cuando tiene imagen.
- **Sincronía**: el sonido lo reproduce Jocker-MX (con EQ, efectos, stems, key lock y todo lo demás) y la imagen lo sigue: misma velocidad si cambias el tempo, se detiene en pausa, salta con el cue y los hot cues, y en scratch muestra el cuadro exacto.
- **Panel 🎬 Video**: arriba a la izquierda, la **mezcla** que verá el público; a la derecha, la imagen de cada deck. La mezcla sigue al **crossfader**:
  - **Fundido**: una imagen se funde en la otra.
  - **Barrido**: la imagen del deck B entra de izquierda a derecha.
  - **Corte**: cambia de golpe al pasar el crossfader por el centro.
  Si sólo un deck tiene video, se muestra ese aunque el otro esté sonando. **Llenar pantalla** recorta para no dejar bandas negras; **Ajustar** muestra la imagen completa.
- **📺 Pantalla del público**: es la misma ventana que la del karaoke. Muestra el karaoke cuando hay un cantante o suena una pista con gráficos/letra; si no, la mezcla de video; si no, la pantalla de espera.
- Los videos de karaoke con la letra «quemada» en la imagen se usan como cualquier video; el tono (panel Karaoke) también funciona con ellos.

## 5 sexies. Transmitir en vivo (radio por internet)
Necesitas un servidor **Icecast 2** o **SHOUTcast** (propio o contratado con un proveedor de radio por internet), con su dirección, puerto y contraseña de fuente.
1. Abre la pestaña **📡 En vivo** y elige **Icecast 2** o **SHOUTcast**.
2. Escribe la **dirección del servidor**, el **puerto** y:
   - Icecast: el **punto de montaje** (p. ej. `/jocker`), el **usuario** (normalmente `source`) y la **contraseña**.
   - SHOUTcast: la **contraseña** (y, en SHOUTcast 2, el **ID del stream**, normalmente `1`). Jocker-MX envía al puerto siguiente al indicado, como pide SHOUTcast.
3. **Formato**: **Opus** suena mejor con menos datos (96–128 kbps es excelente); **MP3** lo reproduce cualquier aparato. SHOUTcast sólo acepta MP3.
4. Nombre, género, descripción y sitio web aparecen en los reproductores y, si marcas **Emisión pública**, en el directorio del servidor.
5. Pulsa **📡 SALIR EN VIVO**. Se transmite exactamente lo que sale por el master (decks, efectos, sampler). En la barra superior aparece **EN VIVO**.
- Si se cae internet o el servidor, Jocker-MX **se reconecta solo** cada pocos segundos y sigue desde el audio actual.
- **Enviar el título**: manda «Artista - Título» de la pista que más se oye. En MP3 se actualiza en cuanto cambia; en Opus el título va al inicio de cada conexión.
- Errores típicos: «Usuario o contraseña incorrectos», «El punto de montaje está en uso» (ya hay otra fuente conectada), «No se pudo conectar» (dirección o puerto equivocados, o un cortafuegos).
- La configuración (incluida la contraseña) se guarda en este equipo. Desde la línea de comandos, `--broadcast` sale en vivo al abrir la app.

## 5 sexies bis. Radio local (por la Wi-Fi, con código QR)
Para que los asistentes escuchen la mezcla en su celular **sin internet ni servidor**: en una silent disco, en otro salón, en el estacionamiento o simplemente si están lejos de las bocinas.
1. Abre la pestaña **📻 Radio** (en la interfaz táctil: **⋯ Más → 📻 Radio local**).
2. Escribe el nombre de la radio y, si quieres, cambia la calidad (128 kbps va muy bien) o el puerto (8000).
3. Pulsa **ENCENDER RADIO LOCAL**. Aparece un **código QR** con la dirección (p. ej. `http://192.168.1.50:8000/`).
4. Quien esté **en la misma red Wi-Fi** lo escanea con la cámara del celular y toca **▶**. La página muestra lo que suena y cuántas personas escuchan.
- Se transmite lo mismo que sale por el master. Cada persona tiene unos segundos de retraso respecto a las bocinas (normal en cualquier radio por internet).
- También funciona en VLC, en una bocina inteligente o en otra computadora: abre la dirección y agrega `stream.mp3` (o `stream.ogg` para Opus).
- Si los celulares no abren la página: comprueba que estén en la misma Wi-Fi (no en datos móviles) y que el cortafuegos de la computadora permita el puerto. Algunas redes de hoteles o «Wi-Fi de invitados» aíslan a los dispositivos entre sí; ahí no funciona.
- Puedes tener la radio local y la transmisión a Icecast/SHOUTcast al mismo tiempo. Desde la línea de comandos, `--radio` la enciende al abrir.

## 5 septies. Interfaz táctil (teléfono, tableta y TV Box)
En Android se abre directamente la **interfaz táctil**. En la computadora puedes probarla con el botón **📱 TÁCTIL** de la barra superior; para volver, **⋯ Más → 🖥 Escritorio**. La elección se recuerda.
- **Pestañas de abajo**: **🎛 Decks**, **🎵 Música**, **🔁 Automix**, **🥁 Pads** (sampler), **🎤 Karaoke** y **⋯ Más** (video, en vivo, grabar, pantalla del público, MIDI y cambio a escritorio). La barra superior muestra AUTO, REC y EN VIVO cuando están activos (tócalos para ir a su panel).
- **Decks**: arrastra la forma de onda con el dedo para hacer **scratch**; toca la franja fina de abajo para saltar a esa parte. **CUE** (mantén pulsado para escuchar desde el cue), **▶/❚❚**, **SYNC** (mantén pulsado para el sync fijo), hot cues **1–4** (toca para fijar o saltar; mantén pulsado para borrar) y **LOOP 4** (mantén pulsado: loop de 8 o la mitad). Encima de los pads elige su modo:
  - **CUES**: hot cues 1–4 y LOOP 4.
  - **LOOP**: ◀◀/▶▶ saltan 4 beats; **½**, **1**, **4**, **8** y **×2** fijan o cambian el loop (toca el activo para salir).
  - **FX**: ECO, REV (reverb), FLA (flanger), PHA (phaser), CRU (crusher) y GAT (gate); toca para encender/apagar. En pantallas anchas aparece el fader de mezcla del efecto.
  - **STEMS**: VOZ, BAT, BAJO y MEL encienden o apagan cada parte; **ACAP** deja sólo la voz e **INST** la quita. Mientras la IA separa la canción verás «⏳ %»; si no se separó, toca una parte para empezar. Abajo, el **tempo** con **−/+** para empujar el ritmo y el botón de porcentaje: toca para **key lock**, mantén pulsado para volver a 0. En teléfonos en horizontal la fila de tempo se oculta para dar espacio.
- **Mezclador**: agudos, medios, graves y filtro de cada deck (arrastra hacia arriba o abajo; doble toque para centrar), volumen de cada deck y crossfader (doble toque en cualquier fader para volver a su valor normal).
- **Automix**, **Pads** y **Karaoke** tienen su versión táctil: botón grande de encendido del Automix con estilo y duración de la mezcla y la cola con ▲▼✕; 8 pads grandes (mantén pulsado un pad para cambiar su modo: una vez, bucle o mantener) con volumen y PARAR; y en karaoke la vista previa, el **TONO** −/+, la pantalla del público 📺, «SIGUIENTE» y la rotación con el tono de cada cantante (mantén pulsado un nombre para cambiarlo).
- **Música**: mantén pulsada una canción para ver sus acciones: cargar en A o B, Automix (al final o a continuación), **🎤 Karaoke…** (agrega un cantante con esa canción) y agregarla a una lista (o crear una nueva). Busca por título, artista o BPM; elige vista (Todo, Historial, Más tocadas, Recientes o tus listas). Toca una canción para cargarla en el deck libre, o usa **A**, **B** o **+** (cola del Automix). **📁** elige la carpeta y **⚡** analiza BPM y tonalidad.
- **Vertical**: deck A arriba, mezclador en medio y deck B abajo. **Horizontal**: deck A, mezclador y deck B lado a lado.

**Con el control remoto (Android TV / TV Box)**: las **flechas** mueven el recuadro blanco de un botón a otro; **OK** pulsa. Sobre un fader, las flechas en su dirección lo mueven y OK lo devuelve a su valor normal. **Atrás** vuelve a los decks; en los decks, pulsa **Atrás dos veces** para salir. Jocker-MX aparece en el lanzador de Android TV y mantiene la pantalla encendida mientras está abierto.

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

## Android (alfa)
- Instala el archivo `.apk` (Ajustes → permitir instalar apps de orígenes desconocidos para tu navegador o administrador de archivos).
- Al abrirla por primera vez, acepta el permiso de **música y audio**; la biblioteca usa la carpeta **Music** del dispositivo.
- La interfaz es la misma del escritorio: se ve mejor en **tableta en horizontal**; en teléfono puedes desplazarla hacia arriba y abajo. Una interfaz táctil dedicada llegará más adelante.
- Las grabaciones se guardan en la carpeta privada de la app.

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
| `Ctrl+M` | Encender / apagar el Automix |
| `Ctrl+K` | Karaoke: siguiente cantante |
| `Ctrl+Intro` | Automix: mezclar en el siguiente compás |

## 9. Solución de problemas
- **"Sin dispositivo de audio"**: el sistema no encontró tarjeta de sonido; la interfaz funciona pero en silencio. Conecta una salida de audio y reinicia la app.
- **Una pista no carga**: se leen MP3, WAV, FLAC, OGG, AAC/M4A y AIFF. Los archivos con protección de copia (DRM, p. ej. compras antiguas de iTunes o descargas de Apple Music/Spotify) no se pueden abrir. En Linux, AAC/M4A requiere las bibliotecas de FFmpeg (se instalan con el paquete). En Android, por ahora, sólo MP3, WAV, FLAC y OGG.
- **macOS dice que no puede abrir la app**: intenta abrirla una vez, luego ve a *Ajustes del Sistema → Privacidad y seguridad* y pulsa **Abrir de todos modos** (la app aún no está notarizada por Apple).

## 10. Próximamente
Más efectos, guardar stems en disco, video y karaoke, transmisión en vivo e interfaz táctil para teléfono. Ver el [plan de trabajo](04-PLAN-DE-TRABAJO.md).
