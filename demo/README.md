# Terrain Mesh Demo

This guide explains how to run the built executable from the `build/Debug` folder and how to use the command-line options.

## Expected layout

After building, the executable is generated in:

```
terrain-mesh/
  build/
    Debug/
      TerrainMesh.exe
  CMakeLists.txt
  Readme.md
  assets/
  shaders/
  src/
  test/
  output/
  demo/
```

The repository ignores build artifacts, so the executable should remain in `build/Debug`.

## Run the demo

From the project root:

```powershell
cd build\Debug
.\TerrainMesh.exe --input ..\..\assets\mount-fuji.png --num_points 100 --algorithm 0
```

## Command-line options

- `--input <file>`
  - Input heightmap image file.
  - Default: `assets/mount-fuji.png`
- `--num_points <int>`
  - Maximum number of points used by the mesh reconstruction stage.
  - Default: `100`
- `--algorithm <int>`
  - Select the mesh-generation algorithm.
  - Default: `0`
- `--help`
  - Show usage help and exit.

## Example commands

Run a custom heightmap from the build folder:

```powershell
cd build\Debug
.\TerrainMesh.exe --input ..\..\assets\dark_rocky.png --num_points 500 --algorithm 1
```

Generate a detailed mesh:

```powershell
cd build\Debug
.\TerrainMesh.exe --input ..\..\assets\test-terrain.png --num_points 1000 --algorithm 2
```

## Output files

When the mesh is exported, the program writes:

- `output/vertices.csv`
- `output/triangles.csv`

Use these files for external analysis or visualization.

## Benchmarking the Mesh

To evaluate mesh accuracy against the original heightmap:

1. **Export mesh data**: While running the application, press `C` to export the current mesh to CSV files.

2. **Run the benchmark script**:

   ```bash
   python test/benchmark.py --vertices_csv output/vertices.csv --triangles_csv output/triangles.csv --heightmap assets/mount-fuji.png --output_dir output --show_plot
   ```

   This will:
   - Calculate RMSE and MAE accuracy metrics
   - Generate visualization plots comparing the mesh to the original heightmap
   - Save results to `output/benchmark_results.txt` and `output/benchmark_plot.png`

### Benchmark script options

- `--vertices_csv`: Path to exported vertices CSV (required)
- `--triangles_csv`: Path to exported triangles CSV (required)
- `--heightmap`: Path to original heightmap image (required)
- `--output_dir`: Directory for results (default: "output")
- `--width_extent`: Terrain width extent (default: 4.0)
- `--depth_extent`: Terrain depth extent (default: 4.0)
- `--max_height`: Maximum terrain height (default: 1.0)
- `--show_plot`: Display the visualization plots

## Generating a test heightmap

A test terrain generator is available at `test/testTerrain.py`.

Run:

```bash
python test/testTerrain.py
```

That saves a heightmap image at `assets/test-terrain.png`.
