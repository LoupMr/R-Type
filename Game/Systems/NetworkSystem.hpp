#ifndef NETWORKSYSTEM_HPP
#define NETWORKSYSTEM_HPP

#include "ECS/System.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/ComponentManager.hpp"
#include "Components/Components.hpp"  // For Position, etc.
#include <string>

class NetworkSystem : public System {
public:
    // Constructor takes server IP, server port, and the client port to bind to.
    NetworkSystem(const std::string& serverIP, int serverPort, int clientPort);
    ~NetworkSystem();

    // This update method is called each frame from your ECS update loop.
    void update(float dt, EntityManager& em, ComponentManager& cm) override;

private:
    int sock;                       // Socket descriptor.
    struct sockaddr_in serverAddr;  // Address structure for the server.

    // Helper function to send data over UDP.
    bool sendData(const std::string &data);
};

#endif // NETWORKSYSTEM_HPP
