// https://github.com/assimp/assimp/blob/master/samples/SimpleTexturedDirectx11/SimpleTexturedDirectx11/ModelLoader.h

#ifndef SILHOUETTE_MODEL_LOADER_H
#define SILHOUETTE_MODEL_LOADER_H

#include <vector>
#include <string>
#include <d3d11_1.h>
#include <cstdlib>
#include <DirectXMath.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include "mesh.h"
#include "texture_loader.h"

using namespace DirectX;

class model_loader
{
    std::string name;

public:
    model_loader(std::string name);
    ~model_loader();

    bool Load(HWND hwnd, ID3D11Device* dev, ID3D11DeviceContext* devcon, std::string filename);
    void Draw(ID3D11DeviceContext* devcon);

    void Close();

    std::pair<FLOAT, VERTEX> getMaxScaleAndCenter();

    void rotate();

    std::string & getName() { return name; }

    int getRotationState() { return rotationState; }
    int getBlockState() { return blockState; }

    VERTEX getRotation() { return rotation; }
    VERTEX getBlockTransformation() { return blockTransformation; }
    bool modelEnd() { return rotationState >= 30 * 30 * 30 && blockState >= 15; }
    std::string getState()
    {
        std::string state = "_B_" + std::to_string(blockState) + "_R_";
        state += std::to_string(rotationState % 30 / 5) + "_";
        state += std::to_string(rotationState / 30 % 30 / 5) + "_";
        state += std::to_string(rotationState / 30 / 30 % 30 / 5);
        return state;
    }
private:
    ID3D11Device *dev_;
    ID3D11DeviceContext *devcon_;
    std::vector<mesh> meshes_;
    std::string directory_;
    std::vector<texture> textures_loaded_;
    HWND hwnd_;

    bool centerAndScaleCalculated = false;
    FLOAT scale = 1.0;
    VERTEX center = {0.0, 0.0, 0.0};
    VERTEX rotation = {0.0, 0.0, 0.0};
    VERTEX blockTransformation;
    unsigned int rotationState = 0;
    unsigned int blockState = 0;

    void processNode(aiNode* node, const aiScene* scene);
    mesh processMesh(aiMesh* ai_mesh, const aiScene* scene);
    std::vector<texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
    std::string determineTextureType(const aiScene* scene, aiMaterial* mat);
    int getTextureIndex(aiString* str);
    ID3D11ShaderResourceView* getTextureFromModel(const aiScene* scene, int textureindex);

	void setBlockTransformation() {
        blockTransformation = {
	    (float(blockState % 4) - 1.5f) * 0.18f + float(std::rand() % 100 - 50) / 100.0f * 0.15f,
	    -(float(blockState / 4) - 1.5f) * 0.18f + float(std::rand() % 100 - 50) / 100.0f * 0.15f,
	    0.0
        };
	}
};

#endif //SILHOUETTE_MODEL_LOADER_H
