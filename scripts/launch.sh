#!/bin/bash

# AprilTag Detector System Launch Script
# This script starts all components of the AprilTag detection system

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Create logs directory with timestamp
LOG_DIR="$PROJECT_ROOT/logs/$(date +%Y-%m-%d_%H-%M-%S)"
mkdir -p "$LOG_DIR/system"

# Configuration directory
CONFIG_DIR="$PROJECT_ROOT/config"

# Binary directory
BIN_DIR="$PROJECT_ROOT/bin"

# Function to check if a process is running
is_process_running() {
    pgrep -f "$1" > /dev/null
}

# Function to start a process
start_process() {
    local name="$1"
    local cmd="$2"
    
    if is_process_running "$name"; then
        echo "[$name] Already running"
        return 0
    fi
    
    echo "[$name] Starting..."
    nohup $cmd > "$LOG_DIR/system/${name}.log" 2>&1 &
    
    # Wait a bit to check if it started
    sleep 1
    if is_process_running "$name"; then
        echo "[$name] Started successfully"
        return 0
    else
        echo "[$name] Failed to start"
        return 1
    fi
}

# Function to stop a process
stop_process() {
    local name="$1"
    
    if ! is_process_running "$name"; then
        echo "[$name] Not running"
        return 0
    fi
    
    echo "[$name] Stopping..."
    pkill -f "$name"
    
    # Wait for it to stop
    local count=0
    while is_process_running "$name" && [ $count -lt 10 ]; do
        sleep 1
        count=$((count + 1))
    done
    
    if ! is_process_running "$name"; then
        echo "[$name] Stopped"
        return 0
    else
        echo "[$name] Failed to stop"
        return 1
    fi
}

# Function to show status
show_status() {
    echo "=== AprilTag Detector System Status ==="
    echo ""
    
    local processes=("launcher" "web_ui" "pose_finder")
    
    # Add camera detectors
    if [ -d "$CONFIG_DIR/cameras" ]; then
        for config in "$CONFIG_DIR/cameras"/camera_*.json; do
            if [ -f "$config" ]; then
                local camera_id=$(basename "$config" .json | sed 's/camera_//')
                processes+=("camera_detector_$camera_id")
            fi
        done
    fi
    
    for process in "${processes[@]}"; do
        if is_process_running "$process"; then
            local pid=$(pgrep -f "$process" | head -1)
            echo "  $process: Running (PID: $pid)"
        else
            echo "  $process: Stopped"
        fi
    done
    
    echo ""
    echo "=== Logs ==="
    echo "  Log directory: $LOG_DIR"
    echo ""
}

# Main command handling
case "${1:-start}" in
    start)
        echo "=== Starting AprilTag Detector System ==="
        echo ""
        
        # Build if binaries don't exist
        if [ ! -d "$BIN_DIR" ] || [ ! -f "$BIN_DIR/launcher" ]; then
            echo "Building project..."
            cd "$PROJECT_ROOT"
            mkdir -p build
            cd build
            cmake .. -DCMAKE_BUILD_TYPE=Release
            make -j$(nproc)
            
            if [ ! -f "$BIN_DIR/launcher" ]; then
                echo "Build failed!"
                exit 1
            fi
        fi
        
        # Start launcher (which starts everything else)
        start_process "launcher" "$BIN_DIR/launcher"
        
        echo ""
        echo "System started. Access web UI at http://localhost:8080"
        echo "Logs are in: $LOG_DIR"
        ;;
    
    stop)
        echo "=== Stopping AprilTag Detector System ==="
        echo ""
        
        # Stop all processes in reverse order
        stop_process "camera_detector_"  # This will match all camera_detector_* processes
        stop_process "pose_finder"
        stop_process "web_ui"
        stop_process "launcher"
        
        echo ""
        echo "All processes stopped"
        ;;
    
    restart)
        echo "=== Restarting AprilTag Detector System ==="
        echo ""
        
        $0 stop
        sleep 2
        $0 start
        ;;
    
    status)
        show_status
        ;;
    
    logs)
        if [ -d "$LOG_DIR" ]; then
            tail -f "$LOG_DIR/system/"*.log
        else
            echo "No log directory found: $LOG_DIR"
        fi
        ;;
    
    build)
        echo "=== Building AprilTag Detector System ==="
        echo ""
        
        cd "$PROJECT_ROOT"
        mkdir -p build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j$(nproc)
        
        if [ -f "$BIN_DIR/launcher" ]; then
            echo ""
            echo "Build successful!"
        else
            echo ""
            echo "Build failed!"
            exit 1
        fi
        ;;
    
    *)
        echo "Usage: $0 {start|stop|restart|status|logs|build}"
        echo ""
        echo "Commands:"
        echo "  start    - Start all processes"
        echo "  stop     - Stop all processes"
        echo "  restart  - Restart all processes"
        echo "  status   - Show process status"
        echo "  logs     - Show logs"
        echo "  build    - Build the project"
        exit 1
        ;;
esac

exit 0
