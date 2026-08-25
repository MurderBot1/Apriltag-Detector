#include "NT4Client.hpp"
#include "NT4Types.hpp"
#include "../utils/TimeUtils.hpp"
#include "../logging/Logger.hpp"
#include "../utils/Platform.hpp"

#if PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#endif

NT4Client::NT4Client() 
    : m_serverPort(nt4::DEFAULT_PORT), m_socket(-1),
      m_connectionState(nt4::ConnectionState::DISCONNECTED),
      m_nextTopicId(1), m_sequenceNumber(0), m_running(false) {
    
    #if PLATFORM_WINDOWS
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif
}

NT4Client::~NT4Client() {
    stop();
    disconnect();
    
    #if PLATFORM_WINDOWS
        WSACleanup();
    #endif
}

bool NT4Client::connect(const std::string& address, int port) {
    m_serverAddress = address;
    m_serverPort = port;
    
    if (isConnected()) {
        disconnect();
    }
    
    if (doConnect()) {
        LOG_INFO_F("Connected to NT4 server at %s:%d", address.c_str(), port);
        return true;
    }
    
    LOG_ERROR_F("Failed to connect to NT4 server at %s:%d", address.c_str(), port);
    return false;
}

void NT4Client::disconnect() {
    doDisconnect();
    LOG_INFO("Disconnected from NT4 server");
}

bool NT4Client::isConnected() const {
    return m_connectionState == nt4::ConnectionState::CONNECTED;
}

bool NT4Client::publish(const std::string& topic, const std::vector<uint8_t>& data) {
    if (!isConnected()) {
        return false;
    }
    
    return sendMessage(nt4::MessageType::PUBLISH, topic, data);
}

bool NT4Client::publish(const std::string& topic, const std::vector<uint8_t>& data, nt4::TypeId type) {
    if (!isConnected()) {
        return false;
    }
    
    // For now, we don't use the type in the message
    // In a full implementation, we'd include it
    return sendMessage(nt4::MessageType::PUBLISH, topic, data);
}

bool NT4Client::subscribe(const std::string& topic, SubscribeCallback callback, nt4::TypeId type) {
    if (!isConnected()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if already subscribed
    if (m_subscriptions.find(topic) != m_subscriptions.end()) {
        LOG_WARN_F("Already subscribed to topic '%s'", topic.c_str());
        return false;
    }
    
    // Store subscription
    m_subscriptions[topic] = callback;
    m_subscriptionTypes[topic] = type;
    
    // Send subscribe message
    std::vector<uint8_t> typeData;
    typeData.push_back(static_cast<uint8_t>(type));
    
    return sendMessage(nt4::MessageType::SUBSCRIBE, topic, typeData);
}

void NT4Client::unsubscribe(const std::string& topic) {
    if (!isConnected()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_subscriptions.find(topic);
    if (it != m_subscriptions.end()) {
        m_subscriptions.erase(it);
        m_subscriptionTypes.erase(topic);
        
        // Send unsubscribe message
        sendMessage(nt4::MessageType::UNSUBSCRIBE, topic, {});
    }
}

bool NT4Client::publishCombinedRobotPosition(
    const std::vector<double>& position,
    const std::vector<double>& orientation,
    double confidence,
    int numCamerasUsed) {
    
    if (!isConnected()) {
        return false;
    }
    
    // Create robot position message
    nt4::RobotPositionMessage msg;
    msg.timestamp = TimeUtils::getCurrentTimestampUs() / 1000000.0;
    
    if (position.size() >= 3) {
        msg.x = position[0];
        msg.y = position[1];
        msg.z = position[2];
    }
    
    if (orientation.size() >= 3) {
        msg.roll = orientation[0];
        msg.pitch = orientation[1];
        msg.yaw = orientation[2];
    }
    
    msg.confidence = confidence;
    msg.numCamerasUsed = numCamerasUsed;
    
    // Serialize and publish
    std::vector<uint8_t> data = msg.serialize();
    return publish("/robot/position", data, nt4::TypeId::RAW);
}

bool NT4Client::publishDetectionCount(int cameraId, int count) {
    if (!isConnected()) {
        return false;
    }
    
    // Create detection count message
    nt4::DetectionCountMessage msg;
    msg.cameraId = cameraId;
    msg.count = count;
    msg.timestamp = TimeUtils::getCurrentTimestampUs() / 1000000.0;
    
    // Serialize and publish
    std::vector<uint8_t> data = msg.serialize();
    std::string topic = "/cameras/" + std::to_string(cameraId) + "/detection_count";
    return publish(topic, data, nt4::TypeId::RAW);
}

void NT4Client::setConnectionCallbacks(ConnectCallback onConnect, DisconnectCallback onDisconnect) {
    m_onConnect = onConnect;
    m_onDisconnect = onDisconnect;
}

void NT4Client::processEvents() {
    // This is handled by the process thread
}

void NT4Client::start() {
    if (m_running) return;
    
    m_running = true;
    m_processThread = std::thread(&NT4Client::processThread, this);
    LOG_INFO("Started NT4 client processing thread");
}

void NT4Client::stop() {
    if (!m_running) return;
    
    m_running = false;
    m_cv.notify_all();
    
    if (m_processThread.joinable()) {
        m_processThread.join();
    }
    
    LOG_INFO("Stopped NT4 client processing thread");
}

nt4::ConnectionState NT4Client::getConnectionState() const {
    return m_connectionState;
}

const std::string& NT4Client::getServerAddress() const {
    return m_serverAddress;
}

int NT4Client::getServerPort() const {
    return m_serverPort;
}

bool NT4Client::doConnect() {
    closeSocket();
    
    // Create socket
    m_socket = createSocket();
    if (m_socket < 0) {
        LOG_ERROR("Failed to create socket");
        return false;
    }
    
    // Set non-blocking
    #if PLATFORM_WINDOWS
        u_long mode = 1;
        ioctlsocket(m_socket, FIONBIO, &mode);
    #else
        fcntl(m_socket, F_SETFL, O_NONBLOCK);
    #endif
    
    // Resolve address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_serverPort);
    
    // Try to parse as IP address first
    if (inet_pton(AF_INET, m_serverAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        // Not an IP address, try to resolve hostname
        struct hostent* host = gethostbyname(m_serverAddress.c_str());
        if (!host) {
            LOG_ERROR_F("Failed to resolve hostname: %s", m_serverAddress.c_str());
            closeSocket();
            return false;
        }
        serverAddr.sin_addr = *reinterpret_cast<struct in_addr*>(host->h_addr);
    }
    
    // Connect
    m_connectionState = nt4::ConnectionState::CONNECTING;
    
    int result = ::connect(m_socket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr));
    
    if (result < 0) {
        #if PLATFORM_WINDOWS
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                LOG_ERROR_F("Connect failed: %d", error);
                closeSocket();
                m_connectionState = nt4::ConnectionState::ERROR;
                return false;
            }
        #else
            if (errno != EINPROGRESS) {
                LOG_ERROR_F("Connect failed: %s", strerror(errno));
                closeSocket();
                m_connectionState = nt4::ConnectionState::ERROR;
                return false;
            }
        #endif
    }
    
    // Wait for connection to complete
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(m_socket, &writeSet);
    
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 second timeout
    timeout.tv_usec = 0;
    
    result = select(m_socket + 1, nullptr, &writeSet, nullptr, &timeout);
    
    if (result <= 0) {
        LOG_ERROR("Connection timeout");
        closeSocket();
        m_connectionState = nt4::ConnectionState::ERROR;
        return false;
    }
    
    // Check if connection succeeded
    int error = 0;
    socklen_t len = sizeof(error);
    
    #if PLATFORM_WINDOWS
        if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len) < 0) {
            LOG_ERROR("getsockopt failed");
            closeSocket();
            m_connectionState = nt4::ConnectionState::ERROR;
            return false;
        }
    #else
        if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            LOG_ERROR("getsockopt failed");
            closeSocket();
            m_connectionState = nt4::ConnectionState::ERROR;
            return false;
        }
    #endif
    
    if (error != 0) {
        LOG_ERROR_F("Connection failed: %d", error);
        closeSocket();
        m_connectionState = nt4::ConnectionState::ERROR;
        return false;
    }
    
    // Set blocking mode for main operations
    #if PLATFORM_WINDOWS
        mode = 0;
        ioctlsocket(m_socket, FIONBIO, &mode);
    #else
        fcntl(m_socket, F_SETFL, 0);
    #endif
    
    // Send client hello
    std::vector<uint8_t> helloData;
    // Include client info if needed
    
    sendMessage(nt4::MessageType::CLIENT_HELLO, "", helloData);
    
    m_connectionState = nt4::ConnectionState::CONNECTED;
    
    // Notify callback
    if (m_onConnect) {
        m_onConnect(true);
    }
    
    return true;
}

void NT4Client::doDisconnect() {
    if (m_socket < 0) {
        return;
    }
    
    // Send goodbye
    try {
        sendMessage(nt4::MessageType::CLIENT_GOODBYE, "", {});
    } catch (...) {
        // Ignore errors
    }
    
    closeSocket();
    m_connectionState = nt4::ConnectionState::DISCONNECTED;
    
    // Clear subscriptions
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subscriptions.clear();
    m_subscriptionTypes.clear();
    m_topicIds.clear();
    m_idToTopic.clear();
    
    // Notify callback
    if (m_onDisconnect) {
        m_onDisconnect();
    }
}

bool NT4Client::sendMessage(nt4::MessageType type, uint16_t topicId, const std::vector<uint8_t>& data) {
    if (m_socket < 0) {
        return false;
    }
    
    nt4::MessageHeader header;
    header.magic = nt4::MAGIC_NUMBER;
    header.version = nt4::PROTOCOL_VERSION;
    header.messageType = type;
    header.topicId = topicId;
    header.payloadSize = static_cast<uint32_t>(data.size());
    header.timestamp = TimeUtils::getCurrentTimestampUs();
    header.sequenceNumber = m_sequenceNumber++;
    
    // Send header
    ssize_t sent = send(m_socket, reinterpret_cast<const char*>(&header), sizeof(header), 0);
    if (sent != static_cast<ssize_t>(sizeof(header))) {
        LOG_ERROR("Failed to send message header");
        return false;
    }
    
    // Send data
    if (!data.empty()) {
        sent = send(m_socket, reinterpret_cast<const char*>(data.data()), data.size(), 0);
        if (sent != static_cast<ssize_t>(data.size())) {
            LOG_ERROR("Failed to send message data");
            return false;
        }
    }
    
    return true;
}

bool NT4Client::sendMessage(nt4::MessageType type, const std::string& topic, const std::vector<uint8_t>& data) {
    uint16_t topicId = getTopicId(topic);
    return sendMessage(type, topicId, data);
}

uint16_t NT4Client::getTopicId(const std::string& topic) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_topicIds.find(topic);
    if (it != m_topicIds.end()) {
        return it->second;
    }
    
    // Assign new ID
    uint16_t newId = m_nextTopicId++;
    m_topicIds[topic] = newId;
    m_idToTopic[newId] = topic;
    
    return newId;
}

void NT4Client::receiveMessage(const nt4::MessageHeader& header, const std::vector<uint8_t>& data) {
    switch (header.messageType) {
        case nt4::MessageType::PUBLISH:
            handlePublish(header, data);
            break;
        case nt4::MessageType::DATA:
            handleData(header, data);
            break;
        case nt4::MessageType::SERVER_HELLO:
            handleServerHello(header, data);
            break;
        case nt4::MessageType::PING:
            handlePing(header);
            break;
        case nt4::MessageType::SERVER_GOODBYE:
            LOG_INFO("Server sent goodbye");
            disconnect();
            break;
        default:
            LOG_WARN_F("Received unknown message type: %d", static_cast<int>(header.messageType));
            break;
    }
}

void NT4Client::handlePublish(const nt4::MessageHeader& header, const std::vector<uint8_t>& data) {
    // Handle publish message
    // In NT4, publish is used to announce topics
}

void NT4Client::handleData(const nt4::MessageHeader& header, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_idToTopic.find(header.topicId);
    if (it == m_idToTopic.end()) {
        LOG_WARN_F("Received data for unknown topic ID: %d", header.topicId);
        return;
    }
    
    const std::string& topic = it->second;
    auto subIt = m_subscriptions.find(topic);
    if (subIt != m_subscriptions.end()) {
        subIt->second(topic, data);
    }
}

void NT4Client::handleServerHello(const nt4::MessageHeader& header, const std::vector<uint8_t>& data) {
    LOG_INFO("Received server hello");
    // Could parse server info from data
}

void NT4Client::handlePing(const nt4::MessageHeader& header) {
    // Respond with pong
    nt4::MessageHeader pongHeader;
    pongHeader.magic = nt4::MAGIC_NUMBER;
    pongHeader.version = nt4::PROTOCOL_VERSION;
    pongHeader.messageType = nt4::MessageType::PONG;
    pongHeader.topicId = 0;
    pongHeader.payloadSize = 0;
    pongHeader.timestamp = TimeUtils::getCurrentTimestampUs();
    pongHeader.sequenceNumber = m_sequenceNumber++;
    
    send(m_socket, reinterpret_cast<const char*>(&pongHeader), sizeof(pongHeader), 0);
}

bool NT4Client::receiveMessage(nt4::MessageHeader& header, std::vector<uint8_t>& data) {
    if (m_socket < 0) {
        return false;
    }
    
    // Receive header
    ssize_t received = recv(m_socket, reinterpret_cast<char*>(&header), sizeof(header), 0);
    if (received != static_cast<ssize_t>(sizeof(header))) {
        if (received <= 0) {
            // Connection closed or error
            LOG_INFO("Connection closed by server");
            disconnect();
        }
        return false;
    }
    
    // Check magic number
    if (header.magic != nt4::MAGIC_NUMBER) {
        LOG_ERROR_F("Invalid magic number: 0x%08X (expected 0x%08X)", 
                    header.magic, nt4::MAGIC_NUMBER);
        return false;
    }
    
    // Check version
    if (header.version != nt4::PROTOCOL_VERSION) {
        LOG_WARN_F("Protocol version mismatch: %d (expected %d)", 
                   header.version, nt4::PROTOCOL_VERSION);
    }
    
    // Receive data
    if (header.payloadSize > 0) {
        data.resize(header.payloadSize);
        received = recv(m_socket, reinterpret_cast<char*>(data.data()), header.payloadSize, 0);
        if (received != static_cast<ssize_t>(header.payloadSize)) {
            LOG_ERROR("Failed to receive message data");
            return false;
        }
    } else {
        data.clear();
    }
    
    return true;
}

void NT4Client::processThread() {
    while (m_running) {
        nt4::MessageHeader header;
        std::vector<uint8_t> data;
        
        if (receiveMessage(header, data)) {
            receiveMessage(header, data);
        }
        
        // Sleep a bit
        TimeUtils::sleepMs(10);
    }
}

int NT4Client::createSocket() {
    #if PLATFORM_WINDOWS
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            return -1;
        }
        return static_cast<int>(sock);
    #else
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return -1;
        }
        return sock;
    #endif
}

void NT4Client::closeSocket() {
    if (m_socket < 0) {
        return;
    }
    
    #if PLATFORM_WINDOWS
        closesocket(static_cast<SOCKET>(m_socket));
    #else
        close(m_socket);
    #endif
    
    m_socket = -1;
}
