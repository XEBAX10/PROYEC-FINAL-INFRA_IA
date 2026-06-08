# Documentación Técnica — Motor Paralelo de Mancala

## Índice

| # | Archivo | Contenido |
|---|---------|-----------|
| 01 | [01-arquitectura.md](01-arquitectura.md) | Visión general, diagrama de orquestación, API REST, CORS |
| 02 | [02-motor.md](02-motor.md) | Reglas Kalah(6,4), heurística, Minimax+Alfa-Beta, pruebas |
| 03 | [03-paralelizacion.md](03-paralelizacion.md) | OpenMP Root Parallelism, métricas T(p)/S(p)/E(p), gráficas |
| 04 | [04-despliegue-local.md](04-despliegue-local.md) | Dockerfiles, docker-compose, Kubernetes local |
| 05 | [05-despliegue-nube.md](05-despliegue-nube.md) | Kubernetes en la nube, manifiestos, requests/limits |
| 06 | [06-cicd.md](06-cicd.md) | GitHub Actions, SonarQube en YAML, pipeline |
| 07 | [07-analisis-comparativo.md](07-analisis-comparativo.md) | Latencia p50/p95, throughput, local vs nube |
| 08 | [08-conclusiones.md](08-conclusiones.md) | Retos, lecciones aprendidas, mejoras futuras |

## Mapeo criterio → archivo

| Criterio rúbrica | Archivo |
|------------------|---------|
| Motor de Mancala: corrección | [02-motor.md](02-motor.md) |
| Paralelización con OpenMP | [03-paralelizacion.md](03-paralelizacion.md) |
| Instrumentación local | [03-paralelizacion.md](03-paralelizacion.md) |
| Separación de componentes | [01-arquitectura.md](01-arquitectura.md) |
| Despliegue local | [04-despliegue-local.md](04-despliegue-local.md) |
| Despliegue en la nube con Kubernetes | [05-despliegue-nube.md](05-despliegue-nube.md) |
| CI/CD y calidad de código | [06-cicd.md](06-cicd.md) |
| Análisis comparativo local vs. nube | [07-analisis-comparativo.md](07-analisis-comparativo.md) |
| Claridad de explicaciones | Transversal |
| Conclusiones | [08-conclusiones.md](08-conclusiones.md) |
