# ARTrail

[English](README.md) | [简体中文](README.zh-CN.md)

## 项目概述
ARTrail 是一个基于 UE5.7 的轨迹可视化项目。仓库包含运行时插件、Blueprint/Niagara 资源、示例数据与自动化测试，构成从数据接入到实时渲染的完整链路。

轨迹数据会被解析为带时间戳的点集，由 World Subsystem 统一管理，按时间窗过滤后以位置/速度数组形式提供给玩法逻辑与 Niagara 效果使用。

## 核心能力
- 支持从字符串或文件解析轨迹 JSON，并进行结构校验。
- 支持异步加载 JSON 文件，避免阻塞游戏线程。
- 在 World Subsystem 中管理播放时间、播放速度与时间窗长度。
- 输出当前时间窗的位置与速度数组，供 Blueprint/Niagara 使用。
- 支持运行时 UDP 消息接入（工作线程接收 + 游戏线程队列轮询）。
- 提供解析器与子系统窗口逻辑的自动化测试。

## 运行时组件
- `UARTrailSubsystem`
  - 负责轨迹数据生命周期、播放状态、窗口缓存与 `Tick` 更新。
  - 处理异步加载完成后的状态写入，并维护时间戳有序轨迹。
  - 对外暴露 Blueprint API 与事件（`OnTrailsParsedFinished`）。
- `UARTrailBlueprintFunctionLibrary`
  - 实现 JSON 解析（文件/字符串）、输入校验与时间单位转换工具。
- `FARTrailUdpReceiver`
  - 封装 Socket 创建/启动/停止，并将接收包入队，供游戏线程安全消费。

## 数据模型
`FARTrail` 字段：
- `Timestamp`（`int64`，微秒）
- `Position`（`FVector`，运行时单位厘米；JSON 输入单位米）
- `Velocity`（`float`，米/秒）

期望的 JSON 结构：
```json
{
  "<timestamp_us>": {
    "position": [x, y, z],
    "velocity": 1.23
  }
}
```

## 播放与窗口机制
- 当前窗口定义为 `[CurrentTime - TrailDuration, CurrentTime]`。
- 子系统基于有序时间戳和双指针窗口更新，保证逐帧刷新效率。
- 通过 `SpeedRate`、`SetCurrentTime`、`SetTrailDurationSeconds` 控制播放行为。

## 快速使用
1. 打开 `Content/Level/ARtrailMap.umap`。
2. 保持示例数据在 `Content/trail-points.json`（或在同路径同文件名下替换内容）。
3. 在 Blueprint 中调用 `LoadTrailsFromJsonFileAsync("trail-points.json")`。
4. 在 `OnTrailsParsedFinished` 后，通过 `GetCurrentWindowArrays` 获取数据并驱动渲染/Niagara。
5. 使用播放参数（`bAdvanced`、`SpeedRate`、当前时间、窗口时长）控制动画。

## 项目结构
```text
ARTrail/
|- Source/
|- Plugins/ARTrailRuntime/
|  |- Source/ARTrailRuntime/Public/
|  \- Source/ARTrailRuntime/Private/
|- Content/
|  |- Level/ARtrailMap.umap
|  |- Blueprints/
|  \- trail-points*.json
|- Config/
|- README.md
\- README.zh-CN.md
```

## 关键代码入口
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Public/ARTrailSubsystem.h`
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Private/ARTrailSubsystem.cpp`
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Public/ARTrailBlueprintFunctionLibrary.h`
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Private/ARTrailBlueprintFunctionLibrary.cpp`
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Public/ARTrailUdpReceiver.h`
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Private/ARTrailUdpReceiver.cpp`

## 测试
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Private/Tests/ARTrailBlueprintFunctionLibraryTests.cpp`
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Private/Tests/ARTrailSubsystemTests.cpp`

## 兼容性
- 目标平台：Windows x64
- 引擎版本：Unreal Engine 5.7
