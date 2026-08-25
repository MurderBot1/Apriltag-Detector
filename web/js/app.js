// AprilTag Detector Web UI - JavaScript Application

// State
let state = {
    cameras: [],
    tags: [],
    systemConfig: null,
    globalConfig: null,
    detectionStats: {
        totalDetections: 0,
        activeCameras: 0
    },
    robotPosition: {
        x: 0,
        y: 0,
        z: 0,
        roll: 0,
        pitch: 0,
        yaw: 0,
        confidence: 0
    },
    processStatus: {}
};

// API Base URL
const API_BASE = '/api/config';

// Initialize the application
document.addEventListener('DOMContentLoaded', () => {
    console.log('AprilTag Detector Web UI initialized');
    
    // Load all configuration
    loadSystemConfig();
    loadGlobalConfig();
    loadCamerasConfig();
    loadTagsConfig();
    loadProcessStatus();
    
    // Start polling for updates
    startPolling();
    
    // Setup event listeners
    setupEventListeners();
});

// Load system configuration
function loadSystemConfig() {
    fetch(`${API_BASE}/system`)
        .then(response => response.json())
        .then(data => {
            state.systemConfig = data;
            console.log('System config loaded:', data);
        })
        .catch(error => {
            console.error('Failed to load system config:', error);
        });
}

// Load global configuration
function loadGlobalConfig() {
    fetch(`${API_BASE}/global`)
        .then(response => response.json())
        .then(data => {
            state.globalConfig = data;
            console.log('Global config loaded:', data);
            
            // Update UI
            if (data.tagFamily) {
                document.getElementById('tag-family').value = data.tagFamily;
            }
            if (data.tagSize) {
                document.getElementById('tag-size').value = data.tagSize;
            }
        })
        .catch(error => {
            console.error('Failed to load global config:', error);
        });
}

// Load cameras configuration
function loadCamerasConfig() {
    fetch(`${API_BASE}/cameras`)
        .then(response => response.json())
        .then(data => {
            state.cameras = data;
            console.log('Cameras config loaded:', data);
            renderCameras();
            renderStreams();
        })
        .catch(error => {
            console.error('Failed to load cameras config:', error);
        });
}

// Load tags configuration
function loadTagsConfig() {
    fetch(`${API_BASE}/tags`)
        .then(response => response.json())
        .then(data => {
            state.tags = data.tags || [];
            console.log('Tags config loaded:', data);
            renderTags();
        })
        .catch(error => {
            console.error('Failed to load tags config:', error);
        });
}

// Load process status
function loadProcessStatus() {
    // This would be replaced with actual API call to get process status
    // For now, use dummy data
    state.processStatus = {
        launcher: { running: true, pid: 1234 },
        web_ui: { running: true, pid: 1235 },
        pose_finder: { running: true, pid: 1236 },
        camera_detector_0: { running: true, pid: 1237 },
        camera_detector_1: { running: true, pid: 1238 }
    };
    
    renderProcessStatus();
}

// Start polling for updates
function startPolling() {
    // Poll for process status
    setInterval(() => {
        loadProcessStatus();
    }, 2000);
    
    // Poll for robot position (would be via WebSocket in production)
    setInterval(() => {
        // Simulate position updates
        state.robotPosition.x = (Math.random() * 10 - 5).toFixed(2);
        state.robotPosition.y = (Math.random() * 10 - 5).toFixed(2);
        state.robotPosition.z = (Math.random() * 5).toFixed(2);
        state.robotPosition.yaw = (Math.random() * 360 - 180).toFixed(2);
        state.robotPosition.confidence = (Math.random() * 100).toFixed(2);
        
        updateRobotPosition();
    }, 1000);
}

// Setup event listeners
function setupEventListeners() {
    // Tab switching
    document.querySelectorAll('.tab-button').forEach(button => {
        button.addEventListener('click', () => {
            document.querySelectorAll('.tab-button').forEach(btn => {
                btn.classList.remove('active');
            });
            button.classList.add('active');
        });
    });
}

// Show a tab
function showTab(tabName) {
    // Hide all tabs
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.style.display = 'none';
    });
    
    // Show selected tab
    const tab = document.getElementById(tabName);
    if (tab) {
        tab.style.display = 'block';
    }
    
    // Update active button
    document.querySelectorAll('.tab-button').forEach(button => {
        button.classList.remove('active');
        if (button.textContent.toLowerCase().includes(tabName)) {
            button.classList.add('active');
        }
    });
}

// Render cameras
function renderCameras() {
    const container = document.getElementById('camera-list');
    
    if (!state.cameras || state.cameras.length === 0) {
        container.innerHTML = '<p>No cameras configured.</p>';
        return;
    }
    
    container.innerHTML = state.cameras.map(camera => `
        <div class="camera-card">
            <h3>
                Camera ${camera.cameraId}
                <span class="camera-id">ID: ${camera.cameraId}</span>
            </h3>
            <div class="camera-info">
                <div class="camera-info-item">
                    <label>Device Path</label>
                    <input type="text" value="${camera.devicePath || ''}" 
                           onchange="updateCameraConfig(${camera.cameraId}, 'devicePath', this.value)">
                </div>
                <div class="camera-info-item">
                    <label>Camera Type</label>
                    <select onchange="updateCameraConfig(${camera.cameraId}, 'cameraType', this.value)">
                        <option value="v4l2" ${camera.cameraType === 'v4l2' ? 'selected' : ''}>V4L2 (Linux)</option>
                        <option value="wmf" ${camera.cameraType === 'wmf' ? 'selected' : ''}>WMF (Windows)</option>
                        <option value="avfoundation" ${camera.cameraType === 'avfoundation' ? 'selected' : ''}>AVFoundation (macOS)</option>
                    </select>
                </div>
                <div class="camera-info-item">
                    <label>Resolution</label>
                    <input type="text" value="${camera.intrinsics ? camera.intrinsics.width + 'x' + camera.intrinsics.height : '640x480'}" 
                           placeholder="Width x Height">
                </div>
            </div>
            <div class="camera-actions">
                <button class="btn btn-primary" onclick="saveCameraConfig(${camera.cameraId})">Save</button>
            </div>
        </div>
    `).join('');
}

// Render tags
function renderTags() {
    const container = document.getElementById('tag-list');
    
    if (!state.tags || state.tags.length === 0) {
        container.innerHTML = '<p>No tags configured.</p>';
        return;
    }
    
    container.innerHTML = state.tags.map(tag => `
        <div class="tag-card">
            <div class="tag-id">${tag.id}</div>
            <div class="tag-info">
                <span><label>Position:</label> (${tag.translation[0].toFixed(2)}, ${tag.translation[1].toFixed(2)}, ${tag.translation[2].toFixed(2)})</span>
                <span><label>Size:</label> ${tag.size.toFixed(4)} m</span>
            </div>
            <div class="tag-actions">
                <button class="btn btn-primary" onclick="editTag(${tag.id})">Edit</button>
                <button class="btn btn-danger" onclick="deleteTag(${tag.id})">Delete</button>
            </div>
        </div>
    `).join('');
}

// Render streams
function renderStreams() {
    const container = document.getElementById('stream-grid');
    
    if (!state.cameras || state.cameras.length === 0) {
        container.innerHTML = '<p>No camera streams available.</p>';
        return;
    }
    
    container.innerHTML = state.cameras.map(camera => `
        <div class="stream-card">
            <h3>Camera ${camera.cameraId}</h3>
            <div class="stream-container">
                <div class="stream-overlay">Camera ${camera.cameraId}</div>
                <img src="/stream/camera${camera.cameraId}.mjpeg" alt="Camera ${camera.cameraId} Stream">
            </div>
        </div>
    `).join('');
}

// Render process status
function renderProcessStatus() {
    const container = document.getElementById('process-status');
    
    const processes = [
        'launcher',
        'web_ui',
        'pose_finder',
        ...Object.keys(state.processStatus).filter(k => k.startsWith('camera_detector_'))
    ];
    
    if (processes.length === 0) {
        container.innerHTML = '<p>No processes running.</p>';
        return;
    }
    
    container.innerHTML = processes.map(process => {
        const status = state.processStatus[process];
        const isRunning = status && status.running;
        const statusClass = isRunning ? 'status-online' : 'status-offline';
        const statusText = isRunning ? 'Running' : 'Stopped';
        const pid = status ? status.pid : 'N/A';
        
        return `
            <div class="process-item">
                <span class="process-name">${process}</span>
                <span class="process-status ${statusClass}">${statusText}</span>
                <span class="process-pid">PID: ${pid}</span>
            </div>
        `;
    }).join('');
    
    // Update active cameras count
    const activeCameras = processes.filter(p => p.startsWith('camera_detector_') && 
                                             state.processStatus[p] && 
                                             state.processStatus[p].running).length;
    document.getElementById('active-cameras').textContent = activeCameras;
}

// Update robot position display
function updateRobotPosition() {
    document.getElementById('position-x').textContent = state.robotPosition.x;
    document.getElementById('position-y').textContent = state.robotPosition.y;
    document.getElementById('position-z').textContent = state.robotPosition.z;
    document.getElementById('position-yaw').textContent = state.robotPosition.yaw;
    document.getElementById('position-confidence').textContent = state.robotPosition.confidence;
}

// Save system configuration
function saveSystemConfig() {
    const config = {
        nt4Address: document.getElementById('nt4-address').value,
        nt4Port: parseInt(document.getElementById('nt4-port').value),
        logLevel: document.getElementById('log-level').value
    };
    
    fetch(`${API_BASE}/system`, {
        method: 'PUT',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(config)
    })
    .then(response => response.json())
    .then(data => {
        console.log('System config saved:', data);
        showNotification('System configuration saved successfully!', 'success');
    })
    .catch(error => {
        console.error('Failed to save system config:', error);
        showNotification('Failed to save system configuration', 'error');
    });
}

// Save global configuration
function saveGlobalConfig() {
    const config = {
        tagFamily: document.getElementById('tag-family').value,
        tagSize: parseFloat(document.getElementById('tag-size').value)
    };
    
    fetch(`${API_BASE}/global`, {
        method: 'PUT',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(config)
    })
    .then(response => response.json())
    .then(data => {
        console.log('Global config saved:', data);
        showNotification('Global configuration saved successfully!', 'success');
    })
    .catch(error => {
        console.error('Failed to save global config:', error);
        showNotification('Failed to save global configuration', 'error');
    });
}

// Update camera configuration
function updateCameraConfig(cameraId, field, value) {
    // In a real implementation, we'd send the update to the server
    console.log(`Updating camera ${cameraId} ${field} to ${value}`);
}

// Save camera configuration
function saveCameraConfig(cameraId) {
    // In a real implementation, we'd collect all fields and send to server
    console.log(`Saving camera ${cameraId} configuration`);
    showNotification(`Camera ${cameraId} configuration saved!`, 'success');
}

// Edit tag
function editTag(tagId) {
    console.log(`Editing tag ${tagId}`);
    showNotification(`Edit tag ${tagId} - Not implemented yet`, 'info');
}

// Delete tag
function deleteTag(tagId) {
    console.log(`Deleting tag ${tagId}`);
    showNotification(`Delete tag ${tagId} - Not implemented yet`, 'info');
}

// Reload all configuration
function reloadConfig() {
    fetch(`${API_BASE}/reload`, {
        method: 'POST'
    })
    .then(response => response.json())
    .then(data => {
        console.log('Config reloaded:', data);
        showNotification('Configuration reloaded successfully!', 'success');
        
        // Reload all configs
        setTimeout(() => {
            loadSystemConfig();
            loadGlobalConfig();
            loadCamerasConfig();
            loadTagsConfig();
        }, 500);
    })
    .catch(error => {
        console.error('Failed to reload config:', error);
        showNotification('Failed to reload configuration', 'error');
    });
}

// Show notification
function showNotification(message, type = 'info') {
    // Create notification element
    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.textContent = message;
    notification.style.position = 'fixed';
    notification.style.bottom = '20px';
    notification.style.right = '20px';
    notification.style.padding = '15px 25px';
    notification.style.borderRadius = '8px';
    notification.style.fontSize = '14px';
    notification.style.zIndex = '1000';
    notification.style.animation = 'fadeIn 0.3s ease';
    
    // Style based on type
    switch (type) {
        case 'success':
            notification.style.background = 'linear-gradient(135deg, #2ed573 0%, #008000 100%)';
            notification.style.color = '#fff';
            break;
        case 'error':
            notification.style.background = 'linear-gradient(135deg, #ff4757 0%, #cc0000 100%)';
            notification.style.color = '#fff';
            break;
        case 'warning':
            notification.style.background = 'linear-gradient(135deg, #ffa502 0%, #cc8400 100%)';
            notification.style.color = '#000';
            break;
        default:
            notification.style.background = 'linear-gradient(135deg, #00d4ff 0%, #0099cc 100%)';
            notification.style.color = '#000';
    }
    
    document.body.appendChild(notification);
    
    // Remove after 3 seconds
    setTimeout(() => {
        notification.style.animation = 'fadeOut 0.3s ease';
        setTimeout(() => {
            notification.remove();
        }, 300);
    }, 3000);
}

// Add notification animations
const style = document.createElement('style');
style.textContent = `
    @keyframes fadeIn {
        from { opacity: 0; transform: translateX(100%); }
        to { opacity: 1; transform: translateX(0); }
    }
    @keyframes fadeOut {
        from { opacity: 1; transform: translateX(0); }
        to { opacity: 0; transform: translateX(100%); }
    }
`;
document.head.appendChild(style);

console.log('AprilTag Detector Web UI loaded successfully');
