



# Tornado Scene - Real-Time Volumetric Rendering

A real-time 3D tornado scene built with **OpenGL 4.5** and **C++20**, developed as the final project for TDT4230: Graphics and Visualisation at NTNU.

The core technique is **volumetric ray marching**: the tornado funnel, storm cloud, and debris field are all rendered as animated procedural density fields - no polygon meshes involved.

---

## Features

- **Volumetric tornado** - procedural SDF-based funnel with height-varying radius, Gaussian pinch, animated twist/wobble/sway, and two layers of noise detail (shape noise + filament noise)
- **Cumulonimbus storm cloud** - ellipsoid SDF with animated noise displacement and sun-edge albedo shading
- **Debris ring** - volumetric torus shell around the tornado base with numerically-estimated surface normals
- **Deferred rendering pipeline** - G-buffer (position, albedo, normal, AO, depth) -> lighting -> volumetric composite
- **Physically-based volume lighting** - Beer-Lambert transmittance, Henyey-Greenstein phase function, self-shadowing light march
- **Ray-marched soft shadows** cast by the tornado onto the terrain
- **Procedural terrain** - 512x512 noise heightmap, partitioned into a 20x20 tile grid, normal-mapped with grass/rock texture blend
- **Billboard trees** - placed via Poisson disk sampling
- **Atmospheric fog** - exponential height-based fog
- **Lightning flash** - animated procedural lightning bolts with configurable frequency and intensity
- **Blue noise jitter** - eliminates ray march banding using a 1024x1024 blue noise texture
- **Dear ImGui overlay** - live tweaking of all volume parameters without recompilation

---

## Demo


Showcase of tornado scene
![Tornado_showcase-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/be7e0182-e95b-44f7-be3b-65464a96161e)


Dear ImGui showcase
![ImGUI_showcase-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/da920fb9-ad1d-4315-bbbe-e82650fe4548)


---

## Screenshots

*Add screenshots here.*

---

## Building

Requirements: **CMake 3.14+**, a C++20 compiler, **Python 3** (for GLAD generation), and **OpenMP**.

```bash
git clone --recurse-submodules https://github.com/chripaul79/TornadoRayMarcher.git
cd TornadoRayMarcher
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
  scenelogic.cpp/.h       - Main scene setup, update, and render loop
  sceneGraph.cpp/.hpp     - Scene node hierarchy
  utilities/
    terrain.cpp/.h        - Heightmap and tile mesh generation
    trees.cpp/.h          - Billboard tree placement (Poisson disk)
    shapes.cpp/.h         - Primitive geometry helpers
    shader.hpp            - Shader loader with #include support
  ui/
    volume_imgui.cpp/.h   - ImGui parameter panel
res/
  shaders/
    volume_tornado.frag   - Main ray march pass (tornado + cloud + debris)
    tornado_density.glsl  - Shared tornado SDF (included by multiple shaders)
    tornado_debris.glsl   - Debris density field
    composite_final.frag  - Tone mapping and final composite
    simple.vert/.frag     - Geometry pass
  textures/               - Terrain textures, skybox, blue noise, tree sprite
```

---

## Asset Credits

All textures are free assets used under their respective licenses.

| Texture | Source |
|---------|--------|
| `aerial_grass_rock_*` | [Poly Haven](https://polyhaven.com) (CC0) |
| `rocky_terrain_02_*` | [Poly Haven](https://polyhaven.com) (CC0) |
| Tree sprite | [Vecteezy](https://www.vecteezy.com) (Free license) |

---

## References & Credits

- **Inigo Quilez** - [iquilezles.org](https://iquilezles.org)
  Ray marching fundamentals, signed distance functions, noise functions, soft shadows, and domain deformation. Primary reference for the tornado density function and AABB intersection logic.

- **Maxime Heckel** - [Real-Time Cloudscapes with Volumetric Raymarching](https://blog.maximeheckel.com/posts/real-time-cloudscapes-with-volumetric-raymarching/)
  Volume rendering equation, Beer-Lambert transmittance, and Henyey-Greenstein phase function as applied to real-time atmospheric rendering.

- **LearnOpenGL** - [learnopengl.com](https://learnopengl.com)
  Deferred shading pipeline, framebuffer objects, cubemap skyboxes, instanced rendering, and GLSL fundamentals.

---

## License

Academic project - NTNU TDT4230, Spring 2026.
