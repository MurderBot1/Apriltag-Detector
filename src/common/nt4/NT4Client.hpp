#pragma once

#include "NT4Types.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <map>

/**
 * @brief NT4 Client for publishing and subscribing to NetworkTables topics
 * 
 * Implements the custom NT4 protocol for robot communication.
 * Per requirements, only publishes:
 * - Combined robot position from all cameras
 * - Detection count per camera
 */
class NT4Client {
public:
    using SubscribeCallback = std::function<void(const std::string&, const std::vector<uint8_t>&)>;
    using ConnectCallback = std::function<void(bool)>;
    using DisconnectCallback = std::function<void()>;
    
    /**
     * @brief Constructor
     */
    NT4Client();
    
    /**
     * @brief Destructor
     */
    ~NT4Client();
    
    /**
     * @brief Connect to NT4 server
     * @param address Server address (IP or hostname)
     * @param port Server port (default: 5810)
     * @return true if connected successfully
     */
    bool connect(const std::string& address, int port = nt4::DEFAULT_PORT);
    
    /**
     * @brief Disconnect from NT4 server
     */
    void disconnect();
    
    /**
     * @brief Check if connected
     * @return true if connected
     */
    bool isConnected() const;
    
    /**
     * @brief Publish data to a topic
     * @param topic Topic name
     * @param data Data to publish
     * @return true if published successfully
     */
    bool publish(const std::string& topic, const std::vector<uint8_t>& data);
    
    /**
     * @brief Publish data to a topic with type
     * @param topic Topic name
     * @param data Data to publish
     * @param type Data type
     * @return true if published successfully
     */
    bool publish(const std::string& topic, const std::vector<uint8_t>& data, nt4::TypeId type);
    
    /**
     * @brief Subscribe to a topic
     * @param topic Topic name
     * @param callback Callback for received data
     * @param type Expected data type (optional)
     * @return true if subscribed successfully
     */
    bool subscribe(const std::string& topic, SubscribeCallback callback, nt4::TypeId type = nt4::TypeId::RAW);
    
    /**
     * @brief Unsubscribe from a topic
     * @param topic Topic name
     */
    void unsubscribe(const std::string& topic);
    
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
     * @brief Set connection callbacks
     * @param onConnect Callback when connected
     * @param onDisconnect Callback when disconnected
     */
    void setConnectionCallbacks(ConnectCallback onConnect, DisconnectCallback onDisconnect);
    
    /**
     * @brief Process network events (call from main thread)
     */
    void processEvents();
    
    /**
     * @brief Start background processing thread
     */
    void start();
    
    /**
     * @brief Stop background processing thread
     */
    void stop();
    
    /**
     * @brief Get connection state
     * @return Connection state
     */
    nt4::ConnectionState getConnectionState() const;
    
    /**
     * @brief Get server address
     * @return Server address
     */
    const std::string& getServerAddress() const;
    
    /**
     * @brief Get server port
     * @return Server port
     */
    int getServerPort() const;
    
private:
    std::string m_serverAddress;
    int m_serverPort;
    int m_socket;
    std::atomic<nt4::ConnectionState> m_connectionState;
    
    std::map<std::string, SubscribeCallback> m_subscriptions;
    std::map<std::string, nt4::TypeId> m_subscriptionTypes;
    
    ConnectCallback m_onConnect;
    DisconnectCallback m_onDisconnect;
    
    std::thread m_processThread;
    std::atomic<bool> m_running;
    
    std::mutex m_mutex;
    std::condition_variable m_cv;
    
    uint16_t m_nextTopicId;
    std::map<std::string, uint16_t> m_topicIds;
    std::map<uint16_t, std::string> m_idToTopic;
    uint32_t m_sequenceNumber;
    
    /**
     * @brief Process thread function
     */
    void processThread();
    
    /**
     * @brief Connect to server
     * @return true if connected
     */
    bool doConnect();
    
    /**
     * @brief Disconnect from server
     */
    void doDisconnect();
    
    /**
     * @brief Send a message
     * @param type Message type
     * @param topicId Topic ID
     * @param data Data to send
     * @return true if sent successfully
     */
    bool sendMessage(nt4::MessageType type, uint16_t topicId, const std::vector<uint8_t>& data);
    
    /**
     * @brief Send a message with topic name
     * @param type Message type
     * @param topic Topic name
     * @param data Data to send
     * @return true if sent successfully
     */
    bool sendMessage(nt4::MessageType type, const std::string& topic, const std::vector<uint8_t>& data);
    
    /**
     * @brief Get or create topic ID
     * @param topic Topic name
     * @return Topic ID
     */
    uint16_t getTopicId(const std::string& topic);
    
    /**
     * @brief Receive a message
     * @param header Message header
     * @param data Message data
     */
    void receiveMessage(const nt4::MessageHeader& header, const std::vector<uint8_t>& data);
    
    /**
     * @brief Handle publish message
     * @param header Message header
     * @param data Message data
     */
    void handlePublish(const nt4::MessageHeader& header, const std::vector<uint8_t>& data);
    
    /**
     * @brief Handle data message
     * @param header Message header
     * @param data Message data
     */
    void handleData(const nt4::MessageHeader& header, const std::vector<uint8_t>& data);
    
    /**
     * @brief Handle server hello
     * @param header Message header
     * @param data Message data
     */
    void handleServerHello(const nt4::MessageHeader& header, const std::vector<uint8_t>& data);
    
    /**
     * @brief Handle ping
     * @param header Message header
     */
    void handlePing(const nt4::MessageHeader& header);
    
    /**
     * @brief Read a message from socket
     * @param header Output header
     * @param data Output data
     * @return true if message received
     */
    bool receiveMessage(nt4::MessageHeader& header, std::vector<uint8_t>& data);
    
    /**
     * @brief Create socket
     * @return Socket descriptor or -1
     */
    int createSocket();
    
    /**
     * @brief Close socket
     */
    void closeSocket();
};
