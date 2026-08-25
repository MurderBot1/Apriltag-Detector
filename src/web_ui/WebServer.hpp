#pragma once

#include "../common/utils/Platform.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

/**
 * @brief HTTP request structure
 */
struct HttpRequest {
    std::string method;        // GET, POST, PUT, DELETE
    std::string path;
    std::string queryString;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;
    std::string remoteAddress;
    int remotePort;
};

/**
 * @brief HTTP response structure
 */
struct HttpResponse {
    int statusCode;            // 200, 404, 500, etc.
    std::string statusText;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;
    
    HttpResponse() : statusCode(200), statusText("OK") {}
};

/**
 * @brief WebSocket message type
 */
enum class WebSocketMessageType {
    TEXT,
    BINARY,
    CLOSE
};

/**
 * @brief WebSocket message structure
 */
struct WebSocketMessage {
    WebSocketMessageType type;
    std::vector<uint8_t> data;
};

/**
 * @brief WebSocket connection handler
 */
class WebSocketConnection {
public:
    virtual ~WebSocketConnection() {}
    
    virtual void onMessage(const WebSocketMessage& message) = 0;
    virtual void onClose() = 0;
};

/**
 * @brief Web server implementation
 * 
 * Uses platform-optimal APIs:
 * - Linux: epoll
 * - Windows: IOCP
 * - macOS: kqueue
 */
class WebServer {
public:
    using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;
    using WebSocketHandler = std::function<void(const std::string&, std::shared_ptr<WebSocketConnection>)>;
    
    /**
     * @brief Constructor
     * @param port Port to listen on
     * @param numThreads Number of worker threads
     */
    WebServer(int port = 8080, int numThreads = 4);
    
    /**
     * @brief Destructor
     */
    ~WebServer();
    
    /**
     * @brief Start the server
     * @return true if started successfully
     */
    bool start();
    
    /**
     * @brief Stop the server
     */
    void stop();
    
    /**
     * @brief Check if server is running
     * @return true if running
     */
    bool isRunning() const;
    
    /**
     * @brief Register a request handler for a path
     * @param path Path to handle (e.g., "/api/cameras")
     * @param handler Handler function
     * @param methods Allowed methods (empty = all)
     */
    void registerHandler(const std::string& path, RequestHandler handler,
                         const std::vector<std::string>& methods = {});
    
    /**
     * @brief Register a static file directory
     * @param path URL path (e.g., "/static")
     * @param directory Filesystem directory
     */
    void registerStaticDirectory(const std::string& path, const std::string& directory);
    
    /**
     * @brief Register a WebSocket handler
     * @param path WebSocket path (e.g., "/ws")
     * @param handler Handler function
     */
    void registerWebSocketHandler(const std::string& path, WebSocketHandler handler);
    
    /**
     * @brief Set the default handler (for 404)
     * @param handler Handler function
     */
    void setDefaultHandler(RequestHandler handler);
    
    /**
     * @brief Set the error handler
     * @param handler Handler function
     */
    void setErrorHandler(RequestHandler handler);
    
    /**
     * @brief Set server options
     * @param maxConnections Maximum simultaneous connections
     * @param timeoutSeconds Connection timeout in seconds
     */
    void setOptions(int maxConnections = 100, int timeoutSeconds = 30);
    
    /**
     * @brief Get the port
     * @return Port number
     */
    int getPort() const;
    
    /**
     * @brief Broadcast a WebSocket message to all connections
     * @param path WebSocket path
     * @param message Message to send
     */
    void broadcastWebSocket(const std::string& path, const std::string& message);
    
    /**
     * @brief Broadcast a WebSocket message to all connections
     * @param path WebSocket path
     * @param message Message to send
     */
    void broadcastWebSocket(const std::string& path, const std::vector<uint8_t>& message);
    
private:
    int m_port;
    int m_numThreads;
    int m_maxConnections;
    int m_timeoutSeconds;
    
    std::atomic<bool> m_running;
    
    #if PLATFORM_WINDOWS
        SOCKET m_listenSocket;
        HANDLE m_iocp;
        std::vector<std::thread> m_workerThreads;
    #else
        int m_listenSocket;
        #if PLATFORM_LINUX
            int m_epollFd;
        #else
            int m_kqueueFd;
        #endif
        std::vector<std::thread> m_workerThreads;
    #endif
    
    std::map<std::string, RequestHandler> m_handlers;
    std::map<std::string, std::string> m_staticDirectories;
    std::map<std::string, WebSocketHandler> m_webSocketHandlers;
    RequestHandler m_defaultHandler;
    RequestHandler m_errorHandler;
    
    mutable std::mutex m_mutex;
    
    /**
     * @brief Create the listen socket
     * @return true if successful
     */
    bool createListenSocket();
    
    /**
     * @brief Initialize platform-specific event system
     * @return true if successful
     */
    bool initEventSystem();
    
    /**
     * @brief Cleanup platform-specific event system
     */
    void cleanupEventSystem();
    
    /**
     * @brief Accept connections
     */
    void acceptConnections();
    
    /**
     * @brief Handle a client connection
     * @param clientSocket Client socket
     */
    void handleClient(int clientSocket);
    
    /**
     * @brief Process HTTP request
     * @param clientSocket Client socket
     * @param request Request to process
     */
    void processRequest(int clientSocket, const HttpRequest& request);
    
    /**
     * @brief Serve static file
     * @param path File path
     * @return HttpResponse with file content
     */
    HttpResponse serveStaticFile(const std::string& path);
    
    /**
     * @brief Handle WebSocket upgrade
     * @param clientSocket Client socket
     * @param request Request
     */
    void handleWebSocketUpgrade(int clientSocket, const HttpRequest& request);
    
    /**
     * @brief Parse HTTP request
     * @param data Request data
     * @return HttpRequest
     */
    HttpRequest parseRequest(const std::vector<uint8_t>& data);
    
    /**
     * @brief Format HTTP response
     * @param response Response to format
     * @return Formatted response as bytes
     */
    std::vector<uint8_t> formatResponse(const HttpResponse& response);
    
    /**
     * @brief Send data to client
     * @param clientSocket Client socket
     * @param data Data to send
     * @return true if sent successfully
     */
    bool sendData(int clientSocket, const std::vector<uint8_t>& data);
    
    /**
     * @brief Close client connection
     * @param clientSocket Client socket
     */
    void closeClient(int clientSocket);
    
    /**
     * @brief Worker thread function
     */
    void workerThread();
};
