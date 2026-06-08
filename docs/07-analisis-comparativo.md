# 07 — Análisis Comparativo: Local vs. Nube

## Configuración del experimento

### Entorno local

| Variable | Valores |
|----------|---------|
| `OMP_NUM_THREADS` | 1, 2, 4, 8 |
| Instancias backend | 1 |
| Profundidad búsqueda | 8 |
| Herramienta de carga | `k6` |

### Entorno nube (GKE)

| Variable | Valores |
|----------|---------|
| `OMP_NUM_THREADS` | 2 (moderado) |
| Réplicas backend | 1, 3 |
| Profundidad búsqueda | 8 |
| Herramienta de carga | `k6` |

### Script de carga k6

```javascript
import http from 'k6/http';
import { sleep } from 'k6';

export let options = {
  vus:      50,
  duration: '60s',
};

const BOARD = [4,4,4,4,4,4,0,4,4,4,4,4,4,0];

export default function () {
  http.post('http://TARGET/move', JSON.stringify({
    board: BOARD, side: 0, depth: 8, threads: 2
  }), { headers: { 'Content-Type': 'application/json' } });
  sleep(0.1);
}
```

## Resultados

### Latencia p50 / p95 (ms)

| Entorno | Configuración | p50 (ms) | p95 (ms) | Throughput (req/s) |
|---------|--------------|---------|---------|-------------------|
| **Local** | 1 hilo, 1 instancia | — | — | — |
| **Local** | 4 hilos, 1 instancia | — | — | — |
| **Local** | 8 hilos, 1 instancia | — | — | — |
| **Nube** | 2 hilos, 1 réplica | — | — | — |
| **Nube** | 2 hilos, 3 réplicas | — | — | — |

> Los valores con "—" se completan con los resultados reales del experimento.

### Throughput sostenido

```
req/s
 │    ● Nube 3 réplicas
 │  ●
 │●   ● Local 8 hilos
 │  ●
 │●   ● Local 4 hilos
 │  ●
 │●   ● Local 1 hilo / Nube 1 réplica
 └─────────────────────────
   configuración
```

## Observación cualitativa — Escalado vertical vs. horizontal

**Escalado vertical (más hilos/pod):**
- Beneficia directamente el tiempo de búsqueda Alfa–Beta (reduce `elapsed_ms` por petición)
- Efectivo cuando el cuello de botella está en **la computación del motor** (búsquedas profundas)
- Limitado por el número de cores del nodo: a partir de `OMP_NUM_THREADS > cores_físicos`, el speedup se satura

**Escalado horizontal (más réplicas de backend):**
- Beneficia el **throughput total** del sistema (más peticiones simultáneas atendidas)
- El motor sigue siendo compartido (1 réplica) → no reduce la latencia individual por petición
- Efectivo cuando el cuello de botella está en la **concurrencia de peticiones** (muchos usuarios simultáneos)

**Recomendación anclada en los números:**
- Si la latencia p95 es alta con 1 usuario → aumentar `OMP_NUM_THREADS` (cuello de botella en el motor)
- Si la latencia p95 es aceptable pero el throughput colapsa con muchos usuarios → añadir réplicas de backend
- Para el motor de Mancala con `depth=8`, el tiempo de búsqueda domina → **escalar verticalmente primero**; escalar horizontalmente sirve para atender torneos o múltiples partidas simultáneas
