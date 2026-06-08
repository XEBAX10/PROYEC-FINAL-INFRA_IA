# 02 — Motor de Mancala: Algoritmo de IA

## Reglas Kalah(6,4) implementadas

### Parámetros

| Parámetro | Valor |
|-----------|-------|
| Filas de hoyos | 2 |
| Hoyos por fila | 6 |
| Semillas iniciales por hoyo | 4 |
| Total de semillas | 48 |

### Representación del tablero

Array de 14 enteros indexados `[0..13]`:

```
P1: [12][11][10][ 9][ 8][ 7]
    [13]              [ 6]
P0: [ 0][ 1][ 2][ 3][ 4][ 5]
```

El pit opuesto de cualquier pit $i$ es $12 - i$ (válido para ambos lados):

| P0 pit | P1 opuesto |
|--------|-----------|
| 0 | 12 |
| 1 | 11 |
| 2 | 10 |
| 3 | 9 |
| 4 | 8 |
| 5 | 7 |

### Regla 1 — Distribución antihoraria

El jugador toma todas las semillas de un hoyo propio y las distribuye una a una en sentido antihorario, **saltando el kalaha del oponente**.

Orden canónico de siembra: `0→1→2→3→4→5→6(K0)→7→8→9→10→11→12→(skip 13)→0→...`

### Regla 2 — Turno extra

Si la **última semilla** cae en el **kalaha propio**, el jugador repite turno.

### Regla 3 — Captura

Si la última semilla cae en un **hoyo propio vacío** y el hoyo opuesto del rival contiene semillas, el jugador captura ambos grupos hacia su kalaha.

### Regla 4 — Fin de juego

El juego termina cuando **un lado queda vacío**. Las semillas restantes del otro lado van al kalaha de su dueño. Gana quien tenga más semillas en su kalaha.

## Función heurística

$$h(\text{estado}) = (K_\text{own} - K_\text{opp}) + \alpha \cdot (S_\text{own} - S_\text{opp})$$

Donde:
- $K_\text{own}$, $K_\text{opp}$: semillas en el kalaha propio y rival
- $S_\text{own}$, $S_\text{opp}$: semillas en los hoyos del jugador actual y del rival
- $\alpha = 0.1$ (peso para semillas en juego)

La diferencia de kalahas domina porque son las únicas semillas que cuentan al final; $\alpha$ pequeño da bonificación por control del tablero sin distorsionar la evaluación.

## Pseudocódigo Minimax con Alfa–Beta

```
function alphabeta(board, depth, α, β, maximizing):
    if depth == 0 or terminal(board):
        return evaluate(board)

    moves ← legal_moves(board)
    if maximizing:
        best ← -∞
        for mv in moves:
            child ← apply(board, mv)
            extra ← mv landed in own kalaha
            score ← alphabeta(child, depth-1, α, β, extra OR NOT maximizing)
            best  ← max(best, score)
            α     ← max(α, score)
            if α ≥ β:
                prunes++
                break          ← poda beta
        return best
    else:
        best ← +∞
        for mv in moves:
            child ← apply(board, mv)
            extra ← mv landed in own kalaha
            score ← alphabeta(child, depth-1, α, β, extra OR maximizing)
            best  ← min(best, score)
            β     ← min(β, score)
            if α ≥ β:
                prunes++
                break          ← poda alfa
        return best
```

**Nota sobre turno extra:** cuando un movimiento produce turno extra, el jugador sigue siendo el mismo en el siguiente nivel → el booleano `maximizing` no se invierte.

## Suite de pruebas unitarias

Las pruebas se encuentran en [`motor/tests/test_board.cpp`](../motor/tests/test_board.cpp) y cubren:

| Test | Descripción | Criterio |
|------|-------------|---------|
| `BoardInit.SeventyTwoSeeds` | El tablero inicial tiene 48 semillas | Inicialización |
| `BoardInit.KalahasEmpty` | Kalahas inician en 0 | Inicialización |
| `LegalMoves.AllSixAvailableAtStart` | 6 movimientos legales al inicio | Movimientos |
| `LegalMoves.EmptyPitExcluded` | Hoyos vacíos no son movimientos legales | Movimientos |
| `ExtraTurn.LastSeedInOwnKalaha` | Semilla en kalaha propio → turno extra | **Regla 2** |
| `ExtraTurn.NoExtraTurnWhenLandingOnPit` | Aterrizaje en hoyo → cambio de turno | **Regla 2** |
| `Capture.LastSeedInEmptyOwnPitCapturesBoth` | Captura de semillas opuestas | **Regla 3** |
| `Capture.NoCaptureIfOppositeEmpty` | No captura si el opuesto está vacío | **Regla 3** |
| `Terminal.EmptySideIsTerminal` | Lado vacío → juego terminal | **Regla 4** |
| `Terminal.CollectRemainingAddsSeedsToKalahas` | Recolección final correcta | **Regla 4** |
| `AlphaBeta.SameMoveAsMinimaxDepth4` | Alfa–Beta = Minimax en profundidad 4 | **Criterio B** |
| `AlphaBeta.SameMoveAsMinimaxDepth6` | Alfa–Beta = Minimax en profundidad 6 | **Criterio B** |

### Evidencia de equivalencia Alfa–Beta vs. Minimax

Los tests `AlphaBeta.SameMoveAs*` comparan la **evaluación óptima** del algoritmo Alfa–Beta contra la búsqueda Minimax sin poda (fuerza bruta). Ambos producen el mismo score a igual profundidad, confirmando que la poda no altera la jugada óptima.

Para ejecutar:
```bash
cd motor/build
ctest --output-on-failure -R AlphaBeta
```

Salida esperada:
```
[==========] Running 2 tests from 1 test suite.
[----------] 2 tests from AlphaBeta
[ RUN      ] AlphaBeta.SameMoveAsMinimaxDepth4
[       OK ] AlphaBeta.SameMoveAsMinimaxDepth4 (234 ms)
[ RUN      ] AlphaBeta.SameMoveAsMinimaxDepth6
[       OK ] AlphaBeta.SameMoveAsMinimaxDepth6 (8432 ms)
[==========] 2 tests from 1 test suite ran.
[  PASSED  ] 2 tests.
```

## Modo benchmark sin HTTP

```bash
# Secuencial (1 hilo)
OMP_NUM_THREADS=1 ./mancala_bench --depth 12 --positions tests/suite.txt

# Paralelo con perf stat
OMP_NUM_THREADS=8 perf stat -e cycles,instructions,cache-misses \
  ./mancala_bench --depth 12 --positions tests/suite.txt
```

Salida CSV:
```
depth,threads,nodes,prunes,wall_ms
12,1,98234510,31204880,4521
12,8,102187340,29834210,632
```
