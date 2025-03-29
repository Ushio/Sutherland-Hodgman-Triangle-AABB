#include "pr.hpp"
#include <iostream>
#include <memory>
float compMin(glm::vec3 v) {
    return glm::min(glm::min(v.x, v.y), v.z);
}
float compMax(glm::vec3 v) {
    return glm::max(glm::max(v.x, v.y), v.z);
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
            PrimBegin(PrimitiveMode::LineStrip);
            for (int i = 0; i < xs_inputs.size() + 1; i++)
            {
                PrimVertex(xs_inputs[i % xs_inputs.size()], { 255, 255, 0 });
            }
            PrimEnd();
        }

        glm::vec3 clipped_lower = { FLT_MAX ,FLT_MAX, FLT_MAX };
        glm::vec3 clipped_upper = { -FLT_MAX ,-FLT_MAX, -FLT_MAX };

        for (auto p : xs_inputs)
        {
            clipped_lower = glm::min(clipped_lower, p);
            clipped_upper = glm::max(clipped_upper, p);
        }
        //for (int i = 0; i < 3; i++)
        //{
        //    glm::vec3 ro = vs[i];
        //    glm::vec3 rd = vs[(i + 1) % 3] - ro;
        //    glm::vec3 one_over_rd = glm::clamp(1.0f / rd, -FLT_MAX, FLT_MAX);
        //    glm::vec3 t0 = (lower - ro) * one_over_rd;
        //    glm::vec3 t1 = (upper - ro) * one_over_rd;
        //    glm::vec3 tmin = min(t0, t1);
        //    glm::vec3 tmax = max(t0, t1);

        //    float region_min = compMax(tmin);
        //    float region_max = compMin(tmax);

        //    if(region_min <= 0.0f && 0.0f <= region_max ) // the origin is included 
        //    {
        //        clipped_lower = glm::min(clipped_lower, ro);
        //        clipped_upper = glm::max(clipped_upper, ro);
        //        DrawSphere(ro, 0.02f, { 255, 0, 0 });
        //        DrawText(ro, std::to_string(i));
        //    }
        //    if( region_min <= region_max ) // the segment contacts the box volume
        //    {
        //        if (0.0f < region_min && region_min < 1.0f) // the edge contacts box surface
        //        {
        //            glm::vec3 p_min = ro + rd * region_min;
        //            clipped_lower = glm::min(clipped_lower, p_min);
        //            clipped_upper = glm::max(clipped_upper, p_min);
        //            DrawSphere(p_min, 0.01f, { 0, 255, 0 });
        //            DrawText(p_min, std::to_string(i));
        //        }
        //        if (0.0f < region_max && region_max < 1.0f) // the edge contacts box surface
        //        {
        //            glm::vec3 p_max = ro + rd * region_max;
        //            clipped_lower = glm::min(clipped_lower, p_max);
        //            clipped_upper = glm::max(clipped_upper, p_max);

        //            DrawSphere(p_max, 0.01f, { 0, 0, 255 });
        //            DrawText(p_max, std::to_string(i));
        //        }
        //    }
        //}

        DrawAABB(clipped_lower, clipped_upper, { 255,0,255 });

        PopGraphicState();
        EndCamera();

        BeginImGui();

        ImGui::SetNextWindowSize({ 500, 800 }, ImGuiCond_Once);
        ImGui::Begin("Panel");
        ImGui::Text("fps = %f", GetFrameRate());

        ImGui::End();

        EndImGui();
    }

    pr::CleanUp();
}
