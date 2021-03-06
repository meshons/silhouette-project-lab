#include "model_loader.h"

model_loader::model_loader(std::string name) :
        dev_(nullptr),
        devcon_(nullptr),
        meshes_(),
        directory_(),
        textures_loaded_(),
        hwnd_(nullptr),
        name(name){
    // empty
    setBlockTransformation();
}


model_loader::~model_loader() {
    // empty
}

bool model_loader::Load(HWND hwnd, ID3D11Device * dev, ID3D11DeviceContext * devcon, std::string filename) {
    Assimp::Importer importer;

    const aiScene* pScene = importer.ReadFile(filename,
                                              aiProcess_Triangulate |
                                              aiProcess_ConvertToLeftHanded);

    if (pScene == nullptr)
        return false;

    this->directory_ = filename.substr(0, filename.find_last_of("/\\"));

    this->dev_ = dev;
    this->devcon_ = devcon;
    this->hwnd_ = hwnd;

    processNode(pScene->mRootNode, pScene);

    return true;
}

void model_loader::Draw(ID3D11DeviceContext * devcon) {
    for (size_t i = 0; i < meshes_.size(); ++i ) {
        meshes_[i].Draw(devcon);
    }
}

std::string textype;

mesh model_loader::processMesh(aiMesh * ai_mesh, const aiScene * scene) {
    // Data to fill
    std::vector<VERTEX> vertices;
    std::vector<UINT> indices;
    std::vector<texture> textures;

    if (ai_mesh->mMaterialIndex >= 0) {
        aiMaterial* mat = scene->mMaterials[ai_mesh->mMaterialIndex];

        if (textype.empty()) {
            textype = determineTextureType(scene, mat);
        }
    }

    // Walk through each of the mesh's vertices
    for (UINT i = 0; i < ai_mesh->mNumVertices; i++) {
        VERTEX vertex;

        vertex.X = ai_mesh->mVertices[i].x;
        vertex.Y = ai_mesh->mVertices[i].y;
        vertex.Z = ai_mesh->mVertices[i].z;

        if (ai_mesh->mTextureCoords[0]) {
            vertex.texcoord.x = (float)ai_mesh->mTextureCoords[0][i].x;
            vertex.texcoord.y = (float)ai_mesh->mTextureCoords[0][i].y;
        }

        vertices.push_back(vertex);
    }

    for (UINT i = 0; i < ai_mesh->mNumFaces; i++) {
        aiFace face = ai_mesh->mFaces[i];

        for (UINT j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    if (ai_mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[ai_mesh->mMaterialIndex];

        std::vector<texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    }

    return mesh(dev_, vertices, indices, textures);
}

std::vector<texture> model_loader::loadMaterialTextures(aiMaterial * mat, aiTextureType type, std::string typeName, const aiScene * scene) {
    std::vector<texture> textures;
    for (UINT i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        // Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        bool skip = false;
        for (UINT j = 0; j < textures_loaded_.size(); j++) {
            if (std::strcmp(textures_loaded_[j].path.c_str(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded_[j]);
                skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)
                break;
            }
        }
        if (!skip) {   // If texture hasn't been loaded already, load it
            HRESULT hr;
            texture texture;
            if (textype == "embedded compressed texture") {
                int textureindex = getTextureIndex(&str);
                texture.texture_ = getTextureFromModel(scene, textureindex);
            } else {
                std::string filename = std::string(str.C_Str());
                filename = directory_ + '/' + filename;
                std::wstring filenamews = std::wstring(filename.begin(), filename.end());
                hr = CreateWICTextureFromFile(dev_, devcon_, filenamews.c_str(), nullptr, &texture.texture_);
                if (FAILED(hr))
                    MessageBox(hwnd_, "Texture couldn't be loaded", "Error!", MB_ICONERROR | MB_OK);
            }
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            this->textures_loaded_.push_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
        }
    }
    return textures;
}

void model_loader::Close() {
    for (auto& t : textures_loaded_)
        t.Release();

    for (size_t i = 0; i < meshes_.size(); i++) {
        meshes_[i].Close();
    }
}

void model_loader::processNode(aiNode * node, const aiScene * scene) {
    for (UINT i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes_.push_back(this->processMesh(mesh, scene));
    }

    for (UINT i = 0; i < node->mNumChildren; i++) {
        this->processNode(node->mChildren[i], scene);
    }
}

std::string model_loader::determineTextureType(const aiScene * scene, aiMaterial * mat) {
    aiString textypeStr;
    mat->GetTexture(aiTextureType_DIFFUSE, 0, &textypeStr);
    std::string textypeteststr = textypeStr.C_Str();
    if (textypeteststr == "*0" || textypeteststr == "*1" || textypeteststr == "*2" || textypeteststr == "*3" || textypeteststr == "*4" || textypeteststr == "*5") {
        if (scene->mTextures[0]->mHeight == 0) {
            return "embedded compressed texture";
        } else {
            return "embedded non-compressed texture";
        }
    }
    if (textypeteststr.find('.') != std::string::npos) {
        return "textures are on disk";
    }

    return ".";
}

int model_loader::getTextureIndex(aiString * str) {
    std::string tistr;
    tistr = str->C_Str();
    tistr = tistr.substr(1);
    return stoi(tistr);
}

ID3D11ShaderResourceView * model_loader::getTextureFromModel(const aiScene * scene, int textureindex) {
    HRESULT hr;
    ID3D11ShaderResourceView *texture;

    int* size = reinterpret_cast<int*>(&scene->mTextures[textureindex]->mWidth);

    hr = CreateWICTextureFromMemory(dev_, devcon_, reinterpret_cast<unsigned char*>(scene->mTextures[textureindex]->pcData), *size, nullptr, &texture);
    if (FAILED(hr))
        MessageBox(hwnd_, "Texture couldn't be created from memory!", "Error!", MB_ICONERROR | MB_OK);

    return texture;
}

std::pair<FLOAT, VERTEX> model_loader::getMaxScaleAndCenter() {
    if (!centerAndScaleCalculated) {
        VERTEX min = {0.0, 0.0, 0.0}, max = {0.0, 0.0, 0.0};

        if (meshes_.size() < 1)
            return {1.0, {0.0, 0.0, 0.0}};

        for (const auto & mesh : meshes_) {
            for (const auto & vertex : mesh.vertices_) {
                if (vertex.X < min.X)
                    min.X = vertex.X;
                if (vertex.Y < min.Y)
                    min.Y = vertex.Y;
                if (vertex.Z < min.Z)
                    min.Z = vertex.Z;
                if (vertex.X > max.X)
                    max.X = vertex.X;
                if (vertex.Y > max.Y)
                    max.Y = vertex.Y;
                if (vertex.Z > max.Z)
                    max.Z = vertex.Z;
            }
        }

        center = {(max.X + min.X) / 2, (max.Y + min.Y) / 2, (max.Z + min.Z) / 2};

        FLOAT scaleX = abs(1 / (max.X - min.X));
        FLOAT scaleY = abs(1 / (max.Y - min.Y));
        FLOAT scaleZ = abs(1 / (max.Z - min.Z));

        scale = scaleX;
        if (scaleY < scale)
            scale = scaleY;
        if (scaleZ < scale)
            scale = scaleZ;

        centerAndScaleCalculated = true;
    }

    return {scale / 20.0, center};
}

void model_loader::rotate() {
    rotation = {
        float(rotationState % 30) / 30 * 2 * float(M_PI),
        float(rotationState / 30 % 30) / 30 * 2 * float(M_PI),
        float(rotationState / 30 / 30 % 30) / 30 * 2 * float(M_PI)
    };
    if (rotationState >= 30 * 30 * 30)
    {
        rotationState = 0;
        blockState++;
    }
    setBlockTransformation();
    rotationState++;
}
