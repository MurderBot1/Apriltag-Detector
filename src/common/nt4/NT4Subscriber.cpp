#include "NT4Subscriber.hpp"

NT4Subscriber::NT4Subscriber(NT4Client* client) : m_client(client) {
}

NT4Subscriber::~NT4Subscriber() {
}

void NT4Subscriber::setClient(NT4Client* client) {
    m_client = client;
}

bool NT4Subscriber::subscribe(const std::string& topic, SubscribeCallback callback) {
    if (!m_client) {
        return false;
    }
    
    return m_client->subscribe(topic, callback);
}

void NT4Subscriber::unsubscribe(const std::string& topic) {
    if (!m_client) {
        return;
    }
    
    m_client->unsubscribe(topic);
}

bool NT4Subscriber::isConnected() const {
    if (!m_client) {
        return false;
    }
    
    return m_client->isConnected();
}
