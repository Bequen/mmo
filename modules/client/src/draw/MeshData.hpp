#pragma once

#include <vector>

#include "glm/glm.hpp"

namespace tw::drw {
class MeshData {
private:
    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_normals;

public:
    inline const std::vector<glm::vec3>& positions() const {
        return m_positions;
    }

    inline const std::vector<glm::vec3>& normals() const {
        return m_normals;
    }

    MeshData(std::vector<glm::vec3> positions, std::vector<glm::vec3> normals) :
        m_positions{positions},
        m_normals{normals}
    {
    }

    static MeshData plane(glm::vec2 scale) {
        return MeshData({
                    {-0.5f * scale.x,  0.0f, -0.5f * scale.y},
                    { 0.5f * scale.x,  0.0f, -0.5f * scale.y},
                    { 0.5f * scale.x,  0.0f,  0.5f * scale.y},

                    { 0.5f * scale.x,  0.0f,  0.5f * scale.y},
                    {-0.5f * scale.x,  0.0f,  0.5f * scale.y},
                    {-0.5f * scale.x,  0.0f, -0.5f * scale.y},
                }, {
                    {0.0f, 0.0f, 1.0f},
                    {0.0f, 0.0f, 1.0f},
                    {0.0f, 0.0f, 1.0f},

                    {0.0f, 0.0f, 1.0f},
                    {0.0f, 0.0f, 1.0f},
                    {0.0f, 0.0f, 1.0f},
                } );
    }

    static MeshData cube(glm::vec3 scale) {
        return MeshData({
                    {-0.5f, -0.5f,  0.5f},
                    { 0.5f, -0.5f,  0.5f},
                    { 0.5f,  0.5f,  0.5f},

                    { 0.5f,  0.5f,  0.5f},
                    {-0.5f,  0.5f,  0.5f},
                    {-0.5f, -0.5f,  0.5f},

                    // Back face (−Z)
                    { 0.5f, -0.5f, -0.5f},
                    {-0.5f, -0.5f, -0.5f},
                    {-0.5f,  0.5f, -0.5f},

                    {-0.5f,  0.5f, -0.5f},
                    { 0.5f,  0.5f, -0.5f},
                    { 0.5f, -0.5f, -0.5f},
                    
                    // Left face (−X)
                    {-0.5f, -0.5f, -0.5f},
                    {-0.5f, -0.5f,  0.5f},
                    {-0.5f,  0.5f,  0.5f},

                    {-0.5f,  0.5f,  0.5f},
                    {-0.5f,  0.5f, -0.5f},
                    {-0.5f, -0.5f, -0.5f},

                    // Right face (+X)
                    { 0.5f, -0.5f,  0.5f},
                    { 0.5f, -0.5f, -0.5f},
                    { 0.5f,  0.5f, -0.5f},

                    { 0.5f,  0.5f, -0.5f},
                    { 0.5f,  0.5f,  0.5f},
                    { 0.5f, -0.5f,  0.5f},

                    // Top face (+Y)
                    {-0.5f,  0.5f,  0.5f},
                    { 0.5f,  0.5f,  0.5f},
                    { 0.5f,  0.5f, -0.5f},

                    { 0.5f,  0.5f, -0.5f},
                    {-0.5f,  0.5f, -0.5f},
                    {-0.5f,  0.5f,  0.5f},

                    // Bottom face (−Y)
                    {-0.5f, -0.5f, -0.5f},
                    { 0.5f, -0.5f, -0.5f},
                    { 0.5f, -0.5f,  0.5f},

                    { 0.5f, -0.5f,  0.5f},
                    {-0.5f, -0.5f,  0.5f},
                    {-0.5f, -0.5f, -0.5f},
                }, {
                    { 0.0f,  0.0f,  1.0f},
                    { 0.0f,  0.0f,  1.0f},
                    { 0.0f,  0.0f,  1.0f},
                                         
                    { 0.0f,  0.0f,  1.0f},
                    { 0.0f,  0.0f,  1.0f},
                    { 0.0f,  0.0f,  1.0f},
                                         
                                         
                    { 0.0f,  0.0f, -1.0f},
                    { 0.0f,  0.0f, -1.0f},
                    { 0.0f,  0.0f, -1.0f},
                                         
                    { 0.0f,  0.0f, -1.0f},
                    { 0.0f,  0.0f, -1.0f},
                    { 0.0f,  0.0f, -1.0f},
                                         
                                         
                    {-1.0f,  0.0f,  0.0f},
                    {-1.0f,  0.0f,  0.0f},
                    {-1.0f,  0.0f,  0.0f},
                                         
                    {-1.0f,  0.0f,  0.0f},
                    {-1.0f,  0.0f,  0.0f},
                    {-1.0f,  0.0f,  0.0f},
                                         
                                         
                    { 1.0f,  0.0f,  0.0f},
                    { 1.0f,  0.0f,  0.0f},
                    { 1.0f,  0.0f,  0.0f},
                                         
                    { 1.0f,  0.0f,  0.0f},
                    { 1.0f,  0.0f,  0.0f},
                    { 1.0f,  0.0f,  0.0f},
                                         
                                         
                    { 0.0f,  1.0f,  0.0f},
                    { 0.0f,  1.0f,  0.0f},
                    { 0.0f,  1.0f,  0.0f},
                                         
                    { 0.0f,  1.0f,  0.0f},
                    { 0.0f,  1.0f,  0.0f},
                    { 0.0f,  1.0f,  0.0f},
                                         
                                         
                    { 0.0f, -1.0f,  0.0f},
                    { 0.0f, -1.0f,  0.0f},
                    { 0.0f, -1.0f,  0.0f},
                                         
                    { 0.0f, -1.0f,  0.0f},
                    { 0.0f, -1.0f,  0.0f},
                    { 0.0f, -1.0f,  0.0f},
                });
    }
};

}
