#pragma once

#include "logger.hpp"
#include "material.hpp"
#include <defines.hpp>

#include <model.hpp>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>

namespace Humongous
{
struct TransformComponent
{
    glm::vec3 translation{}; // position offset;
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::vec3 rotation{};

    glm::mat4 Mat4() const;
    glm::mat3 NormalMatrix();

    bool operator==(const TransformComponent& other)
    {
        return translation == other.translation && scale == other.scale && rotation == other.rotation;
    }
};

struct RigidBodyComponent
{
    glm::vec2 velocity;
    float     mass{1.0f};
};

class GameObject
{
public:
    using id_t = unsigned int;
    using Map = std::unordered_map<id_t, GameObject>;

    static GameObject CreateGameObject()
    {
        static id_t currentId = 0;
        return GameObject{currentId++};
    }

    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&&) = default;
    GameObject& operator=(GameObject&&) = default;

    const id_t GetId() { return m_id; };

    glm::vec3          color{};
    TransformComponent transform{};
    RigidBodyComponent rigidBody{};

    void        SetModel(std::shared_ptr<Model> model);
    void        Update();
    BoundingBox GetBoundingBox() const { return m_aabb; }

    static std::vector<glm::vec3> TransformAABBToWorldSpace(const Model::Dimensions& modelBB, const glm::mat4& modelMatrix);
    static BoundingBox            ComputeWorldAABB(const std::vector<glm::vec3>& worldCorners);

    std::shared_ptr<Model> model{};

private:
    id_t m_id;

    GameObject(id_t objId) : m_id{objId} {};
    BoundingBox m_aabb;

    TransformComponent m_prevFrameTransform{};
};

} // namespace Humongous
