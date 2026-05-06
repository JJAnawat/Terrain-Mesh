# Terrain Mesh

Transform 2D heightmap images into 3D terrain using Delaunay triangulation.

## Project structure

- `src/` – source code files
- `include/` – public headers grouped by feature
- `shaders/` – GLSL shader files
- `assets/` – heightmap input images
- `output/` – generated CSV exports
- `test/` – Python terrain generator and test scripts
- `demo/` – demo usage documentation

## Building

Use CMake to configure and build the project. From the project root:

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

The executable will be generated in `build/Debug`.

## Running the executable

From the project root, run the executable from its build folder:

```powershell
cd build\Debug
.\TerrainMesh.exe --input ..\..\assets\mount-fuji.png --num_points 100 --algorithm 0
```

If you prefer to run from the project root, copy the executable to the root and use asset paths relative to the root.

### Supported options

- `--input <file>`
  - Input heightmap image (PNG or applicable image format).
  - Default: `assets/mount-fuji.png`
- `--num_points <int>`
  - Maximum mesh point budget.
  - Default: `100`
- `--algorithm <int>`
  - Choose the mesh algorithm.
  - Default: `0`
- `--help`
  - Print usage information.

## Demo instructions

See `demo/README.md` for an executable-focused demo guide.

## Generating test terrain

Generate a synthetic heightmap at `assets/test-terrain.png` by running:

```powershell
python test/testTerrain.py
```

Then run the executable against that generated image.

## Notes

- Exported mesh CSV files are written to `output/vertices.csv` and `output/triangles.csv`.

## Benchmarking Result

**Test Terrain**

| Algo        | #Points | Avg Time (ms) |     RMSE |      MAE |
| ----------- | ------: | ------: | -------: | -------: |
| Uni         |    1000 |   24.66 | 0.030241 | 0.023704 |
|             |   10000 | 1034.83 |  0.00355 | 0.002809 |
|             |   50000 | 8148.13 | 0.001384 | 0.001118 |
| Uni+DAG     |    1000 |  111.96 |        - |        - |
|             |   10000 |  944.55 |        - |        - |
|             |   50000 | 3950.75 |        - |        - |
| Sobel       |    1000 |   59.76 | 0.058859 | 0.041727 |
|             |   10000 | 1019.98 | 0.016868 | 0.007333 |
|             |   50000 | 9194.96 | 0.011306 | 0.002323 |
| Sobel+DAG   |    1000 |  157.42 |        - |        - |
|             |   10000 | 1112.56 |        - |        - |
|             |   50000 | 4311.77 |        - |        - |
| Garland     |    1000 | 1187.29 | 0.016577 | 0.013296 |
|             |   10000 | 2503.13 | 0.002031 | 0.001621 |
|             |   50000 | 4334.15 | 0.001345 | 0.001051 |

**Ridge Terrain**
| Algo        | #Points | Avg Time (ms) |     RMSE |      MAE |
| ----------- | ------: | ------: | -------: | -------: |
| Uni         |    1000 |   18.12 | 0.014314 | 0.007772 |
|             |   10000 |  888.70 | 0.004155 | 0.002733 |
|             |   50000 | 7797.81 | 0.002594 | 0.001959 |
| Uni+DAG     |    1000 |  103.61 |        - |        - |
|             |   10000 |  850.02 |        - |        - |
|             |   50000 | 4174.64 |        - |        - |
| Sobel       |    1000 |   85.85 |  0.01708 | 0.010685 |
|             |   10000 | 1002.71 | 0.008985 | 0.003585 |
|             |   50000 | 9594.98 | 0.008049 | 0.002382 |
| Sobel+DAG   |    1000 |  141.66 |        - |        - |
|             |   10000 | 1121.46 |        - |        - |
|             |   50000 | 5343.45 |        - |        - |
| Garland     |    1000 |  840.86 | 0.010201 | 0.008134 |
|             |   10000 | 2228.93 | 0.003187 |  0.00248 |
|             |   50000 | 4348.38 | 0.002323 | 0.001842 |