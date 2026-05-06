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