#include "NT4Types.hpp"
#include <cstring>
#include <algorithm>

namespace nt4 {
    
    std::vector<uint8_t> RobotPositionMessage::serialize() const {
        std::vector<uint8_t> data;
        data.resize(sizeof(RobotPositionMessage));
        
        uint8_t* ptr = data.data();
        
        // Copy fields
        std::memcpy(ptr, &timestamp, sizeof(timestamp));
        ptr += sizeof(timestamp);
        
        std::memcpy(ptr, &x, sizeof(x));
        ptr += sizeof(x);
        
        std::memcpy(ptr, &y, sizeof(y));
        ptr += sizeof(y);
        
        std::memcpy(ptr, &z, sizeof(z));
        ptr += sizeof(z);
        
        std::memcpy(ptr, &roll, sizeof(roll));
        ptr += sizeof(roll);
        
        std::memcpy(ptr, &pitch, sizeof(pitch));
        ptr += sizeof(pitch);
        
        std::memcpy(ptr, &yaw, sizeof(yaw));
        ptr += sizeof(yaw);
        
        std::memcpy(ptr, &confidence, sizeof(confidence));
        ptr += sizeof(confidence);
        
        std::memcpy(ptr, &numCamerasUsed, sizeof(numCamerasUsed));
        
        return data;
    }
    
    RobotPositionMessage RobotPositionMessage::deserialize(const uint8_t* data, size_t size) {
        RobotPositionMessage msg;
        
        if (size < sizeof(RobotPositionMessage)) {
            return msg;  // Return default
        }
        
        const uint8_t* ptr = data;
        
        std::memcpy(&msg.timestamp, ptr, sizeof(msg.timestamp));
        ptr += sizeof(msg.timestamp);
        
        std::memcpy(&msg.x, ptr, sizeof(msg.x));
        ptr += sizeof(msg.x);
        
        std::memcpy(&msg.y, ptr, sizeof(msg.y));
        ptr += sizeof(msg.y);
        
        std::memcpy(&msg.z, ptr, sizeof(msg.z));
        ptr += sizeof(msg.z);
        
        std::memcpy(&msg.roll, ptr, sizeof(msg.roll));
        ptr += sizeof(msg.roll);
        
        std::memcpy(&msg.pitch, ptr, sizeof(msg.pitch));
        ptr += sizeof(msg.pitch);
        
        std::memcpy(&msg.yaw, ptr, sizeof(msg.yaw));
        ptr += sizeof(msg.yaw);
        
        std::memcpy(&msg.confidence, ptr, sizeof(msg.confidence));
        ptr += sizeof(msg.confidence);
        
        std::memcpy(&msg.numCamerasUsed, ptr, sizeof(msg.numCamerasUsed));
        
        return msg;
    }
    
    std::vector<uint8_t> DetectionCountMessage::serialize() const {
        std::vector<uint8_t> data;
        data.resize(sizeof(DetectionCountMessage));
        
        uint8_t* ptr = data.data();
        
        std::memcpy(ptr, &cameraId, sizeof(cameraId));
        ptr += sizeof(cameraId);
        
        std::memcpy(ptr, &count, sizeof(count));
        ptr += sizeof(count);
        
        std::memcpy(ptr, &timestamp, sizeof(timestamp));
        
        return data;
    }
    
    DetectionCountMessage DetectionCountMessage::deserialize(const uint8_t* data, size_t size) {
        DetectionCountMessage msg;
        
        if (size < sizeof(DetectionCountMessage)) {
            return msg;  // Return default
        }
        
        const uint8_t* ptr = data;
        
        std::memcpy(&msg.cameraId, ptr, sizeof(msg.cameraId));
        ptr += sizeof(msg.cameraId);
        
        std::memcpy(&msg.count, ptr, sizeof(msg.count));
        ptr += sizeof(msg.count);
        
        std::memcpy(&msg.timestamp, ptr, sizeof(msg.timestamp));
        
        return msg;
    }
    
} // namespace nt4
