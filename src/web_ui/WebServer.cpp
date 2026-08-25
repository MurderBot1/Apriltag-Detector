#include "WebServer.hpp"
#include "../common/logging/Logger.hpp"
#include "../common/utils/FileSystem.hpp"
#include "../common/utils/TimeUtils.hpp"
#include "../common/utils/Platform.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

// Platform-specific includes
#if PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#endif

WebServer::WebServer(int port, int numThreads)
    : m_port(port), m_numThreads(numThreads),
      m_maxConnections(100), m_timeoutSeconds(30),
      m_running(false) {
    
    #if PLATFORM_WINDOWS
        m_listenSocket = INVALID_SOCKET;
        m_iocp = INVALID_HANDLE_VALUE;
    #else
        m_listenSocket = -1;
        #if PLATFORM_LINUX
            m_epollFd = -1;
        #else
            m_kqueueFd = -1;
        #endif
    #endif
    
    // Set default error handler
    m_errorHandler = [](const HttpRequest&) {
        HttpResponse response;
        response.statusCode = 500;
        response.statusText = "Internal Server Error";
        response.body = std::vector<uint8_t>("500 Internal Server Error".begin(), 
                                              "500 Internal Server Error".end());
        response.headers["Content-Type"] = "text/plain";
        return response;
    };
}

WebServer::~WebServer() {
    stop();
}

bool WebServer::start() {
    if (m_running) {
        return false;
    }
    
    // Initialize platform-specific networking
    #if PLATFORM_WINDOWS
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            LOG_ERROR("WSAStartup failed");
            return false;
        }
    #endif
    
    // Create listen socket
    if (!createListenSocket()) {
        LOG_ERROR("Failed to create listen socket");
        return false;
    }
    
    // Initialize event system
    if (!initEventSystem()) {
        LOG_ERROR("Failed to initialize event system");
        return false;
    }
    
    // Start worker threads
    for (int i = 0; i < m_numThreads; ++i) {
        m_workerThreads.emplace_back(&WebServer::workerThread, this);
    }
    
    m_running = true;
    LOG_INFO_F("Web server started on port %d with %d threads", m_port, m_numThreads);
    
    return true;
}

void WebServer::stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    // Close listen socket
    #if PLATFORM_WINDOWS
        if (m_listenSocket != INVALID_SOCKET) {
            closesocket(m_listenSocket);
            m_listenSocket = INVALID_SOCKET;
        }
    #else
        if (m_listenSocket >= 0) {
            close(m_listenSocket);
            m_listenSocket = -1;
        }
    #endif
    
    // Join worker threads
    for (auto& thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_workerThreads.clear();
    
    // Cleanup event system
    cleanupEventSystem();
    
    #if PLATFORM_WINDOWS
        WSACleanup();
    #endif
    
    LOG_INFO("Web server stopped");
}

bool WebServer::isRunning() const {
    return m_running;
}

void WebServer::registerHandler(const std::string& path, RequestHandler handler,
                                const std::vector<std::string>& methods) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers[path] = handler;
}

void WebServer::registerStaticDirectory(const std::string& path, const std::string& directory) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_staticDirectories[path] = directory;
}

void WebServer::registerWebSocketHandler(const std::string& path, WebSocketHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_webSocketHandlers[path] = handler;
}

void WebServer::setDefaultHandler(RequestHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_defaultHandler = handler;
}

void WebServer::setErrorHandler(RequestHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errorHandler = handler;
}

void WebServer::setOptions(int maxConnections, int timeoutSeconds) {
    m_maxConnections = maxConnections;
    m_timeoutSeconds = timeoutSeconds;
}

int WebServer::getPort() const {
    return m_port;
}

void WebServer::broadcastWebSocket(const std::string& path, const std::string& message) {
    std::vector<uint8_t> data(message.begin(), message.end());
    broadcastWebSocket(path, data);
}

void WebServer::broadcastWebSocket(const std::string& path, const std::vector<uint8_t>& message) {
    // In a full implementation, we'd track WebSocket connections
    // and broadcast to all connections on the specified path
    LOG_INFO_F("Broadcasting WebSocket message to %s: %zu bytes", path.c_str(), message.size());
}

bool WebServer::createListenSocket() {
    #if PLATFORM_WINDOWS
        m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listenSocket == INVALID_SOCKET) {
            LOG_ERROR_F("socket() failed: %d", WSAGetLastError());
            return false;
        }
    #else
        m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_listenSocket < 0) {
            LOG_ERROR_F("socket() failed: %s", strerror(errno));
            return false;
        }
    #endif
    
    // Set socket options
    int opt = 1;
    #if PLATFORM_WINDOWS
        if (setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, 
                       reinterpret_cast<const char*>(&opt), sizeof(opt)) == SOCKET_ERROR) {
            LOG_ERROR_F("setsockopt() failed: %d", WSAGetLastError());
            return false;
        }
    #else
        if (setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            LOG_ERROR_F("setsockopt() failed: %s", strerror(errno));
            return false;
        }
    #endif
    
    // Bind socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    #if PLATFORM_WINDOWS
        if (bind(m_listenSocket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            LOG_ERROR_F("bind() failed: %d", WSAGetLastError());
            return false;
        }
    #else
        if (bind(m_listenSocket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            LOG_ERROR_F("bind() failed: %s", strerror(errno));
            return false;
        }
    #endif
    
    // Listen
    #if PLATFORM_WINDOWS
        if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            LOG_ERROR_F("listen() failed: %d", WSAGetLastError());
            return false;
        }
    #else
        if (listen(m_listenSocket, m_maxConnections) < 0) {
            LOG_ERROR_F("listen() failed: %s", strerror(errno));
            return false;
        }
    #endif
    
    // Set non-blocking
    #if PLATFORM_WINDOWS
        u_long mode = 1;
        if (ioctlsocket(m_listenSocket, FIONBIO, &mode) != 0) {
            LOG_ERROR_F("ioctlsocket() failed: %d", WSAGetLastError());
            return false;
        }
    #else
        if (fcntl(m_listenSocket, F_SETFL, O_NONBLOCK) < 0) {
            LOG_ERROR_F("fcntl() failed: %s", strerror(errno));
            return false;
        }
    #endif
    
    LOG_INFO_F("Listen socket created on port %d", m_port);
    return true;
}

bool WebServer::initEventSystem() {
    #if PLATFORM_WINDOWS
        // Create IOCP
        m_iocp = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE, nullptr, 0, m_numThreads);
        if (m_iocp == INVALID_HANDLE_VALUE) {
            LOG_ERROR_F("CreateIoCompletionPort() failed: %d", GetLastError());
            return false;
        }
        
        // Associate listen socket with IOCP
        if (CreateIoCompletionPort(
            reinterpret_cast<HANDLE>(m_listenSocket), 
            m_iocp, 
            0, 
            0) == INVALID_HANDLE_VALUE) {
            LOG_ERROR_F("CreateIoCompletionPort() for socket failed: %d", GetLastError());
            CloseHandle(m_iocp);
            m_iocp = INVALID_HANDLE_VALUE;
            return false;
        }
    #else
        #if PLATFORM_LINUX
            m_epollFd = epoll_create1(0);
            if (m_epollFd < 0) {
                LOG_ERROR_F("epoll_create1() failed: %s", strerror(errno));
                return false;
            }
            
            // Add listen socket to epoll
            struct epoll_event event;
            event.events = EPOLLIN | EPOLLET;
            event.data.fd = m_listenSocket;
            
            if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_listenSocket, &event) < 0) {
                LOG_ERROR_F("epoll_ctl() failed: %s", strerror(errno));
                close(m_epollFd);
                m_epollFd = -1;
                return false;
            }
        #else
            m_kqueueFd = kqueue();
            if (m_kqueueFd < 0) {
                LOG_ERROR_F("kqueue() failed: %s", strerror(errno));
                return false;
            }
            
            // Add listen socket to kqueue
            struct kevent event;
            EV_SET(&event, m_listenSocket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
            
            if (kevent(m_kqueueFd, &event, 1, nullptr, 0, nullptr) < 0) {
                LOG_ERROR_F("kevent() failed: %s", strerror(errno));
                close(m_kqueueFd);
                m_kqueueFd = -1;
                return false;
            }
        #endif
    #endif
    
    return true;
}

void WebServer::cleanupEventSystem() {
    #if PLATFORM_WINDOWS
        if (m_iocp != INVALID_HANDLE_VALUE) {
            CloseHandle(m_iocp);
            m_iocp = INVALID_HANDLE_VALUE;
        }
    #else
        #if PLATFORM_LINUX
            if (m_epollFd >= 0) {
                close(m_epollFd);
                m_epollFd = -1;
            }
        #else
            if (m_kqueueFd >= 0) {
                close(m_kqueueFd);
                m_kqueueFd = -1;
            }
        #endif
    #endif
}

void WebServer::acceptConnections() {
    while (m_running) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        #if PLATFORM_WINDOWS
            SOCKET clientSocket = accept(m_listenSocket, 
                                         reinterpret_cast<struct sockaddr*>(&clientAddr), 
                                         &clientAddrLen);
        #else
            int clientSocket = accept(m_listenSocket, 
                                      reinterpret_cast<struct sockaddr*>(&clientAddr), 
                                      &clientAddrLen);
        #endif
        
        if (clientSocket < 0) {
            #if PLATFORM_WINDOWS
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) {
                    LOG_ERROR_F("accept() failed: %d", error);
                }
            #else
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    LOG_ERROR_F("accept() failed: %s", strerror(errno));
                }
            #endif
            TimeUtils::sleepMs(10);
            continue;
        }
        
        // Set non-blocking
        #if PLATFORM_WINDOWS
            u_long mode = 1;
            ioctlsocket(clientSocket, FIONBIO, &mode);
        #else
            fcntl(clientSocket, F_SETFL, O_NONBLOCK);
        #endif
        
        // Handle client in a new thread (simplified for now)
        // In a production implementation, we'd use the event system
        std::thread([this, clientSocket, clientAddr]() {
            handleClient(clientSocket);
        }).detach();
    }
}

void WebServer::handleClient(int clientSocket) {
    char buffer[4096];
    std::vector<uint8_t> requestData;
    
    while (m_running) {
        #if PLATFORM_WINDOWS
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        #else
            ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        #endif
        
        if (bytesRead <= 0) {
            break;
        }
        
        requestData.insert(requestData.end(), buffer, buffer + bytesRead);
        
        // Check for end of headers
        if (requestData.size() >= 4 && 
            requestData[requestData.size() - 4] == '\r' &&
            requestData[requestData.size() - 3] == '\n' &&
            requestData[requestData.size() - 2] == '\r' &&
            requestData[requestData.size() - 1] == '\n') {
            
            // Parse request
            HttpRequest request = parseRequest(requestData);
            
            // Process request
            processRequest(clientSocket, request);
            break;
        }
        
        if (requestData.size() > 65536) {
            // Request too large
            break;
        }
    }
    
    closeClient(clientSocket);
}

void WebServer::processRequest(int clientSocket, const HttpRequest& request) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    LOG_DEBUG_F("Processing %s %s", request.method.c_str(), request.path.c_str());
    
    // Check for WebSocket upgrade
    auto it = request.headers.find("Upgrade");
    if (it != request.headers.end() && it->second == "websocket") {
        handleWebSocketUpgrade(clientSocket, request);
        return;
    }
    
    // Find handler
    auto handlerIt = m_handlers.find(request.path);
    if (handlerIt != m_handlers.end()) {
        HttpResponse response = handlerIt->second(request);
        sendData(clientSocket, formatResponse(response));
        return;
    }
    
    // Check static directories
    for (const auto& staticDir : m_staticDirectories) {
        if (request.path.find(staticDir.first) == 0) {
            std::string filePath = staticDir.second + request.path.substr(staticDir.first.size());
            HttpResponse response = serveStaticFile(filePath);
            sendData(clientSocket, formatResponse(response));
            return;
        }
    }
    
    // Use default handler or 404
    if (m_defaultHandler) {
        HttpResponse response = m_defaultHandler(request);
        sendData(clientSocket, formatResponse(response));
    } else {
        HttpResponse response;
        response.statusCode = 404;
        response.statusText = "Not Found";
        response.body = std::vector<uint8_t>("404 Not Found".begin(), "404 Not Found".end());
        response.headers["Content-Type"] = "text/plain";
        sendData(clientSocket, formatResponse(response));
    }
}

HttpResponse WebServer::serveStaticFile(const std::string& path) {
    HttpResponse response;
    
    if (!FileSystem::exists(path)) {
        response.statusCode = 404;
        response.statusText = "Not Found";
        response.body = std::vector<uint8_t>("404 Not Found".begin(), "404 Not Found".end());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    
    // Read file
    std::vector<uint8_t> fileData = FileSystem::readFile(path);
    
    if (fileData.empty()) {
        response.statusCode = 500;
        response.statusText = "Internal Server Error";
        response.body = std::vector<uint8_t>("500 Internal Server Error".begin(), 
                                              "500 Internal Server Error".end());
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    
    // Set content type based on extension
    std::string ext = FileSystem::getExtension(path);
    if (ext == ".html") {
        response.headers["Content-Type"] = "text/html";
    } else if (ext == ".css") {
        response.headers["Content-Type"] = "text/css";
    } else if (ext == ".js") {
        response.headers["Content-Type"] = "application/javascript";
    } else if (ext == ".png") {
        response.headers["Content-Type"] = "image/png";
    } else if (ext == ".jpg" || ext == ".jpeg") {
        response.headers["Content-Type"] = "image/jpeg";
    } else {
        response.headers["Content-Type"] = "application/octet-stream";
    }
    
    response.body = fileData;
    return response;
}

void WebServer::handleWebSocketUpgrade(int clientSocket, const HttpRequest& request) {
    // In a full implementation, we'd handle WebSocket handshake
    // and upgrade the connection
    
    LOG_INFO_F("WebSocket upgrade requested for %s", request.path.c_str());
    
    // For now, just respond with 404
    HttpResponse response;
    response.statusCode = 404;
    response.statusText = "Not Found";
    response.body = std::vector<uint8_t>("WebSocket not implemented".begin(), 
                                          "WebSocket not implemented".end());
    sendData(clientSocket, formatResponse(response));
}

HttpRequest WebServer::parseRequest(const std::vector<uint8_t>& data) {
    HttpRequest request;
    
    std::string strData(data.begin(), data.end());
    std::istringstream stream(strData);
    
    // Parse request line
    std::string line;
    if (!std::getline(stream, line)) {
        return request;
    }
    
    std::istringstream requestLine(line);
    requestLine >> request.method >> request.path;
    
    // Parse query string
    size_t queryPos = request.path.find('?');
    if (queryPos != std::string::npos) {
        request.queryString = request.path.substr(queryPos + 1);
        request.path = request.path.substr(0, queryPos);
    }
    
    // Parse headers
    while (std::getline(stream, line) && line != "\r") {
        if (line.empty() || line == "\r\n") continue;
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" 	"));
            key.erase(key.find_last_not_of(" 	") + 1);
            value.erase(0, value.find_first_not_of(" 	"));
            value.erase(value.find_last_not_of(" 	\r\n") + 1);
            
            request.headers[key] = value;
        }
    }
    
    // Parse body
    size_t contentLength = 0;
    auto it = request.headers.find("Content-Length");
    if (it != request.headers.end()) {
        contentLength = std::stoul(it->second);
    }
    
    if (contentLength > 0 && data.size() > strData.size()) {
        size_t bodyStart = strData.size();
        size_t bodySize = std::min(static_cast<size_t>(contentLength), 
                                   data.size() - bodyStart);
        request.body.assign(data.begin() + bodyStart, data.begin() + bodyStart + bodySize);
    }
    
    return request;
}

std::vector<uint8_t> WebServer::formatResponse(const HttpResponse& response) {
    std::ostringstream stream;
    
    // Status line
    stream << "HTTP/1.1 " << response.statusCode << " " << response.statusText << "\r\n";
    
    // Headers
    for (const auto& header : response.headers) {
        stream << header.first << ": " << header.second << "\r\n";
    }
    
    // Add Content-Length if not set
    if (response.headers.find("Content-Length") == response.headers.end()) {
        stream << "Content-Length: " << response.body.size() << "\r\n";
    }
    
    // End of headers
    stream << "\r\n";
    
    // Body
    std::string headerStr = stream.str();
    std::vector<uint8_t> result(headerStr.begin(), headerStr.end());
    result.insert(result.end(), response.body.begin(), response.body.end());
    
    return result;
}

bool WebServer::sendData(int clientSocket, const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return true;
    }
    
    size_t offset = 0;
    size_t total = data.size();
    
    while (offset < total) {
        #if PLATFORM_WINDOWS
            int sent = ::send(clientSocket, reinterpret_cast<const char*>(data.data() + offset),
                            static_cast<int>(total - offset), 0);
        #else
            ssize_t sent = ::send(clientSocket, data.data() + offset, total - offset, 0);
        #endif
        
        if (sent <= 0) {
            return false;
        }
        
        offset += sent;
    }
    
    return true;
}

void WebServer::closeClient(int clientSocket) {
    #if PLATFORM_WINDOWS
        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
        }
    #else
        if (clientSocket >= 0) {
            close(clientSocket);
        }
    #endif
}

void WebServer::workerThread() {
    // Simplified worker thread
    // In a full implementation, this would use the platform-specific event system
    while (m_running) {
        TimeUtils::sleepMs(100);
    }
}
