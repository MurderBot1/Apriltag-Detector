#pragma once

#include "NT4Client.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

/**
 * @brief Convenience class for NT4 subscriptions
 */
class NT4Subscriber {
public:
    using SubscribeCallback = std::function<void(const std::string&, const std::vector<uint8_t>&)>;
    
    /**
     * @brief Constructor
     * @param client NT4Client instance
     */
    explicit NT4Subscriber(NT4Client* client);
    
    /**
     * @brief Destructor
     */
    ~NT4Subscriber();
    
    /**
     * @brief Set the NT4 client
     * @param client NT4Client instance
     */
    void setClient(NT4Client* client);
    
    /**
     * @brief Subscribe to a topic
     * @param topic Topic name
     * @param callback Callback for received data
     * @return true if subscribed successfully
     */
    bool subscribe(const std::string& topic, SubscribeCallback callback);
    
    /**
     * @brief Unsubscribe from a topic
     * @param topic Topic name
     */
    void unsubscribe(const std::string& topic);
    
    /**
     * @brief Check if connected
     * @return true if connected
     */
    bool isConnected() const;
    
private:
    NT4Client* m_client;
};
