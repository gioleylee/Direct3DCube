# Direct3DCube

A simple Direct3D 12 project that renders a 3D cube. This project demonstrates the fundamentals of modern graphics programming, including 3D transformations, the rendering pipeline, and GPU resource management using DirectX 12.

---

## 📌 Overview

**Direct3DCube** builds upon basic Direct3D 12 concepts by introducing 3D rendering. It displays a cube in 3D space, showcasing how vertex data, transformation matrices, and shaders work together to render objects on screen.

This project is ideal for beginners who have some familiarity with DirectX and want to move from 2D rendering (like a rectangle) to 3D graphics.

---

## 🚀 Features

* Direct3D 12 initialization
* 3D cube rendering using vertex buffers
* World, view, and projection transformations
* Basic shader pipeline (vertex + pixel shaders)
* Depth buffer (z-buffer) for correct 3D rendering
* Double buffering

---

## 🛠 Requirements

* Windows 10 or Windows 11
* **Microsoft Visual Studio 2019 or 2022**
* Windows SDK (latest recommended)
* GPU with DirectX 12 support

---

## ▶️ Getting Started

### 1. Clone the repository

```bash id="cube1"
git clone https://github.com/YOUR_USERNAME/Direct3DCube.git
```

### 2. Open the project

* Open the `.sln` file in Visual Studio

### 3. Build

* Set configuration to `x64`
* Press `Ctrl + Shift + B`

### 4. Run

* Press `F5` to run with debugger
* Or `Ctrl + F5` to run without debugger

---

## 📁 Project Structure

```text id="cube2"
Direct3DCube/
│
├── Direct3DCube.sln
├── Direct3DCube/
│   ├── Direct3DCube.vcxproj
│   ├── src/
│   ├── include/
│   ├── shaders/
│   └── assets/
│
└── README.md
```

---

## 🧠 Learning Concepts

This project introduces key 3D graphics concepts:

* Coordinate systems (world, view, projection)
* Matrix transformations
* Depth testing (z-buffer)
* Perspective projection
* GPU pipeline stages in Direct3D 12

---

## ⚠️ Notes

* This is a learning/demo project, not production-ready
* Code is simplified for clarity
* Minimal error handling included

---

## 📚 Future Improvements

* Add cube rotation/animation
* Implement lighting (Phong/Blinn-Phong)
* Add textures
* Camera controls (FPS/free-look)
* Expand to multiple 3D objects

---

## 📄 License

This project is open source and available under the MIT License.

---

## 🙌 Acknowledgements

* Microsoft DirectX 12 documentation
* Graphics programming tutorials and community resources
