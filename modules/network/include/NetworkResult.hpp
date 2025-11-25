#pragma once

#include <functional>

namespace tw::net {

class NetworkResult {
private:
    int m_errno;

    NetworkResult(int e) :
        m_errno(e) 
    {
        
    }

public:
    bool is(int err) {
        return m_errno == err;
    }

    inline bool is_ok() {
        return m_errno == 0;
    }

    static NetworkResult ok() {
        return {0};
    }

    static NetworkResult from_errno(int e) {
        return NetworkResult(e);
    }

    NetworkResult& and_then(std::function<void(NetworkResult)> handler) {
        if(!is_ok()) {
            handler(*this);
        }
        return *this;
    }
};

}
