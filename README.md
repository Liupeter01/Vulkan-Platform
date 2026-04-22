# Vulkan-Platform

## 0x00 Vulkan-Platform Overall

### Description

A modern C++17 Vulkan-1.3 rendering playground and engine, focused on **GPU-driven effects**, **compute/graphics pipelines**, and **data-oriented particle systems**. This repo is both a learning lab and a reusable foundation for future rendering experiments (particles, post-processing, compute passes, etc.).

```txt
Vulkan-Platform/
├── assets/          # Textures, models, and demo resources
├── cmake/           # CMake helper modules and toolchain logic
├── external/        # Third-party libraries (managed via .gitmodules)
├── shaders/         # GLSL / Slang shader sources and SPIR-V blobs
├── vulkan-1.3/      # Core engine implementation (Vulkan 1.3 backend, scenes, effects)
├── vulkan-adapt/    # Platform / adapter layer and glue code
├── .gitmodules      # Submodule definitions for external dependencies
└── CMakeLists.txt   # Top-level CMake build script
```

Engine-level modules (inside `vulkan-1.3/`), including but not limited to:

- `VulkanEngine.hpp / .cpp` – engine entry point, initialization and frame loop
- `scene/ScenesManager.hpp` – scene graph, node management, and registration
- `scene/SceneNode*, NodeManagerBuilder` – scene node construction and management
- `frame/FrameData.hpp`, `SwapChainImageData.cpp` – per-frame & swapchain data
- `particle/ParticleDataBuffer.hpp`, `ParticleLayoutPolicy.hpp` – GPU particle storage
- `compute/`, `material/` – compute and graphics pipelines, descriptor builders



### Features

#### Vulkan 1.3 Rendering Core

  - Swapchain & frame management

  - Command buffer recording and submission

  - Render passes, framebuffers, sync primitives

    

#### Engine Architecture

  - `VulkanEngine` as the central entry point

  - Scene management via `ScenesManager`, `SceneNode`, and builder utilities

  - Clear separation between **compute effects** and **graphic/material effects**

    

#### GPU Particle System

  - `GPUParticle` driven particle simulation

  - Compute shader updates → graphics pipeline rendering

  - Pluggable layout policies via `ParticleDataBuffer` and `particle::policy::SOALayout`

  - Support for AoS / SoA style layouts through template policies

    

#### Effect Abstraction Layer

  - `Compute_EffectBase` and `Graphic_EffectBase` for reusable effect implementations

  - Combined compute + graphic systems such as `PointSpriteParticleSystemBase`

  - Strong use of RAII and templates to avoid manual resource bookkeeping

    

#### Data-oriented Design

  - Policy-based layout for particle buffers (`SOALayout`, layout policy system)

  - Explicit control over descriptor layouts, pipelines, and push constants

  - Designed to scale to different particle layouts and rendering strategies

    

#### Shader System

  - GLSL / SPIR-V shaders in `shaders/`
  - Slang-compiled shaders for more complex effects (e.g. sprite particle rendering)



## 0x01 Getting Started

### Prerequisites

You will need:

- A C++17-capable compiler
  - **Windows**: MSVC (Visual Studio 2019/2022)
  - **Linux / macOS**: Clang or GCC
- [CMake](https://cmake.org/) (3.20+ recommended)
- [Vulkan SDK](https://vulkan.lunarg.com/) (1.3.x)
- A GPU and driver with Vulkan 1.3 support
- On macOS, MoltenVK is required to run Vulkan on top of Metal.

External dependencies are tracked via Git submodules under `external/`.



### Clone the repository

```
git clone https://github.com/Liupeter01/Vulkan-Platform.git
cd Vulkan-Platform

# Fetch external dependencies
git submodule update --init --recursive
```



### Configure & build (CMake)

#### On Linux / macOS (Makefiles / Ninja)

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### On Windows (Visual Studio / MSVC)

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```



## 0x02 Development Notes

### Language & Style

- Modern C++17 throughout the codebase
- Heavy use of RAII, strong typing, and template policies
- Policy-based design for buffer layouts and effect configuration

### Compute + Graphics Integration

- Compute passes write to storage buffers (e.g. particle positions / colors)
- Vertex shaders read from these buffers as read-only SSBOs
- Proper synchronization between compute and graphics queues via Vulkan barriers

### Extensibility

- Add new particle layouts by implementing a new layout policy and plugging it into `ParticleDataBuffer<ParticleT, LayoutPolicy>`
- Implement new effects by inheriting from `Compute_EffectBase` / `Graphic_EffectBase` and wiring up descriptor layouts & pipelines



## 0x03 Benchmarks

All measurements taken with Nsight Graphics on RTX 3080 Ti Mobile (Windows, MSVC Release build, validation layers disabled).

### 1. Resource upload pipeline rework

Reworked the resource upload path from a tutorial-style architecture (per-frame full re-uploads + serial blocking `vkQueueSubmit` + fence waits) into an
asynchronous system with a decoupled transfer queue, dedicated `ResourceManager`, and callback-driven scheduling.

| Pipeline           | Worst-case frame time | FPS (approx.) |
|--------------------|-----------------------|---------------|
| Tutorial baseline  | 424 ms                | ~2            |
| Async + decoupled  | 4.66 ms               | ~215          |

**~91×** reduction on the same scene, validated with Nsight GPU Trace.



### 2. Steady-state rendering performance

Scene: 700K triangles, 1.1–1.2K draw calls, IMMEDIATE_KHR present mode (VSync disabled) on a 72 Hz display.

| Metric                         | Value         |
|--------------------------------|---------------|
| Frame rate                     | 268 FPS       |
| Frame time                     | ~3.73 ms      |
| CPU command submitting         | 0.02 ~ 0.1 ms |

Multi-queue overlap verified via Nsight Graphics frame analysis.



### 3. GPU-driven particle system

| Config                           | Value    |
|----------------------------------|----------|
| Particles per dispatch           | 16,384   |
| Compute dispatch time            | < 0.01 ms |
| Layout                           | SoA (coalesced access) |
| Buffer strategy                  | Double-buffered SSBO, ping-pong |



### 4. Next-step optimization targets (observed, not yet fixed)

Nsight Compute attribution on the steady-state pipeline shows ~15% L1TEX Long Scoreboard stalls in the graphics path, suggesting further pipeline scheduling
improvements are possible.



## 0x04 Roadmap / Ideas

Planned or potential future work:

- GPU-driven rendering via indirect draw (`vkCmdDrawIndirect`)
- Async compute queue support for particle updates
- More sample scenes (deferred shading, post-processing, etc.)
- Better tooling around shader compilation (Slang / GLSL build integration)
- Continuous Integration (CI) to test builds across platforms

