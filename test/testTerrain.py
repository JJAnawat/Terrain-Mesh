import numpy as np
from PIL import Image
 
W, H = 1024, 1024
width_extent, depth_extent = 10.0, 10.0
 
x = np.linspace(0, 1, W) - 0.5
z = np.linspace(0, 1, H) - 0.5
px = x * width_extent
pz = z * depth_extent
 
PX, PZ = np.meshgrid(px, pz, indexing='ij')
 
py = (0.5 * np.sin(PX * 1.5) * np.cos(PZ * 1.5)
    + 0.2 * np.sin(PX * 3.5 + 1.0) * np.cos(PZ * 3.5)
    + 0.1 * np.sin(PX * 8.0))
 
py = (py - py.min()) / (py.max() - py.min())  # normalize to [0, 1]
img = Image.fromarray((py * 255).astype(np.uint8), mode='L')
img.save("assets/test-terrain.png")
print("Saved test-terrain.png")
