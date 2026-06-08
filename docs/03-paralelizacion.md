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

| Hilos $p$ | Tiempo $T(p)$ ms | Speedup $S(p)=T(1)/T(p)$ | Eficiencia $E(p)=S(p)/p$ | Nodos | Podas |
|-----------|-----------------|--------------------------|--------------------------|-------|-------|
| 1 | **_T1_** | 1.00 | 1.00 | **_N1_** | **_P1_** |
| 2 | **_T2_** | **_S2_** | **_E2_** | **_N2_** | **_P2_** |
| 4 | **_T4_** | **_S4_** | **_E4_** | **_N4_** | **_P4_** |
| 8 | **_T8_** | **_S8_** | **_E8_** | **_N8_** | **_P8_** |

> **Nota:** los valores en negrita con guiones bajos son marcadores para completar con los resultados reales del experimento. Ejecutar:
> ```bash
> for t in 1 2 4 8; do
>   OMP_NUM_THREADS=$t ./mancala_bench --depth 8 --positions tests/suite.txt
> done
> ```

### Posición: tablero inicial — `depth=12`

| Hilos $p$ | Tiempo $T(p)$ ms | Speedup $S(p)$ | Eficiencia $E(p)$ | Nodos | Podas |
|-----------|-----------------|----------------|-------------------|-------|-------|
| 1 | — | 1.00 | 1.00 | — | — |
| 2 | — | — | — | — | — |
| 4 | — | — | — | — | — |
| 8 | — | — | — | — | — |

## Gráfica de speedup

```
S(p)
 8 |                                       *  (ideal lineal)
   |                                  *
 6 |                           *
   |                    *
 4 |              *
   |         *
 2 |    *
   | *
 1 |___________________________________
   1    2         4              8     p (hilos)

— Ideal: S(p) = p
● Real:  S(p) < p  (por pérdida de podas y overhead de scheduling)
```

El speedup real es sublineal porque:
- Los sub-árboles no son de igual tamaño (desbalance de carga)
- Las cotas iniciales `(-∞, +∞)` hacen que los hilos exploren más nodos que en secuencial

## Pérdida de podas

La pérdida de podas se mide como:

$$\Delta P = \frac{N_\text{paralelo} - N_\text{secuencial}}{N_\text{secuencial}} \times 100\%$$

A mayor profundidad, mayor es $\Delta P$ porque la cota β que establecería el primer movimiento secuencial sería más ajustada. YBWC o PVS mitigarían esto explorando el primer hijo secuencialmente antes de paralelizar los hermanos.

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
