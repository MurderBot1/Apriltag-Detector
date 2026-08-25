#pragma once

#include "NT4Client.hpp"
#include <string>
#include <vector>
#include <memory>

/**
 * @brief Convenience class for NT4 publishing
 */
class NT4Publisher {
public:
    /**
     * @brief Constructor
     * @param client NT4Client instance
     */
    explicit NT4Publisher(NT4Client* client);
    
    /**
     * @brief Destructor
     */
    ~NT4Publisher();
    
    /**
     * @brief Set the NT4 client
     * @param client NT4Client instance
     */
    void setClient(NT4Client* client);
    
    /**
     * @brief Publish combined robot position
     * @param position Position vector [x, y, z]
     * @param orientation Orientation vector [roll, pitch, yaw]
     * @param confidence Confidence value (0.0 to 1.0)
     * @param numCamerasUsed Number of cameras contributing
     * @return true if published successfully
     */
    bool publishCombinedRobotPosition(
        const std::vector<double>& position,
        const std::vector<double>& orientation,
        double confidence = 1.0,
        int numCamerasUsed = 1
    );
    
    /**
     * @brief Publish detection count for a camera
     * @param cameraId Camera ID
     * @param count Number of detections
     * @return true if published successfully
     */
    bool publishDetectionCount(int cameraId, int count);
    
    /**
     * @brief Publish raw data to a topic
     * @param topic Topic name
     * @param data Data to publish
     * @return true if published successfully
     */
    bool publish(const std::string& topic, const std::vector<uint8_t>& data);
    
    /**
     * @brief Check if connected
     * @return true if connected
     */
    bool isConnected() const;
    
private:
    NT4Client* m_client;
};
