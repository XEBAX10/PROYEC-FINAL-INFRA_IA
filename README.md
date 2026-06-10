# Motor Paralelo de Mancala (Kalah) — Universidad del Valle

**Materia:** Inteligencia Artificial + Infraestructuras Paralelas y Distribuidas  
**Semestre:** 6° — Ingeniería de Sistemas  
**Docente:** Carlos Andrés Delgado S., MSc

## Integrantes

| Nombre | Código | Correo |
|--------|--------|--------|
| Sebastian Barco | 2380599 | sebastian.barco@correounivalle.edu.co |
| Santiago Montoya | 2380372 | Santiago.mayor@correounivalle.edu.co |
| Laura Rivera | 2380712 | Laura.rivera.arias@correounivalle.edu.co |


## Descripción

Motor de juego para Mancala variante **Kalah(6,4)** con búsqueda adversaria **Minimax + Alfa–Beta** paralelizado con **OpenMP** (Root Parallelism), desplegado en contenedores Docker y Kubernetes.

## Levantar la aplicación localmente

```bash
# Docker Compose — un solo comando
docker compose up --build

# Frontend: http://localhost:8080
# Backend:  http://localhost:8000
# Motor:    http://localhost:8001
```

```bash
# Kubernetes local (kind)
kind create cluster --name mancala
kubectl apply -f deploy/local/k8s/configmap.yaml
kubectl apply -f deploy/local/k8s/motor-deployment.yaml
kubectl apply -f deploy/local/k8s/backend-deployment.yaml
kubectl apply -f deploy/local/k8s/frontend-deployment.yaml
```

## Compilar y probar el motor

```bash
cd motor
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
cd build && ctest --output-on-failure

# Benchmark con perf
OMP_NUM_THREADS=1 ./build/mancala_bench --depth 8 --positions tests/suite.txt
OMP_NUM_THREADS=4 ./build/mancala_bench --depth 8 --positions tests/suite.txt
OMP_NUM_THREADS=8 ./build/mancala_bench --depth 12 --positions tests/suite.txt
```

## Estructura del repositorio

```
.
├── motor/          # C++/OpenMP — algoritmo Minimax + Alfa-Beta paralelo
├── backend/        # Python FastAPI — wrapper HTTP y validación
├── frontend/       # nginx + HTML/JS — interfaz del juego
├── deploy/
│   ├── local/      # Docker Compose + Kubernetes local
│   └── cloud/      # Manifiestos Kubernetes nube
├── docs/           # Informe técnico Markdown
└── .github/workflows/  # CI/CD + SonarQube
```

## Documentación

| Sección | Archivo |
|---------|---------|
| Arquitectura y API REST | [docs/01-arquitectura.md](docs/01-arquitectura.md) |
| Motor Kalah(6,4) e IA | [docs/02-motor.md](docs/02-motor.md) |
| Paralelización OpenMP | [docs/03-paralelizacion.md](docs/03-paralelizacion.md) |
| Despliegue local | [docs/04-despliegue-local.md](docs/04-despliegue-local.md) |
| Despliegue en la nube | [docs/05-despliegue-nube.md](docs/05-despliegue-nube.md) |
| CI/CD y calidad de código | [docs/06-cicd.md](docs/06-cicd.md) |
| Análisis comparativo | [docs/07-analisis-comparativo.md](docs/07-analisis-comparativo.md) |
| Conclusiones | [docs/08-conclusiones.md](docs/08-conclusiones.md) |

## Mapeo criterios rúbrica → archivo

| Criterio | Archivo |
|----------|---------|
| Motor de Mancala: corrección | [docs/02-motor.md](docs/02-motor.md) |
| Paralelización con OpenMP | [docs/03-paralelizacion.md](docs/03-paralelizacion.md) |
| Instrumentación local | [docs/03-paralelizacion.md](docs/03-paralelizacion.md) |
| Separación de componentes | [docs/01-arquitectura.md](docs/01-arquitectura.md) |
| Despliegue local | [docs/04-despliegue-local.md](docs/04-despliegue-local.md) |
| Despliegue en la nube con Kubernetes | [docs/05-despliegue-nube.md](docs/05-despliegue-nube.md) |
| CI/CD y calidad de código | [docs/06-cicd.md](docs/06-cicd.md) |
| Análisis comparativo local vs. nube | [docs/07-analisis-comparativo.md](docs/07-analisis-comparativo.md) |
| Claridad de explicaciones | Transversal |
| Conclusiones | [docs/08-conclusiones.md](docs/08-conclusiones.md) |