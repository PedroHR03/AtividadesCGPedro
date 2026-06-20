/* Exercício de Textura + Iluminação de 3 Pontos - Pedro
 *
 * Controles:
 *   1 - Liga/desliga Luz Principal  (Key Light)
 *   2 - Liga/desliga Luz de Preench. (Fill Light)
 *   3 - Liga/desliga Luz de Fundo   (Back Light)
 *   ESC - Sair
 *
 * Buffer de vértice (ObjLoader): position(loc=0) | normal(loc=1) | texcoord(loc=2)
 * Stride = 8 floats
 */

#include <iostream>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "ObjLoader.hpp"

using namespace std;

// --- Protótipos ---
GLuint loadTexture(const string& filePath);
void   key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// --- Estado das luzes (toggled por teclas 1/2/3) ---
bool luzAtiva[3] = { true, true, true };

// ---------------------------------------------------------------------------
// Vertex shader: Phong — calcula posição em espaço de mundo e passa normal
// ---------------------------------------------------------------------------
const GLchar* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 TexCoord;

uniform mat4 model, view, projection;

void main()
{
    vec4 worldPos   = model * vec4(position, 1.0);
    fragPos         = vec3(worldPos);
    fragNormal      = mat3(transpose(inverse(model))) * normal;
    TexCoord        = texCoord;
    gl_Position     = projection * view * worldPos;
}
)glsl";

// ---------------------------------------------------------------------------
// Fragment shader: iluminação de Phong com 3 luzes pontuais + atenuação
// ---------------------------------------------------------------------------
const GLchar* fragmentShaderSource = R"glsl(
#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 TexCoord;

out vec4 outColor;

uniform sampler2D ourTexture;
uniform vec3 viewPos;

// Cada luz: posição, cor e flag de ativação
uniform vec3  lightPos[3];
uniform vec3  lightColor[3];
uniform bool  lightOn[3];

// Intensidades relativas para cada papel (key / fill / back)
// key=1.0, fill=0.5, back=0.3  — definido no C++, mas o shader só usa lightColor
const float ambientStrength  = 0.08;
const float specularStrength = 0.4;
const float shininess        = 32.0;

// Coeficientes de atenuação (quadrática)
const float kC = 1.0;
const float kL = 0.09;
const float kQ = 0.032;

void main()
{
    vec3 norm    = normalize(fragNormal);
    vec3 texCol  = vec3(texture(ourTexture, TexCoord));

    vec3 result = vec3(0.0);

    for (int i = 0; i < 3; i++)
    {
        if (!lightOn[i]) continue;

        vec3  lDir  = lightPos[i] - fragPos;
        float dist  = length(lDir);
        lDir        = normalize(lDir);

        // Atenuação
        float atten = 1.0 / (kC + kL * dist + kQ * dist * dist);

        // Ambiente
        vec3 ambient = ambientStrength * lightColor[i];

        // Difuso
        float diff   = max(dot(norm, lDir), 0.0);
        vec3 diffuse = diff * lightColor[i] * atten;

        // Especular (Phong)
        vec3 viewDir = normalize(viewPos - fragPos);
        vec3 reflDir = reflect(-lDir, norm);
        float spec   = pow(max(dot(viewDir, reflDir), 0.0), shininess);
        vec3 specular = specularStrength * spec * lightColor[i] * atten;

        result += (ambient + diffuse + specular) * texCol;
    }

    outColor = vec4(result, 1.0);
}
)glsl";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Textura + Iluminação 3 Pontos", nullptr, nullptr);
    if (!window) {
        cout << "Falha ao criar janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // --- Compilação dos shaders ---
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);

    GLuint shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // --- Carrega OBJ e textura ---
    int nVertices = 0;
    string texName;
    GLuint VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVertices, texName);

    GLuint textureID = 0;
    if (!texName.empty())
        textureID = loadTexture("../assets/Modelos3D/" + texName);
    else
        cout << "Aviso: nenhuma textura encontrada no MTL." << endl;

    // --- Câmera e projeção ---
    glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glm::mat4 view       = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // -------------------------------------------------------------------
    // Iluminação de 3 pontos posicionada automaticamente em relação ao
    // objeto principal (posição = origem, escala ≈ 1.0)
    //
    // Convenções:
    //   Key  light : frente-direita-cima,  intensidade 1.0 (branca)
    //   Fill light : frente-esquerda,      intensidade 0.5 (branca fria)
    //   Back light : atrás-cima,           intensidade 0.7 (levemente azulada)
    //
    // Os offsets são escalados pelo raio aproximado do objeto (aqui ~1.5)
    // para posicionar as luzes proporcionalmente ao tamanho do objeto.
    // -------------------------------------------------------------------
    const glm::vec3 objCenter(0.0f, 0.0f, 0.0f);
    const float     objRadius = 1.5f;  // raio aproximado da Suzanne em unidades de mundo

    glm::vec3 lightPos[3] = {
        objCenter + glm::vec3( 1.5f,  1.2f,  2.0f) * objRadius,  // Key  (frente-direita-cima)
        objCenter + glm::vec3(-1.5f,  0.2f,  1.5f) * objRadius,  // Fill (frente-esquerda)
        objCenter + glm::vec3( 0.0f,  1.0f, -2.0f) * objRadius,  // Back (atrás-cima)
    };

    glm::vec3 lightColor[3] = {
        glm::vec3(1.0f,  1.0f,  1.0f) * 1.0f,   // Key  — branca, intensidade total
        glm::vec3(0.85f, 0.9f,  1.0f) * 0.5f,   // Fill — branca fria, meia intensidade
        glm::vec3(0.7f,  0.8f,  1.0f) * 0.7f,   // Back — azulada, 70%
    };

    cout << "\nControles de Iluminação:" << endl;
    cout << "  1 - Luz Principal  (Key Light)"   << endl;
    cout << "  2 - Luz de Preenchimento (Fill)"  << endl;
    cout << "  3 - Luz de Fundo   (Back Light)"  << endl;

    // --- Loop Principal ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Atualiza título com estado de cada luz
        string titulo = "3-Point Light | Key:";
        titulo += luzAtiva[0] ? "ON" : "OFF";
        titulo += " Fill:";
        titulo += luzAtiva[1] ? "ON" : "OFF";
        titulo += " Back:";
        titulo += luzAtiva[2] ? "ON" : "OFF";
        glfwSetWindowTitle(window, titulo.c_str());

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        // Rotação automática do objeto em Y
        glm::mat4 model = glm::rotate(glm::mat4(1.0f),
                                      (float)glfwGetTime() * 0.5f,
                                      glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(glGetUniformLocation(shader, "model"),      1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"),       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shader, "viewPos"),          1, glm::value_ptr(cameraPos));

        // Passa as 3 luzes para o shader
        for (int i = 0; i < 3; i++) {
            string base = "lightPos["  + to_string(i) + "]";
            string basec = "lightColor[" + to_string(i) + "]";
            string baseo = "lightOn["  + to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(shader, base.c_str()),  1, glm::value_ptr(lightPos[i]));
            glUniform3fv(glGetUniformLocation(shader, basec.c_str()), 1, glm::value_ptr(lightColor[i]));
            glUniform1i(glGetUniformLocation(shader, baseo.c_str()),  luzAtiva[i] ? 1 : 0);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shader, "ourTexture"), 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_1) { luzAtiva[0] = !luzAtiva[0]; cout << "Key Light:  " << (luzAtiva[0] ? "ON" : "OFF") << endl; }
        if (key == GLFW_KEY_2) { luzAtiva[1] = !luzAtiva[1]; cout << "Fill Light: " << (luzAtiva[1] ? "ON" : "OFF") << endl; }
        if (key == GLFW_KEY_3) { luzAtiva[2] = !luzAtiva[2]; cout << "Back Light: " << (luzAtiva[2] ? "ON" : "OFF") << endl; }
    }
}

GLuint loadTexture(const string& filePath)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int w, h, c;
    unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &c, 0);
    if (data) {
        GLenum fmt = (c == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        cout << "Textura carregada: " << filePath << " (" << w << "x" << h << ", " << c << "ch)" << endl;
    } else {
        cout << "Erro ao carregar textura: " << filePath << endl;
    }

    return id;
}
