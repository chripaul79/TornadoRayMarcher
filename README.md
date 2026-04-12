# Tornado Scene — Real-Time Volumetric Rendering

A real-time 3D tornado scene built with **OpenGL 4.5** and **C++20**, developed as the final project for TDT4230: Graphics and Visualisation at NTNU.

The core technique is **volumetric ray marching**: the tornado funnel, storm cloud, and debris field are all rendered as animated procedural density fields — no polygon meshes involved.

---

## Features

- **Volumetric tornado** — procedural SDF-based funnel with height-varying radius, Gaussian pinch, animated twist/wobble/sway, and two layers of noise detail (shape noise + filament noise)
- **Cumulonimbus storm cloud** — ellipsoid SDF with animated noise displacement and sun-edge albedo shading
- **Debris ring** — volumetric torus shell around the tornado base with numerically-estimated surface normals
- **Deferred rendering pipeline** — G-buffer (position, albedo, normal, AO, depth) → lighting → volumetric composite
- **Physically-based volume lighting** — Beer-Lambert transmittance, Henyey-Greenstein phase function, self-shadowing light march
- **Ray-marched soft shadows** cast by the tornado onto the terrain
- **Procedural terrain** — 512×512 noise heightmap, partitioned into a 20×20 tile grid, normal-mapped with grass/rock texture blend
- **Billboard trees** — placed via Poisson disk sampling
- **Atmospheric fog** — exponential height-based fog
- **Lightning flash** — animated procedural lightning bolts with configurable frequency and intensity
- **Blue noise jitter** — eliminates ray march banding using a 1024×1024 blue noise texture
- **Dear ImGui overlay** — live tweaking of all volume parameters without recompilation

---

## Building

Requirements: **CMake 3.14+**, a C++20 compiler, **Python 3** (for GLAD generation), and **OpenMP**.

```bash
git clone --recurse-submodules https://github.com/YOUR_USERNAME/TornadoSceneProject.git
cd TornadoSceneProject
cmake -S . -B build
cmake --build build
./build/tornado
```

All required libraries are included as submodules or vendored in `lib/`.

---

## Controls

| Key | Action |
|-----|--------|
| `W A S D` | Move camera |
| Mouse | Look around |
| `F` | Toggle performance mode |
| `L` | Trigger lightning |
| ImGui overlay | Tune all volume parameters live |

---

## Libraries Used

| Library | Purpose |
|---------|---------|
| [GLFW](https://www.glfw.org/) | Window and input |
| [GLAD](https://github.com/Dav1dde/glad) | OpenGL loader |
| [GLM](https://github.com/g-truc/glm) | Math |
| [Dear ImGui](https://github.com/ocornut/imgui) | Parameter overlay |
| [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) | Noise generation |
| [lodepng](https://github.com/lvandeve/lodepng) | PNG loading |
| [fmt](https://github.com/fmtlib/fmt) | String formatting |

---

## Project Structure

```
src/
  scenelogic.cpp/.h       — Main scene setup, update, and render loop
  sceneGraph.cpp/.hpp     — Scene node hierarchy
  utilities/
    terrain.cpp/.h        — Heightmap and tile mesh generation
    trees.cpp/.h          — Billboard tree placement (Poisson disk)
    shapes.cpp/.h         — Primitive geometry helpers
    shader.hpp            — Shader loader with #include support
  ui/
    volume_imgui.cpp/.h   — ImGui parameter panel
res/
  shaders/
    volume_tornado.frag   — Main ray march pass (tornado + cloud + debris)
    tornado_density.glsl  — Shared tornado SDF (included by multiple shaders)
    tornado_debris.glsl   — Debris density field
    composite_final.frag  — Tone mapping and final composite
    simple.vert/.frag     — Geometry pass
  textures/               — Terrain textures, skybox, blue noise, tree sprite
```

---

## Screenshots

*Add screenshots or a demo video here.*

---

## License

Academic project — NTNU TDT4230, Spring 2026.
