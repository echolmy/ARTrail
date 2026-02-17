# ARTrail

## Project Overview
ARTrail is a trail visualization project built with Unreal Engine.  
Its core runtime capability is provided by the `ARTrailRuntime` plugin, which converts external JSON data into UE-consumable data streams (position/velocity) and drives runtime behavior with data.

## Features
- Parse trail JSON (string / file).
- Load trail files asynchronously to avoid blocking the game thread.
- Filter trail points by current time and window duration through `WorldSubsystem`.
- Output position and velocity arrays in the current window for direct use in Blueprint or Niagara.
- Dynamically adjust trail color and width by velocity: higher speed makes trails thicker and yellow, lower speed makes trails thinner and blue.
- Playback speed can be adjusted to control Niagara animation speed.
- Provide automated tests for JSON parsing and windowing logic.

## Controls
Hold the left mouse button to rotate.
Hold the right mouse button and use WASD to pan.
Use the mouse wheel to zoom.

## Project Structure
```text
ARTrail/
|- Source/
|- Plugins/ARTrailRuntime/             # Core runtime logic
|  |- Source/ARTrailRuntime/Public/
|  \- Source/ARTrailRuntime/Private/
|- Content/
|  |- Level/ARtrailMap.umap
|  |- Blueprints/                      # Blueprint and Niagara assets
|  \- trail-points*.json               # Sample trail data
|- Config/
\- ExternalDataToUE.md                 # External data integration design notes
```

Note: To replace the test JSON file, overwrite `trail-points.json` at the original path and keep the same filename.

## Key Code Entry Points
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Public/ARTrailSubsystem.h`  
  trail subsystem that manages time, windowing, and cached arrays.
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Private/ARTrailSubsystem.cpp`  
  Core logic for async loading and two-pointer sliding-window updates.
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Public/ARTrailBlueprintFunctionLibrary.h`  
  Blueprint-callable parsing and time-conversion utility functions.
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Private/ARTrailBlueprintFunctionLibrary.cpp`  
  JSON parsing implementation, including field validation and unit conversion.

## Tests
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Private/Tests/ARTrailBlueprintFunctionLibraryTests.cpp`
- `Plugins/ARTrailRuntime/Source/ARTrailRuntime/Private/Tests/ARTrailSubsystemTests.cpp`

## Compatibility
- Target platform: Windows x64
- Engine version: Unreal Engine 5.7
