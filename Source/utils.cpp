#include "utils.h"

#include <algorithm>

namespace Utils {
    // https://iquilezles.org/articles/intersectors/
    bool RayBoxIntersection(const Ray& ray, const glm::vec3& min, const glm::vec3& max, glm::vec2& t)
    {
        glm::vec3 invRd = 1.0f / ray.direction;
        glm::vec3 t0 = (min - ray.origin) * invRd;
        glm::vec3 t1 = (max - ray.origin) * invRd;

        float tMin = t0.x;
        float tMax = t1.x;
        if (tMin > tMax) std::swap(tMin, tMax);

        if (t0.y > t1.y) std::swap(t0.y, t1.y);

        if (t0.y > tMax || tMin > t1.y) return false;
        tMin = std::max(t0.y, tMin);
        tMax = std::min(t1.y, tMax);

        if (t0.z > t1.z) std::swap(t0.z, t1.z);

        if (t0.z > tMax || tMin > t1.z) return false;
        tMin = std::max(t0.z, tMin);
        tMax = std::min(t1.z, tMax);

        t = { tMin, tMax };
        return true;
    }

    glm::vec3 GetRayDir(const glm::mat4& P, const glm::mat4& V, const glm::vec2& ndcCoord)
    {
        glm::vec4 ndc{ ndcCoord, -1.0f, 1.0f };
        glm::vec4 viewCoord = glm::inverse(P) * ndc;
        viewCoord.x /= viewCoord.w;
        viewCoord.y /= viewCoord.w;
        viewCoord.z = -1.0f;
        viewCoord.w = 0.0f;

        glm::vec4 worldCoord = glm::inverse(V) * viewCoord;
        return glm::normalize(glm::vec3(worldCoord));
    }

}