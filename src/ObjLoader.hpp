#ifndef OBJ_LOADER_HPP
#define OBJ_LOADER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

using namespace std;

// Coeficientes de material lidos do .mtl
struct Material {
    glm::vec3 ka = glm::vec3(0.1f);   // ambiente
    glm::vec3 kd = glm::vec3(0.8f);   // difuso
    glm::vec3 ks = glm::vec3(0.5f);   // especular
    float     ns = 32.0f;              // brilho (shininess)
    string    textureName;
};

inline Material loadMTL(const string& mtlFilePath)
{
    Material mat;
    ifstream f(mtlFilePath);
    if (!f.is_open()) {
        cerr << "Aviso: nao foi possivel abrir MTL " << mtlFilePath << endl;
        return mat;
    }

    string line;
    while (getline(f, line)) {
        istringstream ss(line);
        string token;
        ss >> token;

        if      (token == "Ka") ss >> mat.ka.r >> mat.ka.g >> mat.ka.b;
        else if (token == "Kd") ss >> mat.kd.r >> mat.kd.g >> mat.kd.b;
        else if (token == "Ks") ss >> mat.ks.r >> mat.ks.g >> mat.ks.b;
        else if (token == "Ns") ss >> mat.ns;
        else if (token == "map_Kd") ss >> mat.textureName;
    }
    return mat;
}

// Mantida por compatibilidade com código antigo
inline string loadMTLTextureName(const string& mtlFilePath)
{
    return loadMTL(mtlFilePath).textureName;
}

// Carrega OBJ → buffer: position(3) | normal(3) | texcoord(2)  stride = 8
// Lê também o MTL e preenche 'material'
inline GLuint loadSimpleOBJ(const string& filePATH, int& nVertices,
                             string& texturePath, Material* material = nullptr)
{
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat>   vBuffer;

    ifstream arqEntrada(filePATH);
    if (!arqEntrada.is_open()) {
        cerr << "Erro ao abrir OBJ: " << filePATH << endl;
        nVertices = 0;
        return 0;
    }

    string basePath;
    size_t lastSlash = filePATH.find_last_of("/\\");
    if (lastSlash != string::npos) basePath = filePATH.substr(0, lastSlash + 1);

    string line;
    while (getline(arqEntrada, line)) {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "mtllib") {
            string mtlFile;
            ssline >> mtlFile;
            Material m = loadMTL(basePath + mtlFile);
            texturePath = m.textureName;
            if (material) *material = m;
        }
        else if (word == "v") {
            glm::vec3 v; ssline >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (word == "vt") {
            glm::vec2 vt; ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn") {
            glm::vec3 n; ssline >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (word == "f") {
            while (ssline >> word) {
                int vi = 0, ti = 0, ni = 0;
                istringstream ss(word);
                string idx;
                if (getline(ss, idx, '/')) vi = !idx.empty() ? stoi(idx) - 1 : 0;
                if (getline(ss, idx, '/')) ti = !idx.empty() ? stoi(idx) - 1 : 0;
                if (getline(ss, idx))      ni = !idx.empty() ? stoi(idx) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);

                if (!normals.empty() && ni >= 0 && ni < (int)normals.size()) {
                    vBuffer.push_back(normals[ni].x);
                    vBuffer.push_back(normals[ni].y);
                    vBuffer.push_back(normals[ni].z);
                } else {
                    vBuffer.push_back(0.0f); vBuffer.push_back(1.0f); vBuffer.push_back(0.0f);
                }

                if (!texCoords.empty() && ti >= 0 && ti < (int)texCoords.size()) {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                } else {
                    vBuffer.push_back(0.0f); vBuffer.push_back(0.0f);
                }
            }
        }
    }
    arqEntrada.close();

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    GLsizei stride = 8 * sizeof(GLfloat);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = (int)vBuffer.size() / 8;
    return VAO;
}

#endif
