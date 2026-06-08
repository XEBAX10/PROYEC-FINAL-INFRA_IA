# 08 — Conclusiones

## Conclusiones basadas en métricas

### Motor y paralelización

1. **Root Parallelism ofrece speedup real pero sublineal.** El speedup empírico con 4 hilos se aproxima a 3x (no 4x ideal) por la pérdida de podas y el desbalance de carga entre sub-árboles. A mayor profundidad, la pérdida de podas crece porque las cotas Alfa–Beta del primer hijo (que en secuencial podarían ramas de los siguientes) no se comparten entre hilos.

2. **El beneficio de los hilos adicionales decrece exponencialmente.** Ir de 1→2 hilos produce el mayor salto en speedup; de 4→8 hilos el beneficio es marginal, confirmando la ley de Amdahl: hay una fracción secuencial irreducible (recolección de resultados, movimiento de la raíz).

3. **La función heurística con $\alpha = 0.1$ es suficientemente discriminativa** para guiar la búsqueda hacia jugadas agresivas (captura y control de kalahas) sin distorsionar la evaluación en posiciones equilibradas.

### Despliegue distribuido

4. **La separación estricta en contenedores facilitó el escalado independiente.** El backend se escala a 3 réplicas sin afectar el motor, permitiendo atender más usuarios concurrentes manteniendo la misma latencia individual por partida.

5. **Kubernetes local (kind) reproduce fielmente el comportamiento de la nube** para desarrollo y pruebas. Las diferencias de latencia entre ambos entornos se explican por la red del clúster cloud y el acceso a nodos con más CPUs físicas.

## Retos encontrados y cómo se resolvieron

| Reto | Solución |
|------|---------|
| **Sincronización de cotas α/β en paralelo** | Root Parallelism evita el problema usando cotas independientes por hilo; se documentó la pérdida de podas como trade-off explícito |
| **Serialización del tablero entre C++ y Python** | Contrato JSON con array de 14 enteros y validación pydantic; nunca datos binarios |
| **CORS entre frontend (puerto 8080) y backend (puerto 8000)** | `CORSMiddleware` con orígenes explícitos y preflight OPTIONS manejado automáticamente |
| **Tests de equivalencia Alfa–Beta vs. Minimax** | Implementación de Minimax puro (sin poda) como oráculo para comparar scores |
| **Build multi-stage del motor C++** | `gcc:13` como builder, `debian:bookworm-slim` como runtime; los tests corren durante el build |

## Recomendaciones de mejoras futuras

1. **Implementar YBWC (Young Brothers Wait Concept):** explorar el primer hijo secuencialmente para obtener una cota β ajustada antes de paralelizar los hermanos. Esto reduciría la pérdida de podas de Root Parallelism sin sacrificar mucha paralelización.

2. **Transposition Table thread-safe:** usar una hash table global con `std::atomic` para cachear posiciones ya evaluadas. Significativa para profundidades > 10 donde se repiten muchas sub-posiciones por transposiciones del tablero.

3. **Iterative Deepening:** combinar búsqueda iterativa (depth 1,2,...,N) con la transposition table para ordenar movimientos y mejorar la calidad de las podas desde la profundidad 2 en adelante.

4. **Escalado automático del motor (HPA):** con la métrica `mancala_elapsed_ms` expuesta en `/metrics`, un Horizontal Pod Autoscaler de Kubernetes podría escalar el motor basándose en latencia real, no solo en CPU.

5. **WebSocket para partidas en tiempo real:** reemplazar el modelo request/response del frontend por un WebSocket persistente, eliminando el overhead de establecer conexión TCP por cada jugada.

## Lecciones aprendidas

- La **separación arquitectural en contenedores** tiene costo de setup (red, CORS, serialización) pero beneficios claros: cada componente se puede actualizar, escalar y depurar independientemente.
- El **paralelismo en búsqueda adversaria** no es gratuito: la pérdida de podas es real y medible, y diseñar estrategias que la minimicen (YBWC, PVS) requiere balancear complejidad de implementación vs. ganancia de rendimiento.
- Los **manifiestos YAML de Kubernetes** son la fuente de verdad del despliegue: un cambio en la consola web del proveedor sin actualizar el YAML genera deuda técnica difícil de rastrear.
- **Medir antes de optimizar:** el barrido con `perf stat` y `htop` mostró que el cuello de botella real a depth=8 está en los ciclos de CPU del motor, no en la red ni en el backend Python.
