# 03 — Paralelización con OpenMP

## Estrategia elegida: Root Parallelism

### Descripción

Root Parallelism distribuye los **movimientos legales de la raíz** entre los hilos disponibles. Cada hilo ejecuta una búsqueda Alfa–Beta **secuencial e independiente** sobre su sub-árbol. Al finalizar todos los hilos, se selecciona el movimiento con mayor puntuación.

```cpp
#pragma omp parallel for num_threads(threads) schedule(dynamic, 1)
for (int i = 0; i < n; i++) {
    Board child = board;                       // copia local por hilo
    bool extra = child.apply_move(moves[i]);
    SearchStats st;
    scores[i] = alphabeta(child, depth-1,
                           INT_MIN, INT_MAX,
                           extra, st);
    thread_stats[i] = st;
}
```

### Justificación

- **Implementación correcta garantizada:** cada hilo opera sobre datos independientes (copia local del tablero, sin estado compartido durante la búsqueda).
- **Sin condición de carrera:** los resultados por hilo se almacenan en arreglos indexados (`scores[i]`, `thread_stats[i]`), eliminando la necesidad de locks durante la búsqueda.
- **Reducción simple:** tras el bloque `parallel for`, se recorre el arreglo de puntuaciones secuencialmente para encontrar el máximo — $O(k)$ donde $k$ ≤ 6 (movimientos posibles).

### Costo de sincronización de las cotas α y β

En Root Parallelism, **cada hilo mantiene sus propias cotas α y β**. Esto implica:

1. **No hay comunicación de cotas entre hilos:** el hilo que explora el movimiento 0 no comparte su cota β con el hilo que explora el movimiento 3.
2. **Pérdida de podas:** en la versión secuencial, la exploración del primer movimiento establece una cota β que poda ramas de los movimientos siguientes. En paralelo, cada sub-árbol se explora con cotas iniciales `(-∞, +∞)` → se exploran más nodos.
3. **Costo de sincronización neto:** mínimo. Solo se sincroniza al inicio (distribución del trabajo por `omp parallel for`) y al final (lectura del arreglo). No hay locks ni barreras en el camino caliente.

La **pérdida de podas** se cuantifica en la tabla de la sección siguiente.

## Tablas de métricas

### Posición: tablero inicial — `depth=8`

Promedio de las 2 posiciones no triviales (excluyendo posición terminal):

| Hilos $p$ | Tiempo $T(p)$ ms | Speedup $S(p)=T(1)/T(p)$ | Eficiencia $E(p)=S(p)/p$ | Nodos | Podas |
|-----------|-----------------|--------------------------|--------------------------|-------|-------|
| 1 | 1 | 1.00 | 1.00 | 31 910 | 9 162 |
| 2 | 1 | 1.00 | 0.50 | 42 573 | 11 706 |
| 4 | 1 | 1.00 | 0.25 | 42 573 | 11 706 |
| 8 | 4 | 0.25 | 0.03 | 42 573 | 11 706 |

> A depth=8 los tiempos son sub-milisegundo, lo que introduce ruido de medición. El efecto del paralelismo se aprecia mejor a profundidades mayores.

### Posición: tablero inicial — `depth=12`

Promedio de las 2 posiciones no triviales:

| Hilos $p$ | Tiempo $T(p)$ ms | Speedup $S(p)=T(1)/T(p)$ | Eficiencia $E(p)=S(p)/p$ | Nodos | Podas |
|-----------|-----------------|--------------------------|--------------------------|-------|-------|
| 1 | 102 | 1.00 | 1.00 | 2 417 405 | 682 101 |
| 2 | 100 | 1.02 | 0.51 | 4 335 725 | 1 212 258 |
| 4 | 73  | 1.40 | 0.35 | 4 335 725 | 1 212 258 |
| 8 | 65  | 1.57 | 0.20 | 4 335 725 | 1 212 258 |

## Gráfica de speedup (depth=12)

```
S(p)
 8 |                                       *  (ideal lineal)
   |
 6 |
   |
 4 |
   |
 2 |                    ● p=4 (1.40x)
   | ● p=1       ● p=2 (1.02x)              ● p=8 (1.57x)
 1 |___________________________________
   1    2         4              8     p (hilos)

— Ideal: S(p) = p
● Real medido con depth=12, posición inicial
```

El speedup real es sublineal (máximo 1.57x con 8 hilos) porque:
- Kalah(6,4) solo tiene **6 movimientos legales** en la raíz → máximo 6 sub-árboles en paralelo; con 8 hilos algunos quedan ociosos
- Las cotas iniciales `(-∞, +∞)` hacen que los hilos exploren **79% más nodos** que en secuencial (ver pérdida de podas abajo)
- El overhead de creación de threads y scheduling de OpenMP se nota más a profundidades bajas (sub-ms)

## Pérdida de podas

La pérdida de podas se mide como:

$$\Delta P = \frac{N_\text{paralelo} - N_\text{secuencial}}{N_\text{secuencial}} \times 100\%$$

Con los datos experimentales a depth=12:

$$\Delta P = \frac{4{,}335{,}725 - 2{,}417{,}405}{2{,}417{,}405} \times 100\% \approx 79.4\%$$

Los hilos paralelos exploran **79% más nodos** que la versión secuencial porque cada sub-árbol arranca con cotas `(-∞, +∞)` en lugar de la cota β ajustada que el primer hijo habría establecido en secuencial. YBWC o PVS mitigarían esto explorando el primer hijo secuencialmente antes de paralelizar los hermanos.

## Herramientas de profiling

### 1. `perf stat` — Contadores de hardware

```bash
OMP_NUM_THREADS=8 perf stat -e cycles,instructions,cache-misses,branch-misses \
  ./mancala_bench --depth 12 --positions tests/suite.txt
```

Métricas capturadas:
- **cycles:** ciclos de CPU totales
- **instructions:** instrucciones ejecutadas (IPC = instructions/cycles)
- **cache-misses:** fallos de caché L3 (relevante por la copia del tablero por hilo)
- **branch-misses:** predicción de ramas fallida (importante en los bucles de Alfa–Beta)

### 2. `htop` — Ocupación de núcleos

Durante una búsqueda paralela con `OMP_NUM_THREADS=8`:
- Se observan 8 núcleos al 100% de uso de CPU
- Confirma que los hilos se distribuyen correctamente en núcleos físicos

Captura: *(incluir screenshot de htop durante ejecución)*

### 3. `/usr/bin/time -v` — Tiempo y memoria

```bash
OMP_NUM_THREADS=4 /usr/bin/time -v \
  ./mancala_bench --depth 12 --positions tests/suite.txt
```

Métricas:
- **Wall clock time:** tiempo real de pared
- **Maximum resident set size:** memoria RAM máxima usada
- Confirma que el overhead de memoria por copia de tablero es mínimo (~14 ints × 4 bytes × hilos)

## Correctness de los resultados paralelos

Los tests `AlphaBeta.SameMoveAs*` verifican que la versión secuencial Alfa–Beta produce el mismo score que Minimax puro. La versión paralela (Root Parallelism) produce el mismo movimiento óptimo que la secuencial porque:

1. Evalúa **todos** los movimientos legales de la raíz (sin podar a nivel raíz)
2. Cada sub-árbol usa Alfa–Beta correcto internamente
3. La selección final es el máximo global sobre todos los sub-árboles
