#include "protocol/quicr/QuicrConnectionListener.hpp"
#include "protocol/quicr/QuicrEndpoint.hpp"

#include <spdlog/spdlog.h>

namespace tw::net::quicr {

QuicrConnectionListener::QuicrConnectionListener(QuicrEndpoint* endpoint) :
    m_endpoint(endpoint),
    m_listened_connections()
{
    endpoint->assign_listener(this);
}

QuicrConnectionListener::QuicrConnectionListener(QuicrConnectionListener&& other) :
    m_endpoint(std::move(other.m_endpoint)),
    m_listened_connections(std::move(other.m_listened_connections))
{ }

tl::expected<QuicrConnectionListener, NetworkError> QuicrConnectionListener::listen(QuicrEndpoint* endpoint) {
    return QuicrConnectionListener(endpoint);
};

QuicrConnection* QuicrConnectionListener::listen() {
    if(m_listened_connections.size() > 0) {
        spdlog::info("Listening");
        QuicrConnection* connection = m_listened_connections.front();
        m_listened_connections.pop_front();

        return connection;
    }

    return nullptr;
}

}
