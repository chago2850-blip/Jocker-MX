# Cómo contribuir a Jocker-MX

¡Gracias por tu interés! Jocker-MX es software libre bajo **GPL-3.0-or-later**.

1. Revisa la [matriz de funciones](docs/02-MATRIZ-DE-FUNCIONES.md) y el [plan de trabajo](docs/04-PLAN-DE-TRABAJO.md) para elegir una tarea (usa su ID, p. ej. `A05`, en el título del *issue* o PR).
2. Cada archivo nuevo debe llevar cabecera SPDX:
   `SPDX-License-Identifier: GPL-3.0-or-later`.
3. **Clean room:** no copies código, gráficos, sonidos, skins ni textos de VirtualDJ u otro software propietario. Describe comportamientos, no los copies.
4. Dependencias nuevas: verifica que su licencia aparezca como compatible en [05-LICENCIAS-Y-DEPENDENCIAS](docs/05-LICENCIAS-Y-DEPENDENCIAS.md).
5. Código del hilo de audio: sin reservas de memoria, sin *locks*, sin E/S.
6. Mapeos de controladores: carpeta `controllers/<marca>/<modelo>/` con `controller.json`, `controller.js` y una foto/diagrama propio.

Idioma del proyecto: español (documentación también en inglés).
