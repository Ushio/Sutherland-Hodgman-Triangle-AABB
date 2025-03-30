#include "pr.hpp"
#include <iostream>
#include <memory>
float compMin(glm::vec3 v) {
    return glm::min(glm::min(v.x, v.y), v.z);
}
float compMax(glm::vec3 v) {
    return glm::max(glm::max(v.x, v.y), v.z);
}

template <class T>
inline T ss_max(T x, T y)
{
    return (x < y) ? y : x;
}

template <class T>
inline T ss_min(T x, T y)
{
    return (y < x) ? y : x;
}

template <class V3>
struct AABB
{
    V3 lower;
    V3 upper;

    void set_empty()
    {
        for (int i = 0; i < 3; i++)
        {
            lower[i] = +FLT_MAX;
            upper[i] = -FLT_MAX;
        }
    }
    void extend(const V3& p)
    {
        for (int i = 0; i < 3; i++)
        {
            lower[i] = ss_min(lower[i], p[i]);
            upper[i] = ss_max(upper[i], p[i]);
        }
    }
    AABB<V3> intersect(const AABB<V3>& rhs) const
    {
        AABB<V3> I;
        for (int i = 0; i < 3; i++)
        {
            I.lower[i] = ss_max(lower[i], rhs.lower[i]);
            I.upper[i] = ss_min(upper[i], rhs.upper[i]);
        }
        return I;
    }
};

inline AABB<glm::vec3> clip(const AABB<glm::vec3>& baseAABB, glm::vec3 a, glm::vec3 b, glm::vec3 c, float boundary, int axis, float dir )
{
    AABB<glm::vec3> divided;
    divided.set_empty();
    glm::vec3 vs[] = { a, b, c };
    for (int i = 0; i < 3; i++)
    {
        glm::vec3 ro = vs[i];
        glm::vec3 rd = vs[(i + 1) % 3] - ro;
        float one_over_rd = glm::clamp(1.0f / rd[axis], -FLT_MAX, FLT_MAX);
        float t = (boundary - ro[axis]) * one_over_rd;
        if (0.0f <= (ro[axis] - boundary) * dir)
        {
            divided.extend(ro);
        }

        if (0.0f < t && t < 1.0f)
        {
            glm::vec3 p = ro + rd * t;
            divided.extend(p);
        }
    }
    return baseAABB.intersect(divided);
}

int main() {
    using namespace pr;

    Config config;
    config.ScreenWidth = 1920;
    config.ScreenHeight = 1080;
    config.SwapInterval = 1;
    Initialize(config);

    Camera3D camera;
    camera.origin = { 4, 4, 4 };
    camera.lookat = { 0, 0, 0 };
    camera.zUp = true;

    double e = GetElapsedTime();

    while (pr::NextFrame() == false) {
        if (IsImGuiUsingMouse() == false) {
            UpdateCameraBlenderLike(&camera);
        }

        ClearBackground(0.1f, 0.1f, 0.1f, 1);

        BeginCamera(camera);

        PushGraphicState();

        DrawGrid(GridAxis::XY, 1.0f, 10, { 128, 128, 128 });
        DrawXYZAxis(1.0f);

        static glm::vec3 A = { 0, 0, 0 };
        static glm::vec3 B = { 1, 1, 1 };
        ManipulatePosition(camera, &A, 0.3f);
        ManipulatePosition(camera, &B, 0.3f);

        glm::vec3 lower = glm::min(A, B);
        glm::vec3 upper = glm::max(A, B);
        DrawAABB(lower, upper, {255,255,255});

        PCG rng(4, 0);
        glm::vec3 vs[3] = {
            { rng.uniformf(), rng.uniformf(), rng.uniformf()},
            { rng.uniformf(), rng.uniformf(), rng.uniformf()},
            { rng.uniformf(), rng.uniformf(), rng.uniformf()},
        };

        vs[2].z = vs[0].z;

        for (int i = 0; i < 3; i++)
        {
            DrawLine(vs[i], vs[(i + 1) % 3], {255, 0, 0});
        }

        static bool sutherland_hodgman = true;
        
        if (sutherland_hodgman)
        {
            std::vector<glm::vec3> xs_inputs(vs, vs + 3);
            std::vector<glm::vec3> xs_outputs;

            for (int axis = 0; axis < 3; axis++)
            {
                // lower clip
                for (int i = 0; i < xs_inputs.size(); i++)
                {
                    glm::vec3 a = xs_inputs[i];
                    glm::vec3 b = xs_inputs[(i + 1) % xs_inputs.size()];

                    bool inside_a = lower[axis] < a[axis];
                    bool inside_b = lower[axis] < b[axis];
                    if (inside_a && inside_b) // in -> in 
                    {
                        xs_outputs.push_back(b);
                    }
                    if (inside_a == false && inside_b == false) // out -> out
                    {
                        continue;
                    }
                    if (inside_a && inside_b == false) // in -> out
                    {
                        glm::vec3 rd = b - a;
                        float one_over_rd = glm::clamp(1.0f / rd[axis], -FLT_MAX, FLT_MAX);
                        float t = (lower[axis] - a[axis]) * one_over_rd;
                        glm::vec3 v = a + rd * t;
                        xs_outputs.push_back(v);
                    }
                    if (inside_a == false && inside_b) // out -> in
                    {
                        glm::vec3 rd = b - a;
                        float one_over_rd = glm::clamp(1.0f / rd[axis], -FLT_MAX, FLT_MAX);
                        float t = (lower[axis] - a[axis]) * one_over_rd;
                        glm::vec3 v = a + rd * t;
                        xs_outputs.push_back(v);
                        xs_outputs.push_back(b);
                    }
                }

                xs_inputs = xs_outputs;
                xs_outputs.clear();

                // Upper clip
                for (int i = 0; i < xs_inputs.size(); i++)
                {
                    glm::vec3 a = xs_inputs[i];
                    glm::vec3 b = xs_inputs[(i + 1) % xs_inputs.size()];

                    bool inside_a = a[axis] < upper[axis];
                    bool inside_b = b[axis] < upper[axis];
                    if (inside_a && inside_b) // in -> in 
                    {
                        xs_outputs.push_back(b);
                    }
                    if (inside_a == false && inside_b == false) // out -> out
                    {
                        continue;
                    }
                    if (inside_a && inside_b == false) // in -> out
                    {
                        glm::vec3 rd = b - a;
                        float one_over_rd = glm::clamp(1.0f / rd[axis], -FLT_MAX, FLT_MAX);
                        float t = (upper[axis] - a[axis]) * one_over_rd;
                        glm::vec3 v = a + rd * t;
                        xs_outputs.push_back(v);
                    }
                    if (inside_a == false && inside_b) // out -> in
                    {
                        glm::vec3 rd = b - a;
                        float one_over_rd = glm::clamp(1.0f / rd[axis], -FLT_MAX, FLT_MAX);
                        float t = (upper[axis] - a[axis]) * one_over_rd;
                        glm::vec3 v = a + rd * t;
                        xs_outputs.push_back(v);
                        xs_outputs.push_back(b);
                    }
                }
                xs_inputs = xs_outputs;
                xs_outputs.clear();
            }

            if (!xs_inputs.empty())
            {
                PrimBegin(PrimitiveMode::LineStrip, 2);
                for (int i = 0; i < xs_inputs.size() + 1; i++)
                {
                    PrimVertex(xs_inputs[i % xs_inputs.size()], { 255, 255, 0 });
                }
                PrimEnd();
            }

            AABB<glm::vec3> clipped;
            clipped.set_empty();

            for (auto p : xs_inputs)
            {
                clipped.extend(p);
            }
            DrawAABB(clipped.lower, clipped.upper, { 255, 255, 0 });
        }

        // Binned SAH Kd-Tree Construction on a GPU
        static bool incremental_divide = true;

        if (incremental_divide)
        {
            AABB<glm::vec3> clipped = { lower, upper };
            for (int axis = 0; axis < 3; axis++)
            {
                clipped = clip(clipped, vs[0], vs[1], vs[2], lower[axis], axis, +1.0f);
                clipped = clip(clipped, vs[0], vs[1], vs[2], upper[axis], axis, -1.0f);
            }
            DrawAABB(clipped.lower, clipped.upper, { 0, 255, 255 }, 2);
        }



        PopGraphicState();
        EndCamera();

        BeginImGui();

        ImGui::SetNextWindowSize({ 500, 800 }, ImGuiCond_Once);
        ImGui::Begin("Panel");
        ImGui::Text("fps = %f", GetFrameRate());
        ImGui::Checkbox("sutherland hodgman (yellow)", &sutherland_hodgman);
        ImGui::Checkbox("incremental divide (cyan)", &incremental_divide);

        ImGui::End();

        EndImGui();
    }

    pr::CleanUp();
}
