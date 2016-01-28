#include "Mesh.hpp"
#include "Material.hpp"
#include "MeshVertexArray.hpp"
#include "../utils/Texture.hpp"
#include "../utils/OBJ.hpp"
#include "../utils/log.hpp"

using namespace std;
using namespace glm;

Mesh::Mesh(const char *filename)
    : vertexArray(new MeshVertexArray())
    , diffuseTexture(new Texture())
    , metallicTexture(new Texture())
    , roughTexture(new Texture())
    , normalTexture(new Texture())
{
    OBJ(triangles, vertexes, uvs, normals, indexes, materials).load(filename);

    verifyUVs();
    verifyMeterials();

    initializeTriangleData();
    computeTrianglesPlaneEquations();
    // computeTrianglesNeighbours();
    computeTrianglesTangents();

    vertexArray->initialize(vertexes, uvs, normals, trianglesTangents, trianglesBitangents);

    loadTextures();
}

Mesh::~Mesh()
{
    delete diffuseTexture;
    delete metallicTexture;
    delete roughTexture;
    delete normalTexture;
    delete vertexArray;
}

void Mesh::reloadTextures()
{
    loadTextures();
}

void Mesh::debug()
{
    OBJ::debug(triangles, vertexes, uvs, normals, indexes, materials);
}

void Mesh::loadTextures()
{
    if (materials.size() > 0) {
        diffuseTexture->load(materials[0].map_diffuse.data());
        metallicTexture->load(materials[0].map_metallic.data());
        roughTexture->load(materials[0].map_rough.data());
        normalTexture->load(materials[0].map_normal.data());
    }
}

void Mesh::bindTexture(GLuint diffuse, GLuint metallic, GLuint rough, GLuint normal)
{
    diffuseTexture->bind(diffuse);
    metallicTexture->bind(metallic);
    roughTexture->bind(rough);
    normalTexture->bind(normal);
}

void Mesh::bindIndexes()
{
    vertexArray->uploadIndexes(indexes);
}

void Mesh::bindSilhouette()
{
    vertexArray->uploadIndexes(silhouette);
}

void Mesh::updateShadowVolume(const vec4 &lightDirection)
{
    updateTrianglesVisibility(lightDirection);
    updateSilhouette();
}

void Mesh::updateMatrices(unsigned int instances, const mat4* matrices)
{
    vertexArray->uploadMatrices(instances, matrices);
}

void Mesh::draw(unsigned int instances)
{
    vertexArray->bind();
    glDrawElementsInstanced(GL_TRIANGLES, GLsizei(indexes.size()), GL_UNSIGNED_INT, 0, instances);
    vertexArray->idle();
}

void Mesh::drawAdjacency(unsigned int instances)
{
    vertexArray->bind();
    glDrawElementsInstanced(GL_TRIANGLES_ADJACENCY, GLsizei(indexes.size()), GL_UNSIGNED_INT, 0, instances);
    vertexArray->idle();
}

void Mesh::drawShadowVolume()
{
    if (silhouette.size() == 0) return;

    vertexArray->bind();
    glDrawElements(GL_TRIANGLES, GLsizei(silhouette.size()), GL_UNSIGNED_INT, 0);
    vertexArray->idle();
}

void Mesh::verifyUVs()
{
    if (uvs.size() < vertexes.size()) {
        uvs.resize(vertexes.size(), vec2(0.f));
    }
}

void Mesh::verifyMeterials()
{
    if (materials.size() == 0) {
        materials.push_back(Material("default"));
    }
}

void Mesh::initializeTriangleData()
{
    trianglesVisibility.resize(triangles.size(), false);
    trianglesNeighbours.resize(triangles.size(), ivec3(-1, -1, -1));
    trianglesTangents.resize(vertexes.size(), fvec3(0.f, 0.f, 0.f));
    trianglesBitangents.resize(vertexes.size(), fvec3(0.f, 0.f, 0.f));
    trianglesPlaneEquations.reserve(triangles.size());
}

void Mesh::computeTrianglesTangents()
{
    std::vector<glm::vec3> tan1;
    std::vector<glm::vec3> tan2;

    tan1.reserve(vertexes.size() * 2);
    tan2.reserve(vertexes.size() * 2);

    for (auto &triangle : triangles) {
        unsigned int i1 = triangle[0];
        unsigned int i2 = triangle[1];
        unsigned int i3 = triangle[2];

        const vec4& v1 = vertexes[i1];
        const vec4& v2 = vertexes[i2];
        const vec4& v3 = vertexes[i3];

        const vec3 edge1 = vec3(v2 - v1);
        const vec3 edge2 = vec3(v3 - v1);

        const vec2& uv1 = uvs[i1];
        const vec2& uv2 = uvs[i2];
        const vec2& uv3 = uvs[i3];

        const vec2 deltaUV1 = uv2 - uv1;
        const vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        vec3 tangent = normalize(vec3(
            f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
            f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
            f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
        ));

        vec3 bitangent = normalize(vec3(
            f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x),
            f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y),
            f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z)
        ));

        trianglesTangents[i1] = trianglesTangents[i2] = trianglesTangents[i3] = tangent;
        trianglesBitangents[i1] = trianglesBitangents[i2] = trianglesBitangents[i3] = bitangent;
    }
}

void Mesh::computeTrianglesPlaneEquations()
{
    for (auto &triangle : triangles) {
        const vec4& v1 = vertexes[triangle[0]];
        const vec4& v2 = vertexes[triangle[1]];
        const vec4& v3 = vertexes[triangle[2]];

        trianglesPlaneEquations.push_back(fvec4(
              v1.y * (v2.z - v3.z)
            + v2.y * (v3.z - v1.z)
            + v3.y * (v1.z - v2.z),

              v1.z * (v2.x - v3.x)
            + v2.z * (v3.x - v1.x)
            + v3.z * (v1.x - v2.x),

              v1.x * (v2.y - v3.y)
            + v2.x * (v3.y - v1.y)
            + v3.x * (v1.y - v2.y),

            -(v1.x * (v2.y * v3.z - v3.y * v2.z)
            + v2.x * (v3.y * v1.z - v1.y * v3.z)
            + v3.x * (v1.y * v2.z - v2.y * v1.z))
        ));
    }
}

void Mesh::computeTrianglesNeighbours()
{
    for (unsigned int t1 = 0; t1 < triangles.size(); t1++) {
        if (trianglesNeighbours[t1][0] != -1 && trianglesNeighbours[t1][1] != -1 && trianglesNeighbours[t1][2] != -1) continue;
        for (unsigned int t2 = t1 + 1; t2 < triangles.size(); t2++) {
            for (int a = 0; a < 3; a++) {
                if (trianglesNeighbours[t1][a] != -1) continue;
                for (int b = 0; b < 3; b++) {
                    unsigned int triangle_1_indexA = triangles[t1][a];
                    unsigned int triangle_1_indexB = triangles[t1][(a + 1) % 3];
                    unsigned int triangle_2_indexA = triangles[t2][b];
                    unsigned int triangle_2_indexB = triangles[t2][(b + 1) % 3];

                    if ((triangle_1_indexA == triangle_2_indexA && triangle_1_indexB == triangle_2_indexB)
                            || (triangle_1_indexA == triangle_2_indexB && triangle_1_indexB == triangle_2_indexA)) {
                        trianglesNeighbours[t1][a] = int(t2);
                        trianglesNeighbours[t2][b] = int(t1);
                        break;
                    }
                }
            }
        }
    }
}

void Mesh::updateTrianglesVisibility(const vec4 &lightDirection)
{
	// TODO optimize
    for (unsigned int t = 0; t < triangles.size(); t ++) {
        trianglesVisibility[t] = dot(trianglesPlaneEquations[t], lightDirection) > 0.f;
    }
}

void Mesh::updateSilhouette()
{
	// TODO optimize
    silhouette.clear();
    unsigned int totalVertexes = unsigned(vertexes.size());
    for (unsigned int t = 0; t < triangles.size(); t ++) {
        if (trianglesVisibility[t]) {
            for (int edge = 0; edge < 3; edge ++){ // For each visible triangle's edges
                // If edge's neighbouring face is not visible, or if it has no neighbour
                // then this edge's vertexes are part of the silhouette
                int neighbourIndex = trianglesNeighbours[t][edge];
                if (neighbourIndex == -1 || trianglesVisibility[unsigned(neighbourIndex)] == false) {
                    silhouette.push_back(triangles[t][(edge + 1) % 3]);
                    silhouette.push_back(triangles[t][edge] + totalVertexes);
                    silhouette.push_back(triangles[t][edge]);
                }
            }
        }
    }
}
