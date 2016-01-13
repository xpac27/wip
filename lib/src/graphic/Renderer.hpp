#pragma once
#include "../utils/Program.hpp"
#include "../utils/Aggregator.hpp"
#include "../utils/Cubemap.hpp"
#include "Model.hpp"
#include "MeshStore.hpp"
#include "GBuffer.hpp"
#include "Quad.hpp"
#include "DirectionalLight.hpp"
#include <glm/vec3.hpp>

class Camera;

class Renderer
{

public:

    Renderer();
    ~Renderer();

    void initialize();
    void render(Aggregator<Model> &models);
    void reload();

private:

    void uploadMatrices(Aggregator<Model> &models);
    void depthPass(Aggregator<Model> &models);
    void shadowPass(Aggregator<Model> &models);
    void geometryPass(Aggregator<Model> &models);
    void lightingPass();
    // TODO put that in a separate class?
    void initializeShaders();
    void initializeShader(Program &program, const char* vertexShaderFilePath, const char* fragmentShaderFilePath);

    Program shadowVolume;
    Program filling;
    Program geometryBuffer;
    Program deferredShading;

    Quad quad;
    GBuffer gBuffer;
    Cubemap cubemap;
    MeshStore meshStore;
    DirectionalLight directionalLight;

    // TODO init from outside (systems class?)
    Camera *camera;
};
