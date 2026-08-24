# AprilTag Detector - System Architecture Plan

## Overview

This document outlines the architecture and implementation plan for a high-performance, production-grade AprilTag detection system. The system is designed for cross-platform compatibility (Windows, Linux, macOS), multi-architecture support (x86_64, ARM), and robust fault tolerance.

---

## 1. System Architecture

### 1.1 Design Principles

- **Modularity**: Each component runs as an independent process
- **Fault Isolation**: Component crashes do not affect the entire system
- **Self-Healing**: Automatic restart of failed components
- **Hot Configuration**: All settings modifiable without code redeployment
- **Cross-Platform**: Support for Windows, Linux, macOS on x86_64 and ARM

### 1.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        LAUNCHER (Main Process)                      │
│  - Process Manager                                                  │
│  - Configuration Loader                                            │
│  - System Monitor                                                  │
│  - Logger Initializer                                              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│  Camera 1    │ │  Camera 2    │ │  Camera N    │ │   Web UI    │
│  Process     │ │  Process     │ │  Process     │ │   Process    │
└──────┬──────┘ └──────┬──────┘ └──────┬──────┘ └──────┬──────┘
       │               │               │              │
       ▼               ▼               ▼              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        NT4 COMMUNICATION BUS                        │
│  - Custom NT4 Library                                              │
│  - Topic-based publish/subscribe                                   │
│  - Robot Data Exchange                                             │
└─────────────────────────────────────────────────────────────────┘
       │               │               │              │
       ▼               ▼               ▼              │
┌─────────────┐ ┌─────────────┐ ┌─────────────┐        │
│  Pose Finder │ │  Pose Finder │ │  Pose Finder │        │
│  Process     │ │  Process     │ │  Process     │        │
└─────────────┘ └─────────────┘ └─────────────┘        │
                                                      ▼
                                              ┌─────────────┐
                                              │   External  │
                                              │   Robot     │
                                              └─────────────┘
```

---

## 2. Directory Structure

```
Apriltag-Detector/
├── Plan.md                           # This document
├── README.md                         # Project overview
├── .gitignore                        # Git ignore rules
├── CMakeLists.txt                    # Root CMake configuration
│
├── cmake/                            # CMake modules and toolchain files
│   ├── CrossCompileARM.cmake         # ARM cross-compilation
│   ├── WindowsToolchain.cmake        # Windows-specific toolchain
│   ├── LinuxToolchain.cmake          # Linux-specific toolchain
│   └── MacToolchain.cmake            # macOS-specific toolchain
│
├── src/                              # Source code
│   ├── common/                       # Shared libraries
│   │   ├── logging/                  # Internal logging system
│   │   │   ├── Logger.hpp            # Logger interface
│   │   │   ├── Logger.cpp            # Logger implementation
│   │   │   └── FileSink.hpp          # File output sink
│   │   │
│   │   ├── config/                   # Configuration system
│   │   │   ├── ConfigManager.hpp     # Config loading/management
│   │   │   ├── ConfigWatcher.hpp     # File change detection
│   │   │   ├── JsonConfig.hpp        # JSON config parser
│   │   │   └── schema/               # Config schemas
│   │   │
│   │   ├── ipc/                      # Inter-process communication
│   │   │   ├── ProcessManager.hpp    # Process lifecycle management
│   │   │   ├── RestartPolicy.hpp     # Self-healing policies
│   │   │   └── HealthCheck.hpp       # Component health monitoring
│   │   │
│   │   ├── camera/                   # Camera abstraction layer
│   │   │   ├── CameraInterface.hpp   # Abstract camera interface
│   │   │   ├── V4L2Camera.hpp        # Linux V4L2 implementation
│   │   │   ├── WMFCamera.hpp         # Windows Media Foundation
│   │   │   ├── AVCaptureCamera.hpp   # macOS AVFoundation
│   │   │   └── CameraFactory.hpp     # Factory pattern for camera creation
│   │   │
│   │   ├── nt4/                      # Custom NT4 Library
│   │   │   ├── NT4Client.hpp         # NT4 client implementation
│   │   │   ├── NT4Publisher.hpp      # Data publishing
│   │   │   ├── NT4Subscriber.hpp     # Data subscription
│   │   │   └── NT4Types.hpp          # Type definitions
│   │   │
│   │   └── utils/                    # Utilities
│   │       ├── Platform.hpp          # Platform detection/abstraction
│   │       ├── FileSystem.hpp        # Cross-platform filesystem
│   │       └── TimeUtils.hpp          # Timing utilities
│   │
│   ├── launcher/                     # Main launcher program
│   │   ├── main.cpp                  # Entry point
│   │   ├── Launcher.hpp              # Launcher class
│   │   └── SystemMonitor.hpp         # System health monitoring
│   │
│   ├── camera_detector/             # AprilTag detector per camera
│   │   ├── main.cpp                  # Detector entry point
│   │   ├── AprilTagDetector.hpp      # Detector implementation
│   │   ├── TagDetection.cpp          # Detection algorithm
│   │   └── DetectorConfig.hpp        # Detector-specific config
│   │
│   ├── pose_finder/                  # Pose estimation
│   │   ├── main.cpp                  # Pose finder entry point
│   │   ├── PoseEstimator.hpp         # Pose calculation
│   │   ├── PoseSolver.hpp            # PnP solver
│   │   └── PoseConfig.hpp            # Pose configuration
│   │
│   └── web_ui/                       # Web configuration interface
│       ├── main.cpp                  # Web server entry point
│       ├── WebServer.hpp             # HTTP server
│       ├── StreamHandler.hpp         # Camera stream serving
│       ├── ConfigAPI.hpp             # REST API for configuration
│       └── web/                      # Web assets
│           ├── index.html            # Main page
│           ├── config.html           # Configuration page
│           ├── stream.html           # Stream viewer
│           ├── css/                  # Stylesheets
│           └── js/                   # JavaScript
│
├── config/                           # Configuration files
│   ├── system.json                   # System-wide configuration
│   ├── cameras/                      # Per-camera configurations
│   │   ├── camera_0.json             # Camera 0 config (intrinsics/extrinsics)
│   │   ├── camera_1.json             # Camera 1 config
│   │   └── ...
│   └── web.json                      # Web server configuration
│
├── scripts/                          # Utility scripts
│   ├── launch.sh                     # Linux/macOS launch script
│   ├── launch.bat                    # Windows launch script
│   ├── build_arm.sh                  # ARM build script
│   └── generate_config.py            # Config generator
│
├── build/                            # Build output (gitignored)
│   ├── linux_x64/                    # Linux x86_64 binaries
│   ├── linux_arm/                    # Linux ARM binaries
│   ├── windows_x64/                  # Windows x86_64 binaries
│   ├── windows_arm/                  # Windows ARM binaries
│   ├── mac_x64/                      # macOS x86_64 binaries
│   └── mac_arm/                      # macOS ARM binaries
│
├── logs/                             # Log files
│   └── <launch_time>/                # Timestamped log directory
│       └── system/                   # System logs
│           ├── launcher.log          # Launcher process log
│           ├── camera_0.log          # Camera 0 detector log
│           ├── camera_1.log          # Camera 1 detector log
│           ├── pose_finder_0.log     # Pose finder log
│           ├── web_ui.log            # Web UI log
│           └── system_monitor.log    # System monitor log
│
└── third_party/                      # Third-party dependencies
    ├── apriltag/                     # AprilTag library
    ├── opencv/                       # OpenCV (optional, for image processing)
    └── nlohmann_json/                # JSON library
```

---

## 3. Component Specifications

### 3.1 Launcher (Single Launch File)

**Purpose**: Master process that starts, monitors, and manages all other components.

**Responsibilities**:
- Parse system configuration
- Initialize logging system
- Start all camera detector processes
- Start pose finder processes
- Start web UI process
- Monitor component health
- Implement self-healing (auto-restart)
- Provide clean shutdown

**Implementation**:
```cpp
// launch.cpp - Single entry point for the entire system
int main(int argc, char* argv[]) {
    // Initialize logging
    Logger::initialize("./logs/<timestamp>/system/");
    
    // Load configuration
    auto config = ConfigManager::load("config/system.json");
    
    // Start process manager
    ProcessManager pm(config);
    
    // Start all components
    pm.startComponent("camera_detector", config.cameras);
    pm.startComponent("pose_finder", config.cameras.size());
    pm.startComponent("web_ui", config.web);
    
    // Monitor and self-heal
    pm.monitor();
    
    return 0;
}
```

**Platform-Specific Launch**:
- `launch.sh` for Linux/macOS
- `launch.bat` for Windows
- Both invoke the same binary with appropriate arguments

### 3.2 Camera Detector (Per-Camera Process)

**Purpose**: Detect AprilTags from a single camera feed.

**Responsibilities**:
- Initialize specific camera using system API
- Configure camera settings (exposure, brightness, etc.)
- Capture frames
- Run AprilTag detection
- Publish detection results via NT4
- Stream raw and processed frames

**Configuration File** (`config/cameras/camera_<n>.json`):
```json
{
  "camera_id": 0,
  "device_path": "/dev/video0",
  "camera_type": "v4l2",
  "resolution": {
    "width": 1280,
    "height": 720
  },
  "fps": 30,
  "exposure": 50,
  "brightness": 50,
  "contrast": 50,
  "saturation": 50,
  "gain": 0,
  "intrinsics": {
    "fx": 800.0,
    "fy": 800.0,
    "cx": 640.0,
    "cy": 360.0,
    "distortion": [0.0, 0.0, 0.0, 0.0, 0.0]
  },
  "extrinsics": {
    "translation": [0.0, 0.0, 0.0],
    "rotation": [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
  },
  "apriltag": {
    "family": "tag36h11",
    "min_size": 0.1,
    "max_size": 10.0,
    "detect_threshold": 0.5
  },
  "nt4": {
    "publish_topic": "/camera_0/detections",
    "robot_address": "10.0.0.2"
  }
}
```

**Platform-Specific Camera APIs**:

| Platform | API | Implementation |
|----------|-----|----------------|
| Linux | V4L2 | `V4L2Camera.hpp` |
| Windows | WMF (Windows Media Foundation) | `WMFCamera.hpp` |
| macOS | AVFoundation | `AVCaptureCamera.hpp` |

### 3.3 Pose Finder (Per-Camera Process)

**Purpose**: Calculate robot pose from AprilTag detections.

**Responsibilities**:
- Subscribe to detection data from camera detectors
- Apply pose estimation algorithm (PnP)
- Publish calculated pose via NT4
- Handle multiple tags for robust pose estimation

**Configuration**: Shared with camera detector or separate pose config.

### 3.4 Web UI (Single Process)

**Purpose**: Provide web interface for configuration and monitoring.

**Responsibilities**:
- Host HTTP server on OS-specific port
- Serve camera streams (raw and processed)
- Provide configuration UI
- Display detection overlays
- Show system status

**OS-Specific Port Libraries**:
- Use platform-specific optimal port selection
- Linux: epoll
- Windows: IOCP
- macOS: kqueue

**Endpoints**:
- `GET /` - Main dashboard
- `GET /stream/camera_<n>/raw` - Raw camera feed
- `GET /stream/camera_<n>/processed` - Processed feed with detections
- `GET /config` - Configuration editor
- `GET /api/cameras` - List cameras
- `GET /api/cameras/<n>/config` - Get camera config
- `POST /api/cameras/<n>/config` - Update camera config
- `GET /api/system/status` - System health status

### 3.5 Custom NT4 Library

**Purpose**: Robot communication via NetworkTables 4 protocol.

**Responsibilities**:
- Establish connection to robot
- Publish detection and pose data
- Subscribe to robot data if needed
- Handle connection retries
- Manage topic lifecycle

**Interface**:
```cpp
class NT4Client {
public:
    void connect(const std::string& address);
    void publish(const std::string& topic, const Data& data);
    void subscribe(const std::string& topic, Callback cb);
    void disconnect();
    bool isConnected() const;
};
```

---

## 4. Cross-Platform Support

### 4.1 Supported Platforms

| Platform | Architecture | Camera API | Build System |
|----------|--------------|------------|--------------|
| Linux | x86_64 | V4L2 | CMake |
| Linux | ARM64 | V4L2 | CMake |
| Linux | ARM32 | V4L2 | CMake |
| Windows | x86_64 | WMF | CMake |
| Windows | ARM64 | WMF | CMake |
| macOS | x86_64 | AVFoundation | CMake |
| macOS | ARM64 | AVFoundation | CMake |

### 4.2 Platform Abstraction

**`Platform.hpp`**:
```cpp
#pragma once

#if defined(_WIN32)
    #define PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #define PLATFORM_MACOS 1
#elif defined(__linux__)
    #define PLATFORM_LINUX 1
#endif

#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH_X64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
    #define ARCH_ARM32 1
#endif

// String identifiers
constexpr const char* getPlatformString() {
    #if PLATFORM_WINDOWS
        return "windows";
    #elif PLATFORM_MACOS
        return "macos";
    #elif PLATFORM_LINUX
        return "linux";
    #endif
}

constexpr const char* getArchString() {
    #if ARCH_X64
        return "x64";
    #elif ARCH_ARM64
        return "arm64";
    #elif ARCH_ARM32
        return "arm";
    #endif
}
```

### 4.3 Camera API Abstraction

**`CameraInterface.hpp`**:
```cpp
#pragma once

#include <vector>
#include <memory>

struct CameraFrame {
    std::vector<uint8_t> data;
    int width;
    int height;
    int channels;
    uint64_t timestamp;
};

struct CameraIntrinsics {
    double fx, fy;
    double cx, cy;
    std::vector<double> distortion;
};

struct CameraExtrinsics {
    std::vector<double> translation;  // [x, y, z]
    std::vector<double> rotation;    // 3x3 matrix or quaternion
};

class CameraInterface {
public:
    virtual ~CameraInterface() = default;
    
    virtual bool open(int deviceIndex, const std::string& configPath) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    
    virtual bool captureFrame(CameraFrame& frame) = 0;
    
    virtual bool setResolution(int width, int height) = 0;
    virtual bool setFPS(int fps) = 0;
    virtual bool setExposure(int value) = 0;
    virtual bool setBrightness(int value) = 0;
    virtual bool setContrast(int value) = 0;
    virtual bool setSaturation(int value) = 0;
    virtual bool setGain(int value) = 0;
    
    virtual bool getIntrinsics(CameraIntrinsics& intrinsics) const = 0;
    virtual bool setIntrinsics(const CameraIntrinsics& intrinsics) = 0;
    virtual bool getExtrinsics(CameraExtrinsics& extrinsics) const = 0;
    virtual bool setExtrinsics(const CameraExtrinsics& extrinsics) = 0;
    
    static std::unique_ptr<CameraInterface> create(const std::string& type);
};
```

### 4.4 Build Configuration

**CMake Toolchain Selection**:

```cmake
# Detect platform and architecture
if(WIN32)
    set(PLATFORM "windows")
elseif(APPLE)
    set(PLATFORM "macos")
elseif(UNIX)
    set(PLATFORM "linux")
endif()

# Detect architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH "x64")
else()
    set(ARCH "x86")
endif()

# ARM detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(ARCH "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(ARCH "arm")
endif()

# Platform-specific settings
set(BUILD_DIR "${CMAKE_BINARY_DIR}/${PLATFORM}_${ARCH}")
```

---

## 5. Configuration System

### 5.1 Configuration Hierarchy

```
System Configuration (system.json)
├── cameras: [CameraConfig]
│   ├── device_path
│   ├── camera_type (v4l2/wmf/avfoundation)
│   ├── resolution
│   ├── fps
│   └── ...
├── pose_finders: [PoseConfig]
├── web: WebConfig
│   ├── port
│   ├── bind_address
│   └── ...
├── nt4: NT4Config
│   ├── robot_address
│   ├── team_number
│   └── ...
└── logging: LogConfig
    ├── log_level
    ├── log_directory
    └── rotation_policy

Camera Configuration (cameras/camera_<n>.json)
├── intrinsics
│   ├── fx, fy
│   ├── cx, cy
│   └── distortion
├── extrinsics
│   ├── translation
│   └── rotation
├── camera_settings
│   ├── exposure
│   ├── brightness
│   ├── contrast
│   ├── saturation
│   └── gain
└── apriltag
    ├── family
    ├── min_size
    ├── max_size
    └── detect_threshold
```

### 5.2 Hot Configuration

**Implementation Strategy**:
- ConfigWatcher monitors config files for changes
- On change, components receive configuration update events
- Components apply new settings without restart (where possible)
- For settings requiring restart, ProcessManager handles graceful restart

**ConfigWatcher.hpp**:
```cpp
class ConfigWatcher {
public:
    using ConfigCallback = std::function<void(const std::string&, const json&)>;
    
    void watch(const std::string& filePath, ConfigCallback callback);
    void unwatch(const std::string& filePath);
    void poll();  // Check for changes
};
```

---

## 6. Logging System

### 6.1 Logger Specification

**Output Location**: `./logs/<launch_time>/system/`

**Format**:
```
[YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [PROCESS_NAME:PID] [FILE:LINE] message
```

**Levels**:
- TRACE
- DEBUG
- INFO
- WARN
- ERROR
- CRITICAL

**Rotation**:
- Daily rotation
- Size-based rotation (10MB default)
- Keep last 30 days

**Logger.hpp**:
```cpp
#pragma once

#include <string>
#include <memory>

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static void initialize(const std::string& logDir);
    static void shutdown();
    
    static void setLevel(LogLevel level);
    static void setProcessName(const std::string& name);
    
    static void trace(const std::string& msg);
    static void debug(const std::string& msg);
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);
    static void critical(const std::string& msg);
    
    // With format
    template<typename... Args>
    static void trace(const std::string& fmt, Args... args);
    template<typename... Args>
    static void debug(const std::string& fmt, Args... args);
    // ... etc
};

// Macro for file:line
#define LOG_TRACE(msg) Logger::trace("[%s:%d] %s", __FILE__, __LINE__, msg)
#define LOG_DEBUG(msg) Logger::debug("[%s:%d] %s", __FILE__, __LINE__, msg)
// ... etc
```

### 6.2 Log Directory Structure

```
logs/
└── 2024-01-15_14-30-00/           # Launch timestamp
    └── system/
        ├── launcher_<pid>.log
        ├── camera_detector_0_<pid>.log
        ├── camera_detector_1_<pid>.log
        ├── pose_finder_0_<pid>.log
        ├── pose_finder_1_<pid>.log
        └── web_ui_<pid>.log
```

---

## 7. Process Management & Self-Healing

### 7.1 Process Manager

**Responsibilities**:
- Start child processes
- Monitor process health
- Handle crashes with restart policies
- Manage inter-process communication
- Coordinate clean shutdown

**ProcessManager.hpp**:
```cpp
class ProcessManager {
public:
    struct ProcessConfig {
        std::string name;
        std::string executable;
        std::vector<std::string> arguments;
        std::string workingDirectory;
        int restartPolicy;  // 0=none, 1=always, 2=on_failure
        int maxRestartAttempts;
        int restartDelayMs;
    };
    
    void startProcess(const ProcessConfig& config);
    void stopProcess(const std::string& name);
    void stopAll();
    
    bool isRunning(const std::string& name) const;
    int getPid(const std::string& name) const;
    
    void monitor();  // Main monitoring loop
};
```

### 7.2 Health Monitoring

**HealthCheck.hpp**:
```cpp
class HealthMonitor {
public:
    struct HealthStatus {
        bool isAlive;
        uint64_t lastHeartbeat;
        double cpuUsage;
        double memoryUsage;
        std::string statusMessage;
    };
    
    void registerProcess(const std::string& name, pid_t pid);
    void unregisterProcess(const std::string& name);
    
    HealthStatus getStatus(const std::string& name) const;
    std::map<std::string, HealthStatus> getAllStatus() const;
    
    void checkHeartbeats();  // Check for stale heartbeats
};
```

### 7.3 Heartbeat System

Each process sends periodic heartbeats to the launcher:
- Frequency: Every 1 second
- Timeout: 3 seconds (process considered dead)
- Action: Launcher restarts process based on restart policy

---

## 8. Inter-Process Communication

### 8.1 Communication Methods

| Purpose | Method | Library |
|---------|--------|---------|
| Config Updates | Filesystem + Watcher | std::filesystem |
| Health Monitoring | Shared Memory / Sockets | Custom |
| Stream Sharing | Shared Memory | Custom |
| NT4 Data | Network Sockets | Custom NT4 |

### 8.2 Shared Memory for Camera Streams

**Efficient IPC for video streams**:
```cpp
class SharedMemoryFrameBuffer {
public:
    SharedMemoryFrameBuffer(const std::string& name, size_t size);
    ~SharedMemoryFrameBuffer();
    
    bool writeFrame(const CameraFrame& frame);
    bool readFrame(CameraFrame& frame);
    
    bool lock();
    void unlock();
};
```

### 8.3 NT4 Communication Flow

```
Camera Detector → NT4 Publish → Robot
Camera Detector → NT4 Publish → Pose Finder
Pose Finder → NT4 Publish → Robot
Web UI → NT4 Subscribe ← Camera Detector
Web UI → NT4 Subscribe ← Pose Finder
```

---

## 9. AprilTag Detection

### 9.1 Detection Pipeline

```
Camera Frame
    ▼
Preprocessing (optional)
    ▼
AprilTag Detection
    ▼
Tag Decoding
    ▼
Detection Data + Frame
    ▼
Publish via NT4
    ▼
Stream Output (processed)
```

### 9.2 Detection Data Structure

```cpp
struct AprilTagDetection {
    int id;                     // Tag ID
    double centerX, centerY;    // Image coordinates
    double corners[4][2];       // Four corners
    double pose[3];             // Translation (if solved)
    double orientation[4];     // Quaternion (if solved)
    double confidence;          // Detection confidence
    uint64_t timestamp;         // Frame timestamp
};

struct DetectionFrame {
    std::vector<AprilTagDetection> detections;
    CameraFrame frame;
    int cameraId;
};
```

---

## 10. Pose Estimation

### 10.1 Pose Solver

**Input**: Multiple tag detections from a camera
**Output**: Camera pose relative to tag field

**Algorithm**:
1. For each detected tag:
   - Get known tag pose in world coordinates
   - Use camera intrinsics
   - Solve PnP (Perspective-n-Point)
2. Combine multiple tag poses using RANSAC or averaging
3. Publish final pose

**PoseSolver.hpp**:
```cpp
class PoseSolver {
public:
    struct PoseResult {
        std::vector<double> translation;  // [x, y, z]
        std::vector<double> rotation;    // quaternion [w, x, y, z]
        double confidence;
        std::vector<int> usedTagIds;
    };
    
    PoseResult solvePose(
        const std::vector<AprilTagDetection>& detections,
        const CameraIntrinsics& intrinsics,
        const std::map<int, TagPose>& knownTagPoses
    );
};
```

### 10.2 Known Tag Configuration

```json
{
  "tags": [
    {
      "id": 0,
      "pose": {
        "translation": [0.0, 0.0, 0.0],
        "rotation": [1.0, 0.0, 0.0, 0.0]
      },
      "size": 0.1651  // Tag size in meters
    },
    {
      "id": 1,
      "pose": {
        "translation": [1.0, 0.0, 0.0],
        "rotation": [1.0, 0.0, 0.0, 0.0]
      },
      "size": 0.1651
    }
  ]
}
```

---

## 11. Web UI Implementation

### 11.1 HTTP Server

**Cross-Platform Server**:
- Use platform-optimal networking APIs
- Linux: epoll
- Windows: IOCP
- macOS: kqueue

**WebServer.hpp**:
```cpp
class WebServer {
public:
    using RequestHandler = std::function<void(const HttpRequest&, HttpResponse&)>;
    
    bool start(int port, const std::string& bindAddress = "0.0.0.0");
    void stop();
    
    void addRoute(const std::string& path, RequestHandler handler);
    void addStaticRoute(const std::string& path, const std::string& directory);
    
    void run();  // Blocking
};
```

### 11.2 Stream Handler

**Efficient streaming of camera feeds**:
```cpp
class StreamHandler {
public:
    // MJPEG streaming
    void handleRawStream(int cameraId, HttpResponse& response);
    void handleProcessedStream(int cameraId, HttpResponse& response);
    
    // Snapshot
    void handleSnapshot(int cameraId, bool processed, HttpResponse& response);
};
```

### 11.3 Configuration API

**REST Endpoints**:
```cpp
class ConfigAPI {
public:
    void registerEndpoints(WebServer& server);
    
private:
    void handleGetCameras(HttpRequest& req, HttpResponse& res);
    void handleGetCameraConfig(HttpRequest& req, HttpResponse& res);
    void handlePostCameraConfig(HttpRequest& req, HttpResponse& res);
    void handleGetSystemStatus(HttpRequest& req, HttpResponse& res);
};
```

---

## 12. Custom NT4 Library

### 12.1 NT4 Protocol Overview

- Binary protocol over TCP
- Topic-based publish/subscribe
- Type-safe data serialization
- Connection management

### 12.2 NT4 Client Implementation

**NT4Client.hpp**:
```cpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

class NT4Client {
public:
    using SubscribeCallback = std::function<void(const std::string&, const std::vector<uint8_t>&)>;
    
    NT4Client();
    ~NT4Client();
    
    // Connection
    bool connect(const std::string& address, int port = 5810);
    void disconnect();
    bool isConnected() const;
    
    // Publishing
    bool publish(const std::string& topic, const std::vector<uint8_t>& data);
    template<typename T>
    bool publish(const std::string& topic, const T& data);
    
    // Subscription
    bool subscribe(const std::string& topic, SubscribeCallback callback);
    void unsubscribe(const std::string& topic);
    
    // Robot data
    bool publishRobotPose(const std::vector<double>& pose);
    bool publishTagDetections(int cameraId, const std::vector<AprilTagDetection>& detections);
    
    // Utility
    void run();  // Process network events
    void stop();
};
```

### 12.3 NT4 Message Types

```cpp
namespace nt4 {
    
    enum class MessageType {
        PUBLISH,
        SUBSCRIBE,
        UNSUBSCRIBE,
        DATA,
        PING,
        PONG
    };
    
    struct MessageHeader {
        uint32_t magic;           // NT4 magic number
        uint8_t version;         // Protocol version
        uint8_t messageType;     // MessageType enum
        uint16_t topicId;         // Topic identifier
        uint32_t payloadSize;    // Payload size in bytes
        uint64_t timestamp;       // Message timestamp
    };
    
}
```

---

## 13. Build System

### 13.1 CMake Configuration

**Root CMakeLists.txt**:
```cmake
cmake_minimum_required(VERSION 3.20)
project(ApriltagDetector)

# Platform detection
include(${CMAKE_SOURCE_DIR}/cmake/PlatformDetection.cmake)

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_WEB_UI "Build web UI" ON)

# Dependencies
add_subdirectory(third_party/apriltag)
add_subdirectory(third_party/nlohmann_json)

# Common libraries
add_subdirectory(src/common)

# Applications
add_subdirectory(src/launcher)
add_subdirectory(src/camera_detector)
add_subdirectory(src/pose_finder)
add_subdirectory(src/web_ui)

# Install
install(TARGETS launcher camera_detector pose_finder web_ui
    RUNTIME DESTINATION bin
)
```

### 13.2 Platform-Specific Builds

**Cross-compilation for ARM**:
```bash
# Linux ARM64
mkdir -p build/linux_arm64
cd build/linux_arm64
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/CrossCompileARM64.cmake ..
make -j$(nproc)

# Windows ARM64
mkdir -p build/windows_arm64
cd build/windows_arm64
cmake -G "Visual Studio 17 2022" -A ARM64 ..
cmake --build . --config Release
```

---

## 14. Deployment

### 14.1 Package Structure

```
ApriltagDetector-v1.0.0-<platform>-<arch>.tar.gz / .zip
├── bin/
│   ├── launcher
│   ├── camera_detector
│   ├── pose_finder
│   └── web_ui
├── config/
│   ├── system.json
│   └── cameras/
│       └── camera_0.json
├── web/
│   ├── index.html
│   ├── config.html
│   ├── stream.html
│   ├── css/
│   └── js/
├── scripts/
│   ├── launch.sh
│   └── launch.bat
├── README.md
└── LICENSE
```

### 14.2 Installation

**Linux/macOS**:
```bash
tar -xzf ApriltagDetector-v1.0.0-linux-x64.tar.gz -C /opt/apriltag
export PATH=$PATH:/opt/apriltag/bin
```

**Windows**:
- Extract ZIP to `C:\Program Files\ApriltagDetector`
- Add to PATH

---

## 15. Development Phases

### Phase 1: Foundation (Week 1-2)
- [ ] Set up project structure
- [ ] Implement platform abstraction layer
- [ ] Create logging system
- [ ] Implement configuration system
- [ ] Build cross-platform CMake setup

### Phase 2: Core Components (Week 3-4)
- [ ] Implement camera abstraction layer
- [ ] Create V4L2 camera implementation
- [ ] Create WMF camera implementation
- [ ] Create AVFoundation camera implementation
- [ ] Implement AprilTag detection

### Phase 3: Communication (Week 5-6)
- [ ] Implement custom NT4 library
- [ ] Create inter-process communication
- [ ] Implement process manager
- [ ] Add self-healing mechanisms

### Phase 4: Pose Estimation (Week 7)
- [ ] Implement pose solver
- [ ] Create pose finder service
- [ ] Integrate with detection

### Phase 5: Web Interface (Week 8-9)
- [ ] Implement HTTP server
- [ ] Create stream handler
- [ ] Build configuration API
- [ ] Design and implement web UI

### Phase 6: Integration & Testing (Week 10-12)
- [ ] Integrate all components
- [ ] Cross-platform testing
- [ ] Performance optimization
- [ ] Create launch scripts
- [ ] Documentation

### Phase 7: Deployment (Week 13)
- [ ] Package for all platforms
- [ ] Create installation instructions
- [ ] Final testing

---

## 16. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Camera API differences | High | High | Abstract early, test on all platforms |
| Performance on ARM | Medium | High | Optimize algorithms, test on target hardware |
| NT4 protocol compatibility | Medium | High | Study NT4 spec, test with robot |
| Cross-platform networking | Medium | Medium | Use portable libraries, test thoroughly |
| Memory usage with multiple cameras | Medium | Medium | Use shared memory, efficient streaming |

---

## 17. Dependencies

### 17.1 Required

| Dependency | Purpose | License |
|------------|---------|---------|
| AprilTag Library | Tag detection | BSD |
| nlohmann/json | JSON parsing | MIT |

### 17.2 Optional

| Dependency | Purpose | License |
|------------|---------|---------|
| OpenCV | Image processing | Apache 2.0 |
| libuv | Async I/O | BSD |
| libjpeg | JPEG encoding | Custom |

---

## 18. Performance Considerations

### 18.1 Optimization Strategies

- **Camera Processing**: Use hardware-accelerated APIs where available
- **AprilTag Detection**: Use SIMD optimizations
- **IPC**: Shared memory for high-bandwidth data (video streams)
- **NT4**: Batch small messages, use efficient serialization
- **Web Streaming**: MJPEG for compatibility, H.264 for efficiency (optional)

### 18.2 Resource Limits

- Memory: < 500MB per camera process
- CPU: < 50% per camera on target hardware
- Latency: < 50ms detection + pose estimation

---

## 19. Testing Strategy

### 19.1 Unit Tests

- Platform abstraction layer
- Configuration parsing
- Logging system
- NT4 message serialization
- Pose solver

### 19.2 Integration Tests

- Camera detection with test images
- IPC between components
- NT4 communication with mock robot
- Web UI functionality

### 19.3 System Tests

- Full system with real cameras
- Multiple camera scenarios
- Fault injection (kill processes)
- Configuration changes during operation

### 19.4 Cross-Platform Tests

- Build on all target platforms
- Run tests on all platforms
- Performance benchmarking

---

## 20. Success Criteria

- [ ] System runs on Windows, Linux, macOS (x86_64 and ARM)
- [ ] All components are independent processes
- [ ] Configuration can be changed without redeployment
- [ ] System automatically recovers from component crashes
- [ ] Logs are written to `./logs/<launch_time>/system/`
- [ ] Single launch file starts the entire system
- [ ] Each camera has its own detector process
- [ ] Detectors communicate with pose finders
- [ ] Each camera has individual configuration file
- [ ] Web UI shows streams and allows configuration
- [ ] Uses system camera APIs (V4L2, WMF, AVFoundation)
- [ ] Custom NT4 library publishes data to robot

---

## Appendix A: Configuration Examples

### system.json
```json
{
  "version": "1.0",
  "log_level": "INFO",
  "log_directory": "./logs",
  "num_cameras": 2,
  "nt4": {
    "robot_address": "10.0.0.2",
    "team_number": 0,
    "publish_rate": 30
  },
  "web": {
    "port": 8080,
    "bind_address": "0.0.0.0",
    "enable_auth": false
  },
  "self_healing": {
    "enabled": true,
    "restart_delay": 1000,
    "max_attempts": 5
  }
}
```

### camera_0.json
```json
{
  "camera_id": 0,
  "device_path": "/dev/video0",
  "camera_type": "v4l2",
  "resolution": {
    "width": 1280,
    "height": 720
  },
  "fps": 30,
  "settings": {
    "exposure": 50,
    "brightness": 50,
    "contrast": 50,
    "saturation": 50,
    "gain": 0
  },
  "intrinsics": {
    "fx": 800.0,
    "fy": 800.0,
    "cx": 640.0,
    "cy": 360.0,
    "distortion": [0.0, 0.0, 0.0, 0.0, 0.0]
  },
  "extrinsics": {
    "translation": [0.0, 0.0, 0.0],
    "rotation": [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
  },
  "apriltag": {
    "family": "tag36h11",
    "min_size": 0.1,
    "max_size": 10.0,
    "detect_threshold": 0.5
  }
}
```

---

*Document Version: 1.0*
*Last Updated: $(date)*
*Author: System Architect*
