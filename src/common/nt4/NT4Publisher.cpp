#include "NT4Publisher.hpp"

NT4Publisher::NT4Publisher(NT4Client* client) : m_client(client) {
}

NT4Publisher::~NT4Publisher() {
}

void NT4Publisher::setClient(NT4Client* client) {
    m_client = client;
}

bool NT4Publisher::publishCombinedRobotPosition(
    const std::vector<double>& position,
    const std::vector<double>& orientation,
    double confidence,
    int numCamerasUsed) {
    
    if (!m_client) {
        return false;
    }
    
    return m_client->publishCombinedRobotPosition(position, orientation, confidence, numCamerasUsed);
}

bool NT4Publisher::publishDetectionCount(int cameraId, int count) {
    if (!m_client) {
        return false;
    }
    
    return m_client->publishDetectionCount(cameraId, count);
}

bool NT4Publisher::publish(const std::string& topic, const std::vector<uint8_t>& data) {
    if (!m_client) {
        return false;
    }
    
    return m_client->publish(topic, data);
}

bool NT4Publisher::isConnected() const {
    if (!m_client) {
        return false;
    }
    
    return m_client->isConnected();
}
