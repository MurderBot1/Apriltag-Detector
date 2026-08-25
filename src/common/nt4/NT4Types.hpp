#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

/**
 * @brief NT4 (NetworkTables 4) type definitions
 */
namespace nt4 {
    
    // Message types
    enum class MessageType : uint8_t {
        PUBLISH = 0,
        SUBSCRIBE = 1,
        UNSUBSCRIBE = 2,
        DATA = 3,
        PING = 4,
        PONG = 5,
        SERVER_HELLO = 6,
        CLIENT_HELLO = 7,
        SERVER_GOODBYE = 8,
        CLIENT_GOODBYE = 9
    };
    
    // Data types
    enum class TypeId : uint8_t {
        BOOLEAN = 0,
        DOUBLE = 1,
        INT = 2,
        FLOAT = 3,
        STRING = 4,
        BOOLEAN_ARRAY = 10,
        DOUBLE_ARRAY = 11,
        INT_ARRAY = 12,
        FLOAT_ARRAY = 13,
        STRING_ARRAY = 14,
        RAW = 20
    };
    
    // Message header
    #pragma pack(push, 1)
    struct MessageHeader {
        uint32_t magic;           // NT4 magic number (0x4E543400)
        uint8_t version;         // Protocol version
        MessageType messageType; // Message type
        uint16_t topicId;         // Topic identifier
        uint32_t payloadSize;    // Payload size in bytes
        uint64_t timestamp;       // Message timestamp (microseconds)
        uint32_t sequenceNumber; // Sequence number
    };
    #pragma pack(pop)
    
    // Constants
    constexpr uint32_t MAGIC_NUMBER = 0x4E543400;  // "NT4\0"
    constexpr uint8_t PROTOCOL_VERSION = 1;
    constexpr uint16_t DEFAULT_PORT = 5810;
    
    // Robot position message (what we publish)
    struct RobotPositionMessage {
        double timestamp;       // Unix timestamp with microseconds
        double x;               // X position in meters
        double y;               // Y position in meters
        double z;               // Z position in meters
        double roll;            // Roll in radians
        double pitch;           // Pitch in radians
        double yaw;             // Yaw in radians
        double confidence;      // 0.0 to 1.0
        int32_t numCamerasUsed; // Number of cameras contributing
        
        // Serialize to bytes
        std::vector<uint8_t> serialize() const;
        
        // Deserialize from bytes
        static RobotPositionMessage deserialize(const uint8_t* data, size_t size);
    };
    
    // Detection count message (what we publish)
    struct DetectionCountMessage {
        int32_t cameraId;
        int32_t count;
        double timestamp;
        
        // Serialize to bytes
        std::vector<uint8_t> serialize() const;
        
        // Deserialize from bytes
        static DetectionCountMessage deserialize(const uint8_t* data, size_t size);
    };
    
    // Topic information
    struct TopicInfo {
        std::string name;
        TypeId type;
        uint16_t topicId;
        
        TopicInfo() : type(TypeId::RAW), topicId(0) {}
        TopicInfo(const std::string& n, TypeId t, uint16_t id)
            : name(n), type(t), topicId(id) {}
    };
    
    // Connection state
    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR
    };
    
} // namespace nt4
