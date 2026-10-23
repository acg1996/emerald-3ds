# Plan de dificultad (fork de acg1996)

Documento de diseño para las mejoras de dificultad de este fork. Está escrito
antes de programar nada: recoge lo que se quiere, cómo encaja en el código
actual y lo que queda por decidir. Las decisiones de diseño están cerradas (ver más abajo); todavía no hay nada implementado.

## Objetivo

Hacer Pokémon Esmeralda más difícil y más interesante de jugar en 3DS, con todo
configurable desde una pestaña nueva **DIFICULTAD** en OPTIONS (pantalla
inferior). Con todo apagado, el juego debe comportarse como el original.

Orden de implementación acordado: infraestructura y menú, IVs, level cap, IA,
rediseño de líderes y, **al final del todo, el aleatorizador**.

## Cómo encaja en el código

Lo que he comprobado leyendo el repo:

| Pieza | Dónde está | Consecuencia |
|---|---|---|
| Pestañas de OPTIONS | `3ds_port/include/3ds_extras.h`, `3ds_port/src/3ds_extras.c` | Hay `SETTINGS`, `ENHANCEMENTS` y `CHEATS`. Una pestaña nueva es una entrada más en `enum` y líneas nuevas en la tabla `gCtrExtras`. Los textos van en 4 idiomas con `CTR_TEXT(en, es, fr, it)`. |
| Dónde se guardan los ajustes | `sdmc:/3ds/emerald3ds/settings.txt` (`3ds_settings.c`) | **No van dentro de la partida.** La dificultad es de la consola, no de cada save. Ver decisión D1. |
| Datos de entrenadores | `src/data/trainers.h`, `src/data/trainer_parties.h` | Cada entrenador tiene `aiFlags` y un equipo. El formato admite objeto y movimientos propios, pero **no** EVs, naturaleza ni habilidad. |
| IVs de entrenadores | `battle_main.c`, `CreateNPCTrainerParty` | `fixedIV = iv * 31 / 255`. Es el punto donde forzar IVs altos. |
| Experiencia | `battle_script_commands.c`, `Cmd_getexp` | Punto donde aplicar el level cap. Además hay que cubrir el Caramelo Raro (`item_use.c`). |
| IA | `battle_ai_script_commands.c`, `battle_ai_switch_items.c`, `data/battle_ai_scripts.s` | Ya existe cambio de Pokémon (`ShouldSwitch`) y uso de objetos. Los flags libres son los bits 10 a 28 de `aiFlags`. |
| Encuentros salvajes | `src/wild_encounter.c` y `3ds_port/src/3ds_encounter_tables.c` | El port añade reglas por hora del día que esperan una especie concreta. El aleatorizador tendrá que convivir con ellas. |

### Restricción técnica: las huellas del localizador español

`tools/localize_spanish.py` comprueba el SHA-256 de cada archivo que va a
traducir **antes** de escribir en el árbol. Dos archivos que necesitamos tocar
están en esa lista: `src/battle_main.c` y `src/data/trainers.h`.

Si los parcheamos en el paso normal de parches, la localización fallará con
"Source differs from the pinned patched tree".

**Propuesta:** una carpeta nueva `patches/difficulty/` con parches que
`bootstrap.py` aplica **después** de localizar. Así no se rompe nada del
proyecto original y nuestros cambios quedan separados y fáciles de actualizar
si el upstream cambia. Los archivos que no están en la lista de huellas
(`battle_script_commands.c`, `pokemon.c`, `item_use.c`, `battle_ai_*`) se podrían
tocar en cualquier paso, pero conviene tenerlo todo junto.

## Menú: pestaña DIFICULTAD

Una página nueva en OPTIONS con estas opciones (cada una es una clave en
`settings.txt`):

| Opción | Valores | Clave |
|---|---|---|
| IVs de entrenadores: a quién | Original / Líderes y Alto Mando / Todos | `diff_iv_scope` |
| IVs de entrenadores: calidad | Muy buenos (25 a 31 al azar por estadística) / Perfectos (31) | `diff_iv_level` |
| Level cap | Apagado / Líderes / Líderes y Alto Mando | `diff_level_cap` |
| IA de entrenadores | Original / Mejorada (líderes y Alto Mando) / Mejorada (todos) | `diff_ai` |
| Equipos rediseñados | Apagado / Líderes / Líderes y Alto Mando | `diff_teams` |
| Aleatorizador (fase final) | Apagado / 1 / 2 / 3 / 4 (ver más abajo) | `diff_random` |

## 1. IVs de entrenadores

- **Alcance:** solo líderes y Alto Mando, o todos los entrenadores.
- **Calidad:** "muy buenos" (cada estadística al azar entre 25 y 31) o
  "perfectos" (31 en todas).
- **Cómo:** `CreateMon` solo admite un IV igual para las seis estadísticas.
  Para los muy buenos, tras crear el Pokémon se escribe cada IV por separado
  con `SetMonData`. En `CreateNPCTrainerParty` se llama a una función que mira
  los dos ajustes. Para decidir si un entrenador es
  líder o Alto Mando se usa `gTrainers[id].trainerClass`
  (`TRAINER_CLASS_LEADER`, `TRAINER_CLASS_ELITE_FOUR`, `TRAINER_CLASS_CHAMPION`),
  que cubre también las revanchas.
- No afecta a la Zona de Combate ni a las bases secretas, que usan sus propios
  equipos (D7).
- Los IVs aleatorios deben ser deterministas por combate, para que reiniciar
  un combate no cambie al rival. Se derivan de la personalidad del Pokémon.

## 2. Level cap

- **Regla:** el tope de nivel es el nivel del Pokémon **más fuerte** del
  siguiente líder cuya medalla aún no tienes. Hasta ganar la medalla, tus
  Pokémon no suben por encima.
- **Cómo se calcula:** en tiempo de ejecución, leyendo el equipo del líder en
  `gTrainers`. Así el tope sigue solo al rediseño de líderes, sin tablas
  duplicadas que mantener.
- **Orden de líderes:** Roxanne, Brawly, Wattson, Flannery, Norman, Winona,
  Tate y Liza, Wallace. Se sabe cuál toca por las medallas conseguidas.
- **Alto Mando:** el tope pasa a ser el Pokémon más fuerte del **siguiente
  miembro** no derrotado (Sidney, Phoebe, Glacia, Drake y Steven), usando las
  flags de derrotado de cada uno. Si resultara frágil, el plan B es usar solo
  al último.
- **Qué pasa al llegar al tope:** el Pokémon deja de ganar experiencia
  (sin perder la que ya tiene). Esto cubre `Cmd_getexp`, el Repartir
  Experiencia y el Caramelo Raro. Ver D2 para los EVs.
- Cuando se vence al campeón, el tope se desactiva. Ver D4.

## 3. Rediseño de líderes y Alto Mando

Equipos completos con sinergia: movimientos, objetos equipados, naturaleza,
habilidad y EVs. Hay que ampliar el formato porque el actual no guarda
naturaleza, habilidad ni EVs.

- **Datos:** una tabla nueva en un archivo propio (por ejemplo
  `3ds_port/src/3ds_trainer_overrides.c`), indexada por ID de entrenador. Se
  aplica justo después de crear el equipo, con `CreateMonWithNature`,
  `SetMonData` para habilidad y EVs, y objeto y movimientos del equipo. Así no
  hay que reescribir `trainers.h` (que está protegido por huella).
- **Revanchas:** los líderes tienen versiones `_1` a `_5`. Hay que decidir si
  las revanchas también se rediseñan. Ver D5.
- **Lo que necesito de ti:** el equipo de cada líder. Puedo proponer una
  primera versión por líder, con la sinergia explicada, para que la corrijas.
  Lo trabajamos líder por líder y no todos de golpe.

## 4. IA de entrenadores más avanzada

Objetivo: cambios de Pokémon cuando conviene, movimientos de apoyo
("stack": subidas de estadísticas como Danza Espada o Paz Mental), uso
inteligente de objetos.

- Ya existe una base: `ShouldSwitch` y los flags `SETUP_FIRST_TURN`,
  `PREFER_POWER_EXTREMES`, `HP_AWARE`.
- Se añadirían flags nuevos en los bits libres de `aiFlags` y se activarían
  según el ajuste `diff_ai`.
- Mejoras concretas candidatas: cambiar cuando el rival tiene ventaja de tipo
  clara, no usar movimientos de subida si el rival puede noquear de un golpe,
  preferir el movimiento más fuerte cuando acaba al rival, usar los
  movimientos de apoyo cuando el Pokémon es seguro.
- **Zona de Combate y bases secretas:** sus equipos y sus IVs no se tocan, pero
  la IA mejorada sí se aplica. Sus flags se fijan en una sola línea de
  `battle_ai_script_commands.c` (archivo sin huella), así que basta con sumar los
  flags nuevos cuando `diff_ai` lo pida.
- Es la parte más difícil de afinar: se prueba combatiendo, no solo
  compilando. Se hará en iteraciones cortas.

## 5. Aleatorizador (última fase)

| Modo | Qué aleatoriza |
|---|---|
| 1 | Solo Pokémon salvajes de ruta |
| 2 | Rutas y entrenadores |
| 3 | Rutas, entrenadores y líderes |
| 4 | Todo, más habilidades y ataques |

Puntos delicados:

- **Semilla:** tiene que ser fija y guardarse (en `settings.txt`) para que el
  mismo save vea siempre lo mismo. Ver D1.
- **Reglas por hora del día:** `3ds_encounter_tables.c` espera especies
  concretas. Hay que decidir si el aleatorizador las sustituye o las respeta.
- **Progresión:** el juego exige Pokémon que aprendan movimientos de MO
  (Corte, Surf, etc.). Con aleatorización completa puede haber que garantizar
  que se pueda avanzar.
- **Regalos, estáticos y legendarios:** iniciales, huevos, Rayquaza, etc. Hay
  que decidir si entran.
- **Modo 4 (habilidades y ataques):** es el más invasivo. Afecta a
  movimientos aprendidos y a la IA. Se deja para el final.
- Con rediseño de líderes activo, el modo 3 y el 4 deben decidir quién gana:
  el equipo diseñado o el aleatorio. Ver D6.

## Fases

1. **Infraestructura y menú:** carpeta `patches/difficulty/`, paso nuevo en
   `bootstrap.py`, pestaña DIFICULTAD vacía pero visible, textos en 4 idiomas.
2. **IVs:** el cambio más pequeño; sirve para validar todo el ciclo.
3. **Level cap:** `Cmd_getexp`, Caramelo Raro y tope por medallas.
4. **IA mejorada.**
5. **Rediseño de líderes** (de Roxanne a Wallace) y después Alto Mando.
6. **Aleatorizador**, modos 1 a 4 en orden.

Cada fase termina con una compilación, una prueba (emulador Azahar o 3DS real)
y un commit.

## Decisiones tomadas

- **D1: ajustes globales.** La dificultad vive en `settings.txt`, como el resto
  de ajustes del port, y se puede cambiar en cualquier momento.
- **D2: EVs al llegar al tope.** Se siguen ganando; solo se bloquea la
  experiencia.
- **D3: IVs "muy buenos".** Cada estadística al azar entre 25 y 31.
- **D4: Alto Mando y campeón.** El tope sigue al siguiente miembro no derrotado,
  con Steven como último. Al vencer al campeón el tope se desactiva.
- **D5: revanchas.** Los líderes rediseñados también en las revanchas, con
  equipos más fuertes y el mismo estilo.
- **D6: aleatorizador contra equipos diseñados.** Si el aleatorizador de líderes
  está activo, manda el aleatorio y el equipo diseñado se ignora.
- **D7: Zona de Combate y bases secretas.** Sus equipos e IVs quedan como están,
  pero reciben la IA mejorada.

## Riesgos

- **Mantenimiento con el upstream:** el proyecto de Zallax avanza deprisa. Por
  eso nuestros cambios van en parches aparte y en archivos propios, y se
  procura tocar poco código existente.
- **Compilación local:** necesita Smart App Control desactivado en Windows,
  porque devkitPro no firma sus ejecutables.
- **Pruebas:** los cambios de combate y de IA solo se validan jugando.
