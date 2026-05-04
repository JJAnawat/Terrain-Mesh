import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.tri as mtri
import matplotlib.image as mpimg
from PIL import Image

# ==========================================
# 1. CONFIGURATION
# ==========================================
VERTICES_CSV = "C:/JJ/Year3/Comp-Geo/terrain-mesh/output/vertices.csv"
TRIANGLES_CSV = "C:/JJ/Year3/Comp-Geo/terrain-mesh/output/triangles.csv"
HEIGHTMAP_PATH = "C:/JJ/Year3/Comp-Geo/terrain-mesh/assets/rocky_land.png"

# Visualization stuffs from C++
WIDTH_EXTENT = 4.0
DEPTH_EXTENT = 4.0
MAX_HEIGHT = 1.0

# ==========================================
# 2. LOAD & PROCESS ORIGINAL HEIGHTMAP
# ==========================================

print("Loading original heightmap...")
raw_img = mpimg.imread(HEIGHTMAP_PATH)

if raw_img.ndim == 3:
    img_data = raw_img[:, :, 0]
else:
    img_data = raw_img

img_data = np.array(img_data, dtype=np.float32)

print(f" -> Image Shape: {img_data.shape}")
print(f" -> Raw Min pixel: {img_data.min()}, Raw Max pixel: {img_data.max()}")

# Min-Max Scaling
min_val, max_val = img_data.min(), img_data.max()
range_val = max_val - min_val if max_val > min_val else 1.0
true_heights = (img_data - min_val) / range_val * MAX_HEIGHT

# ==========================================
# 3. LOAD MESH FROM CSV
# ==========================================
print("Loading mesh data from CSVs...")
verts = pd.read_csv(VERTICES_CSV)
tris = pd.read_csv(TRIANGLES_CSV)

x_mesh = verts['x'].values
y_mesh = verts['y'].values
z_mesh = verts['z'].values
triangles = tris[['v0', 'v1', 'v2']].values

# ==========================================
# 4. SET UP INTERPOLATION
# ==========================================
print("Building 2D triangulation mapping...")

try:
    # 1. Filter out degenerate (zero-area) triangles from C++ output
    x0, z0 = x_mesh[triangles[:, 0]], z_mesh[triangles[:, 0]]
    x1, z1 = x_mesh[triangles[:, 1]], z_mesh[triangles[:, 1]]
    x2, z2 = x_mesh[triangles[:, 2]], z_mesh[triangles[:, 2]]
    
    # Calculate 2D area using the Shoelace formula
    areas = 0.5 * np.abs(x0*(z1 - z2) + x1*(z2 - z0) + x2*(z0 - z1))
    valid_mask = areas > 1e-6
    clean_triangles = triangles[valid_mask]
    
    removed_count = np.sum(~valid_mask)
    if removed_count > 0:
        print(f"  -> Cleaned up {removed_count} zero-area triangles.")

    # 2. Try building the map using the cleaned C++ indices
    triangulation = mtri.Triangulation(x_mesh, z_mesh, clean_triangles)
    interpolator = mtri.LinearTriInterpolator(triangulation, y_mesh)
    print("  -> Successfully mapped using C++ triangles!")

except Exception as e:
    # 3. Fallback: If C++ topology is still rejected, let Python connect the vertices.
    print(f"\n  [!] Matplotlib rejected C++ topology. Reason: {e}")
    
    # We drop the C++ indices and let matplotlib draw valid Delaunay lines between your points
    triangulation = mtri.Triangulation(x_mesh, z_mesh)
    tris = triangulation.get_masked_triangles()
    interpolator = mtri.LinearTriInterpolator(triangulation, y_mesh)
    print("  -> Successfully mapped using Python fallback!")

# ==========================================
# 5. CREATE TEST GRID (Every Pixel)
# ==========================================
H, W = true_heights.shape

# Map pixel indices to world coordinates, exactly as done in C++
# Normalized from -0.5 to 0.5, multiplied by extent
x_coords = (np.arange(W) / float(W - 1) - 0.5) * WIDTH_EXTENT
z_coords = (np.arange(H) / float(H - 1) - 0.5) * DEPTH_EXTENT

# Create a dense grid of X and Z coordinates
X_test, Z_test = np.meshgrid(x_coords, z_coords)

# ==========================================
# 6. EVALUATE & CALCULATE RMSE
# ==========================================
print("Evaluating mesh accuracy...")
# Calculate the mesh's height at every single pixel coordinate
predicted_heights = interpolator(X_test, Z_test)

valid_mask = ~predicted_heights.mask
true_valid = true_heights[valid_mask]
pred_valid = predicted_heights[valid_mask]

# RMSE
mse = np.mean((true_valid - pred_valid) ** 2)
rmse = np.sqrt(mse)

# MAE
mae = np.mean(np.abs(true_valid - pred_valid))

print("\n" + "="*30)
print("       BENCHMARK RESULTS       ")
print("="*30)
print(f"Vertices Used : {len(verts)}")
print(f"Triangles Used: {len(tris)}")
print(f"RMSE          : {rmse:.6f}")
print(f"MAE           : {mae:.6f}")
print("="*30)

# ==========================================
# 7. GENERATE VISUALIZATIONS FOR REPORT
# ==========================================
print("Generating visualization plots...")
error_map = np.abs(true_heights - predicted_heights.filled(0))

fig, axes = plt.subplots(1, 3, figsize=(18, 5))

# Calculate the world-space bounds for the image overlay
img_extent = [-WIDTH_EXTENT/2, WIDTH_EXTENT/2, -DEPTH_EXTENT/2, DEPTH_EXTENT/2]

# Plot 1: True Heightmap
im1 = axes[0].imshow(true_heights, cmap='viridis', extent=img_extent, origin='lower')
axes[0].set_title("True Heightmap")
axes[0].set_aspect('equal')
fig.colorbar(im1, ax=axes[0], fraction=0.046, pad=0.04)

# Plot 2: Mesh Triangulation OVERLAY
axes[1].imshow(true_heights, cmap='gray', extent=img_extent, origin='lower', alpha=0.7)
axes[1].triplot(triangulation, lw=0.6, color='cyan')
axes[1].set_title("Delaunay Mesh Overlay")
axes[1].set_xlim([-WIDTH_EXTENT/2, WIDTH_EXTENT/2])
axes[1].set_ylim([-DEPTH_EXTENT/2, DEPTH_EXTENT/2])
axes[1].set_aspect('equal')
axes[1].axis('off')

# Plot 3: Error Heatmap
im3 = axes[2].imshow(error_map, cmap='hot', vmin=0, vmax=0.5, extent=img_extent, origin='lower')
axes[2].set_title("Absolute Error Heatmap (Black=0, Red=High)")
fig.colorbar(im3, ax=axes[2], fraction=0.046, pad=0.04)

plt.tight_layout()
plt.savefig("C:/JJ/Year3/Comp-Geo/terrain-mesh/output/benchmark_plot.png", dpi=300, bbox_inches='tight')
print("Saved benchmark_plot.png!")
plt.show()