#pragma once
#include "graphic/MeshType.hpp"

class Mesh;
class MeshStore
{

public:

    MeshStore();
    ~MeshStore();

    Mesh* getMesh(MeshType type);

private:

    Mesh* cube;
    Mesh* cacodemon;
    Mesh* flan;
};
