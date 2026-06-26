# PBR Renderer

基于 Cook-Torrance BRDF 的 PBR 渲染器，OpenGL 3.3 实现。

## 功能

- **Cook-Torrance BRDF**：GGX 法线分布 + Smith 几何函数 + Schlick 菲涅尔近似
- **Metallic-Roughness 工作流**：albedo、normal、metallic、roughness、ao 五张贴图输入
- **多光源系统**：4 盏点光源环绕旋转 + 方向光
- **IBL 环境光**：Spherical Map 采样
- Tone Mapping + Gamma 校正

## 如何运行

1. 用 Xcode 打开 `TryGL.xcodeproj`
2. 按 ⌘R 运行

## 环境

- macOS (Apple Silicon / Intel)
- Xcode 15+
- C++17
- OpenGL 3.3 Core
