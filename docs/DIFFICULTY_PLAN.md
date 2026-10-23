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
  Tate y Liza, Juan. Se sabe cuál toca por las medallas conseguidas.
- **Alto Mando:** el tope pasa a ser el Pokémon más fuerte del **siguiente
  miembro** no derrotado (Sidney, Phoebe, Glacia y Drake) y, por último, la campeona o el campeón Wallace, usando las
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
5. **Rediseño de líderes** (de Roxanne a Juan) y después Alto Mando.
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
  con Wallace (el campeón de Esmeralda) como último. Al vencer al campeón el tope se desactiva.
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

## Equipos decididos

Cada equipo se decide con el usuario, líder por líder. La tabla está en
`3ds_port/src/3ds_trainer_teams.c` y el tope de nivel lee de ahí.

**Roxanne** (control de velocidad y aguante; el as sigue en nivel 15, como el original, así que el
tope antes de su medalla es 15). Rollout, Garra Metal, Corte Furia y Onda
Trueno van por delante de lo que esas especies aprenden a esos niveles.

| Pokémon | Nv | Habilidad | Naturaleza | Objeto | Movimientos |
|---|---|---|---|---|---|
| Geodude | 12 | Cabeza Roca | Agitada | Baya Zidra | Rollout, Rizo Defensa, Tumba Rocas, Lanzarrocas |
| Aron | 12 | Robustez | Firme | Cinta Focus | Garra Metal, Tumba Rocas, Fortaleza, Cabezazo |
| Anorith | 13 | Armadura Batalla | Firme | Garra Rápida | Garra Metal, Tumba Rocas, Corte Furia, Fortaleza |
| Nosepass | 15 | Robustez | Plácida | Baya Aranja | Bloqueo, Tumba Rocas, Onda Trueno, Fortaleza |

Todos con IV 20 y 40 EVs en dos estadísticas, mientras el ajuste de IVs sea
"originales". Pendiente: sus revanchas (D5).

**Brawly** (Bulk Up y críticos; el as sigue en el nivel 19 del original, así
que el tope antes de su medalla es 19). Patada Salto Alta, Reversión y Ataque
Rápido de Hitmontop van por delante de lo que esas especies aprenden a esos
niveles. El Makuhita usa la segunda habilidad, Agallas.

| Pokémon | Nv | Habilidad | Naturaleza | Objeto | Movimientos |
|---|---|---|---|---|---|
| Machop | 16 | Agallas | Firme | Lupa | Golpe Kárate, Sismo Toss, Pico Impulso, Tumba Rocas |
| Meditite | 16 | Energía Pura | Alegre | Baya Lum | Patada Salto Alta, Pico Impulso, Confusión, Detección |
| Hitmontop | 17 | Intimidación | Alegre | Cinturón Negro | Patada Giro, Venganza, Persecución, Ataque Rápido |
| Makuhita | 19 | Agallas | Firme | Baya Salac | Reversión, Pico Impulso, Llave Vital, Empujón |

**Rival, Ruta 110** (Brendan o May: el mismo equipo; la variante depende del
inicial del jugador; niveles originales 18, 18 y 20). El rival cuenta con los
líderes en los ajustes *IV entrenadores*, *IA entrenadores* y *Equipos nuevos*
(valor "Líderes"). Electrike es un Pokémon nuevo común a las tres variantes.

| Si eliges | Equipo del rival |
|---|---|
| Treecko | Wingull 18, Electrike 18, Lombre 18, Combusken 20 |
| Mudkip | Slugma 18, Wingull 18, Electrike 18, Grovyle 20 |
| Torchic | Lombre 18, Slugma 18, Electrike 18, Marshtomp 20 |

Pendientes del rival: Ruta 103, Petalia, Ruta 119 y Lilycove.

## Equipos acordados, pendientes de implementar

Se decidió acordarlos todos antes de compilar. Niveles originales; el as
mantiene el nivel original, que es el tope.

**Wattson** (parálisis y golpes seguros; 5 Pokémon, tope 24)

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Voltorb | 20 | Electricidad Estática | Baya Zidra | Chispa, Onda Trueno, Chirrido, Autodestrucción |
| Plusle | 20 | Más | Baya Aranja | Chispa, Onda Trueno, Ataque Rápido, Otra Vez |
| Magneton | 22 | Imán | Imán | Rayo, Onda Trueno, Supersónico, Chispa |
| Electabuzz | 23 | Electricidad Estática | Restos | Puño Trueno, Ataque Rápido, Pantalla de Luz, Rapidez |
| Manectric | 24 | Pararrayos | Baya Lum | Rayo, Ataque Rápido, Onda Trueno, Mordisco |

**Flannery** (equipo de sol; 5 Pokémon, tope 29)

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Slugma | 24 | Cuerpo Llama | Baya Zidra | Día Soleado, Recalentón, Lanzarrocas, Bostezo |
| Numel | 24 | Despiste | Baya Aranja | Recalentón, Magnitud, Derribo, Amnesia |
| Magmar | 25 | Cuerpo Llama | Campana Concha | Puño Fuego, Puño Trueno, Polución, Pantalla de Humo |
| Camerupt | 26 | Armadura Magma | Restos | Recalentón, Terremoto, Avalancha, Amnesia |
| Torkoal | 29 | Humo Blanco | Hierba Blanca | Recalentón, Lanzallamas, Golpe Cuerpo, Protección |

**Norman** (Facade y cobertura; 5 Pokémon, tope 31)

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Spinda | 27 | Ritmo Propio | Pañuelo Seda | Facade, Psicorrayo, Puño Mareo, Otra Vez |
| Vigoroth | 27 | Espíritu Vital | Garra Rápida | Facade, Cuchillada, Demolición, Pico Impulso |
| Kecleon | 28 | Cambio Color | Roca del Rey | Ladrón, Psicorrayo, Finta, Chirrido |
| Linoone | 29 | Recogida | Baya Salac | Tambor, Cuchillada, Facade, Cabezazo |
| Slaking | 31 | Truant | Restos | Facade, Respiro, Contraataque, Finta |

**Winona** (Volador con cobertura, sol y Danza Dragón; 5 Pokémon, tope 33)

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Swablu | 29 | Cura Natural | Baya Zidra | Ataque Aéreo, Canto, Salvaguarda, Canto Mortal |
| Tropius | 29 | Clorofila | Semilla Milagro | Día Soleado, Rayo Solar, Ataque Aéreo, Síntesis |
| Pelipper | 30 | Vista Lince | Agua Mística | Surf, Ataque Aéreo, Protección, Supersónico |
| Skarmory | 31 | Vista Lince | Pico Afilado | Ataque Aéreo, Ala de Acero, Agilidad, Tóxico |
| Altaria | 33 | Cura Natural | Baya Lum | Danza Dragón, Terremoto, Garra Dragón, Ataque Aéreo |

**Tate y Liza** (sol y luna, inmunes a Terremoto; combate doble; 5 Pokémon, tope 42)

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Claydol | 41 | Levitación | Restos | Terremoto, Poder Antiguo, Psíquico, Giro Rápido |
| Xatu | 41 | Sincronía | Cuchara Retorcida | Psíquico, Paz Mental, Deseo, Rayo Confuso |
| Lunatone | 42 | Levitación | Baya Zidra | Psíquico, Paz Mental, Hipnosis, Bola Sombra |
| Solrock | 42 | Levitación | Semilla Milagro | Día Soleado, Rayo Solar, Avalancha, Psíquico |
| Gardevoir | 42 | Rastro | Baya Lum | Paz Mental, Psíquico, Bola Sombra, Rayo |

**Juan** (lluvia con Nado Rápido; 5 Pokémon, tope 46)

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Whiscash | 41 | Despiste | Baya Zidra | Danza Lluvia, Surf, Terremoto, Amnesia |
| Luvdisc | 41 | Nado Rápido | Baya Salac | Surf, Flail, Agilidad, Atracción |
| Sealeo | 43 | Sebo | Baya Chesto | Rayo Hielo, Surf, Descanso, Golpe Cuerpo |
| Crawdaunt | 43 | Corte Vigor | Baya Liechi | Danza Espada, Martillazo, Desarme, Mofa |
| Kingdra | 46 | Nado Rápido | Agua Mística | Hidrobomba, Rayo Hielo, Aliento Dragón, Agilidad |

**Sidney** (trampas y desgaste; 5 Pokémon, tope 49; Sharpedo sustituye a Crawdaunt para no repetir el de Juan)

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Mightyena | 46 | Intimidación | Gafas Negras | Triturar, Doble Filo, Mofa, Rugido |
| Shiftry | 48 | Clorofila | Baya Lum | Danza Espada, Finta, Paranormal, Torbellino |
| Cacturne | 46 | Velo Arena | Restos | Púas, Brazo Aguja, Finta, Drenadoras |
| Sharpedo | 48 | Piel Tosca | Agua Mística | Triturar, Surf, Rayo Hielo, Mofa |
| Absol | 49 | Presión | Lupa | Danza Espada, Cuchillada, Ataque Aéreo, Avalancha |

**Phoebe** (Mal de Ojo, estados y Garantía; 5 Pokémon, tope 51; Shedinja y Gengar sustituyen a los repetidos)

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Shedinja | 48 | Garantía | Baya Lum | Bola Sombra, Rayo Confuso, Protección, Rencor |
| Banette | 49 | Insomnio | Hechizo | Bola Sombra, Finta, Fuego Fatuo, Desarme |
| Gengar | 49 | Levitación | Campana Concha | Hipnosis, Comesueños, Bola Sombra, Rayo |
| Sableye | 50 | Vista Lince | Polvo Brillo | Mal de Ojo, Bola Sombra, Rayo Confuso, Desarme |
| Dusclops | 51 | Presión | Restos | Mal de Ojo, Fuego Fatuo, Bola Sombra, Terremoto |

**Glacia** (granizo; 5 Pokémon, tope 53; sin los repetidos del original)

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Glalie | 50 | Fuerza Mental | Antiderretir | Granizo, Rayo Hielo, Triturar, Protección |
| Sneasel | 50 | Vista Lince | Lupa | Danza Espada, Cuchillada, Finta, Ataque Rápido |
| Jynx | 52 | Despiste | Baya Lum | Rayo Hielo, Psíquico, Beso Amoroso, Paz Mental |
| Lapras | 52 | Armadura Concha | Restos | Rayo Hielo, Surf, Rayo, Rayo Confuso |
| Walrein | 53 | Sebo | Baya Chesto | Rayo Hielo, Surf, Golpe Cuerpo, Descanso |

**Drake** (dragones que cubren todo, con Latios; 6 Pokémon, tope 55). En Gen 3
el tipo Dragón es especial, así que Danza Dragón se usa con ataques físicos de
otros tipos. Latios es un legendario usado como pieza "loca" y Tyranitar es
opcional.

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Flygon | 53 | Levitación | Arena Suave | Terremoto, Avalancha, Garra Dragón, Chirrido |
| Gyarados | 53 | Intimidación | Baya Lum | Danza Dragón, Azote, Terremoto, Avalancha |
| Dragonite | 54 | Fuerza Mental | Restos | Onda Trueno, Agilidad, Terremoto, Ataque Aéreo |
| Latios | 54 | Levitación | Rocío Bondad | Psíquico, Paz Mental, Recuperación, Garra Dragón |
| Tyranitar | 54 | Chorro Arena | Campana Concha | Tempestad Arena, Avalancha, Terremoto, Azote |
| Salamence | 55 | Intimidación | Colmillo Dragón | Garra Dragón, Vuelo, Terremoto, Lanzallamas |

**Wallace** (lluvia total con Kyogre; 6 Pokémon, tope 58). Kyogre es un
legendario usado como pieza "loca". Whiscash y Gyarados del original se
cambian porque los usan Juan y Drake.

| Pokémon | Nv | Habilidad | Objeto | Movimientos |
|---|---|---|---|---|
| Wailord | 57 | Velo Agua | Baya Chesto | Danza Lluvia, Water Spout, Rayo Hielo, Descanso |
| Tentacruel | 55 | Cuerpo Puro | Flecha Venenosa | Hidrobomba, Bomba Lodo, Rayo Hielo, Tóxico |
| Ludicolo | 56 | Nado Rápido | Baya Lum | Surf, Gigadrenado, Rayo Hielo, Drenadoras |
| Starmie | 56 | Cura Natural | Baya Salac | Surf, Psíquico, Trueno, Recuperación |
| Kyogre | 57 | Llovizna | Agua Mística | Hidrobomba, Rayo Hielo, Trueno, Paz Mental |
| Milotic | 58 | Escama Especial | Restos | Recuperación, Surf, Rayo Hielo, Tóxico |

**Rival, resto de combates** (acordado el plan; los equipos concretos están en
la propuesta de la conversación y se escriben al implementar). Dos compañeros
fijos evolucionan de un combate a otro: Ralts (Petalia 13) → Kirlia (Ruta 119,
29) → Gardevoir (Lilycove, 33), y Electrike (Ruta 110) → Manectric (Ruta 119,
30; Lilycove, 33). El inicial es el as, con las piezas de tipo de siempre:
Lotad/Lombre/Ludicolo (jugador Treecko), Slugma/Pelipper (Mudkip) y
Lombre/Slugma (Torchic). Ruta 103: un solo Pokémon, nivel 5.

| Combate | Pokémon (variante con Treecko) |
|---|---|
| Ruta 103 | Torchic 5 |
| Petalia | Lotad 13, Ralts 13, Torchic 15 |
| Ruta 119 | Pelipper 29, Kirlia 29, Lombre 29, Manectric 30, Combusken 31 |
| Lilycove | Tropius 31, Pelipper 32, Ludicolo 32, Manectric 33, Gardevoir 33, Combusken 34 |
