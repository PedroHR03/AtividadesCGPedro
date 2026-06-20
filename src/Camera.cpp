/* Câmera em Primeira Pessoa + Trajetórias - Pedro
 *
 * Dois objetos (Suzanne) em trajetórias cíclicas interpoladas por Catmull-Rom.
 * Câmera em primeira pessoa para navegar na cena.
 *
 * Controles de câmera:
 *   W / S       - Mover frente / trás
 *   A / D       - Mover esquerda / direita
 *   Mouse       - Rotacionar câmera
 *   Scroll      - Zoom
 *   ESC         - Sair
 *
 * Controles de trajetória:
 *   TAB         - Alterna objeto selecionado (Obj 0 / Obj 1)
 *   P           - Adiciona ponto de controle na posição atual da câmera
 *   Backspace   - Remove o último ponto adicionado ao objeto selecionado
 *   Enter       - Inicia / pausa o movimento dos objetos
 *   +           - Aumenta velocidade do objeto selecionado
 *   -           - Diminui velocidade do objeto selecionado
 *
 * Controles de luz:
 *   1 / 2 / 3   - Liga/desliga Key / Fill / Back light
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
#include "Camera.hpp"
#include "Trajetoria.hpp"

using namespace std;

// --- Protótipos ---
GLuint loadTexture(const string& filePath);
void   key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void   mouse_callback(GLFWwindow* window, double xpos, double ypos);
void   scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void   processInput(GLFWwindow* window, float deltaTime);

// --- Câmera ---
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
bool  firstMouse = true;
float lastX = 400.0f, lastY = 300.0f;

// --- Trajetórias (uma por objeto) ---
const int NUM_OBJS = 2;
Trajetoria trajetoria[NUM_OBJS];
glm::vec3  posicaoObj[NUM_OBJS]; // posição corrente de cada objeto
int        objSelecionado = 0;
bool       movimentoAtivo = false;

// --- Posições iniciais (fallback quando não há pontos suficientes) ---
const glm::vec3 posInicial[NUM_OBJS] = {
    glm::vec3(0.0f, 0.0f,  4.0f),
    glm::vec3(0.0f, 0.0f, -4.0f),
};

// --- Luzes ---
bool luzAtiva[3] = { true, true, true };

// ---------------------------------------------------------------------------
// Shaders
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
    vec4 worldPos = model * vec4(position, 1.0);
    fragPos       = vec3(worldPos);
    fragNormal    = mat3(transpose(inverse(model))) * normal;
    TexCoord      = texCoord;
    gl_Position   = projection * view * worldPos;
}
)glsl";

const GLchar* fragmentShaderSource = R"glsl(
#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 TexCoord;

out vec4 outColor;

uniform sampler2D ourTexture;
uniform vec3 viewPos;
uniform vec3  lightPos[3];
uniform vec3  lightColor[3];
uniform bool  lightOn[3];

const float ambientStrength  = 0.08;
const float specularStrength = 0.4;
const float shininess        = 32.0;
const float kC = 1.0, kL = 0.09, kQ = 0.032;

void main()
{
    vec3 norm   = normalize(fragNormal);
    vec3 texCol = vec3(texture(ourTexture, TexCoord));
    vec3 result = vec3(0.0);

    for (int i = 0; i < 3; i++) {
        if (!lightOn[i]) continue;
        vec3  lDir    = lightPos[i] - fragPos;
        float dist    = length(lDir);
        lDir          = normalize(lDir);
        float atten   = 1.0 / (kC + kL * dist + kQ * dist * dist);
        vec3 ambient  = ambientStrength * lightColor[i];
        float diff    = max(dot(norm, lDir), 0.0);
        vec3 diffuse  = diff * lightColor[i] * atten;
        vec3 viewDir  = normalize(viewPos - fragPos);
        vec3 reflDir  = reflect(-lDir, norm);
        float spec    = pow(max(dot(viewDir, reflDir), 0.0), shininess);
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

    GLFWwindow* window = glfwCreateWindow(800, 600, "Trajetórias Catmull-Rom", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    // --- Shaders ---
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL); glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL); glCompileShader(fs);
    GLuint shader = glCreateProgram();
    glAttachShader(shader, vs); glAttachShader(shader, fs); glLinkProgram(shader);
    glDeleteShader(vs); glDeleteShader(fs);

    // --- OBJ + textura ---
    int nVertices = 0;
    string texName;
    GLuint VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVertices, texName);
    GLuint textureID = 0;
    if (!texName.empty())
        textureID = loadTexture("../assets/Modelos3D/" + texName);

    // --- Iluminação de 3 pontos ---
    glm::vec3 lightPos[3] = {
        glm::vec3( 3.0f,  2.4f,  0.0f),
        glm::vec3(-3.0f,  0.4f,  1.0f),
        glm::vec3( 0.0f,  2.0f, -2.0f),
    };
    glm::vec3 lightColor[3] = {
        glm::vec3(1.0f,  1.0f,  1.0f) * 1.0f,
        glm::vec3(0.85f, 0.9f,  1.0f) * 0.5f,
        glm::vec3(0.7f,  0.8f,  1.0f) * 0.7f,
    };

    // --- Trajetórias pré-definidas ---
    // Objeto 0: órbita circular no plano XZ
    trajetoria[0].velocidade = 0.8f;
    for (int i = 0; i < 8; i++) {
        float a = glm::radians(i * 45.0f);
        trajetoria[0].adicionarPonto({ 3.5f * cos(a), 0.0f, 4.0f + 3.5f * sin(a) });
    }

    // Objeto 1: zig-zag em X
    trajetoria[1].velocidade = 1.0f;
    trajetoria[1].adicionarPonto({ -2.0f, 0.0f, -4.0f });
    trajetoria[1].adicionarPonto({  0.0f, 1.5f, -4.0f });
    trajetoria[1].adicionarPonto({  2.0f, 0.0f, -4.0f });
    trajetoria[1].adicionarPonto({  0.0f,-1.5f, -4.0f });

    // Inicializa posições
    for (int i = 0; i < NUM_OBJS; i++)
        posicaoObj[i] = posInicial[i];

    cout << "\n=== Controles de Trajetória ===" << endl;
    cout << "  TAB        - Alterna objeto selecionado" << endl;
    cout << "  P          - Adiciona ponto na posição da câmera" << endl;
    cout << "  Backspace  - Remove último ponto do objeto selecionado" << endl;
    cout << "  Enter      - Inicia / pausa movimento" << endl;
    cout << "  + / -      - Velocidade do objeto selecionado" << endl;
    cout << "=== Câmera ===" << endl;
    cout << "  W/A/S/D  Mouse  Scroll" << endl;

    float deltaTime = 0.0f, lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime  = currentFrame - lastFrame;
        lastFrame  = currentFrame;

        glfwPollEvents();
        processInput(window, deltaTime);

        // Atualiza trajetórias
        if (movimentoAtivo) {
            for (int i = 0; i < NUM_OBJS; i++) {
                if (trajetoria[i].numPontos() >= 2) {
                    trajetoria[i].atualizar(deltaTime);
                    posicaoObj[i] = trajetoria[i].getPosicaoAtual();
                }
            }
        }

        // Título dinâmico
        string titulo = "Obj selecionado: " + to_string(objSelecionado) +
                        " | Pontos: " + to_string(trajetoria[objSelecionado].numPontos()) +
                        " | Movimento: " + (movimentoAtivo ? "ON" : "OFF [Enter]") +
                        " | P = add ponto";
        glfwSetWindowTitle(window, titulo.c_str());

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 view       = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.fov), 800.0f/600.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shader, "view"),       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shader, "viewPos"),          1, glm::value_ptr(camera.position));

        for (int i = 0; i < 3; i++) {
            glUniform3fv(glGetUniformLocation(shader, ("lightPos["  +to_string(i)+"]").c_str()), 1, glm::value_ptr(lightPos[i]));
            glUniform3fv(glGetUniformLocation(shader, ("lightColor["+to_string(i)+"]").c_str()), 1, glm::value_ptr(lightColor[i]));
            glUniform1i(glGetUniformLocation(shader,  ("lightOn["   +to_string(i)+"]").c_str()), luzAtiva[i] ? 1 : 0);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shader, "ourTexture"), 0);

        glBindVertexArray(VAO);

        // Obj 0 — olha para o centro
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, posicaoObj[0]);
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, nVertices);
        }
        // Obj 1 — virado 180° (frente a frente com o 0)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, posicaoObj[1]);
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, nVertices);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.mover(0, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.mover(1, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.mover(2, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.mover(3, deltaTime);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action != GLFW_PRESS) return;

    // Alterna objeto selecionado
    if (key == GLFW_KEY_TAB) {
        objSelecionado = (objSelecionado + 1) % NUM_OBJS;
        cout << "Objeto selecionado: " << objSelecionado << endl;
    }

    // Adiciona ponto na posição atual da câmera
    if (key == GLFW_KEY_P) {
        trajetoria[objSelecionado].adicionarPonto(camera.position);
        cout << "Ponto adicionado ao Obj " << objSelecionado
             << ": (" << camera.position.x << ", "
             << camera.position.y << ", "
             << camera.position.z << ")"
             << " | Total: " << trajetoria[objSelecionado].numPontos() << endl;
    }

    // Remove último ponto
    if (key == GLFW_KEY_BACKSPACE) {
        trajetoria[objSelecionado].removerUltimoPonto();
        cout << "Ponto removido do Obj " << objSelecionado
             << " | Total: " << trajetoria[objSelecionado].numPontos() << endl;
    }

    // Inicia/pausa movimento
    if (key == GLFW_KEY_ENTER) {
        movimentoAtivo = !movimentoAtivo;
        cout << "Movimento: " << (movimentoAtivo ? "ON" : "OFF") << endl;
    }

    // Velocidade
    if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) {
        trajetoria[objSelecionado].velocidade += 0.2f;
        cout << "Velocidade Obj " << objSelecionado << ": " << trajetoria[objSelecionado].velocidade << endl;
    }
    if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) {
        trajetoria[objSelecionado].velocidade = max(0.1f, trajetoria[objSelecionado].velocidade - 0.2f);
        cout << "Velocidade Obj " << objSelecionado << ": " << trajetoria[objSelecionado].velocidade << endl;
    }

    // Luzes
    if (key == GLFW_KEY_1) { luzAtiva[0] = !luzAtiva[0]; cout << "Key Light: "  << (luzAtiva[0]?"ON":"OFF") << endl; }
    if (key == GLFW_KEY_2) { luzAtiva[1] = !luzAtiva[1]; cout << "Fill Light: " << (luzAtiva[1]?"ON":"OFF") << endl; }
    if (key == GLFW_KEY_3) { luzAtiva[2] = !luzAtiva[2]; cout << "Back Light: " << (luzAtiva[2]?"ON":"OFF") << endl; }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    float xOffset =  (float)xpos - lastX;
    float yOffset =  lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;
    camera.rotacionar(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.zoom((float)yoffset);
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
        cout << "Textura: " << filePath << endl;
    } else {
        cout << "Erro ao carregar textura: " << filePath << endl;
    }
    return id;
}
