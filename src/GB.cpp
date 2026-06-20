/* GB — Projeto Final de Computação Gráfica - Pedro
 *  Controles 
 *  Câmera:
 *    W/A/S/D     Mover    |   Mouse   Olhar ao redor   |   Scroll  Zoom
 *
 *  Seleção:
 *    TAB         Alterna objeto selecionado (0 → 1 → 2 → ...)
 *
 *  Transformações (atuam no objeto selecionado):
 *    ← → ↑ ↓    Translação X/Z       I / K   Translação Y
 *    X / Y / Z   Rotação no eixo      + / -   Escala uniforme
 *
 *  Material/Textura:
 *    T           Alterna textura ↔ cor do material (Kd)
 *
 *  Iluminação:
 *    1 / 2 / 3   Liga/desliga Key / Fill / Back light
 *
 *  Animação (Bézier):
 *    P           Grava posição atual da câmera como ponto de controle
 *                do objeto selecionado
 *    Backspace   Remove último ponto do objeto selecionado
 *    Enter       Inicia / pausa animação de todos os objetos
 *    F5          Aumenta velocidade   F6  Diminui velocidade
 *
 *  ESC           Sair
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
#include "Bezier.hpp"

using namespace std;

const int   WIN_W = 1000, WIN_H = 750;
const int   NUM_OBJS = 3;

GLuint loadTexture(const string& path);
void   key_callback(GLFWwindow*, int, int, int, int);
void   mouse_callback(GLFWwindow*, double, double);
void   scroll_callback(GLFWwindow*, double, double);
void   processInput(GLFWwindow*, float dt);

Camera camera(glm::vec3(0.0f, 1.0f, 8.0f), -90.0f, -10.0f);
bool   firstMouse = true;
float  lastX = WIN_W / 2.0f, lastY = WIN_H / 2.0f;

int       objSelecionado  = 0;
glm::vec3 posicao[NUM_OBJS]  = { {-3,0,0}, {0,0,0}, {3,0,0} };
glm::vec3 rotacao[NUM_OBJS]  = { {0,0,0},  {0,0,0},  {0,0,0} };
float     escala[NUM_OBJS]   = { 1.0f, 1.0f, 1.0f };

bool modoTextura = true;

bool luzAtiva[3] = { true, true, true };

Bezier    bezier[NUM_OBJS];
bool      animacaoAtiva = false;

// Shaders 
const GLchar* vertexShaderSource = R"glsl(
#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texCoord;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 TexCoord;

uniform mat4 model, view, projection;

void main()
{
    vec4 wp   = model * vec4(position, 1.0);
    fragPos   = vec3(wp);
    fragNormal= mat3(transpose(inverse(model))) * normal;
    TexCoord  = texCoord;
    gl_Position = projection * view * wp;
}
)glsl";

// Fragment shader suporta dois modos: textura e cor do material (Kd)
const GLchar* fragmentShaderSource = R"glsl(
#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 TexCoord;
out vec4 outColor;

uniform sampler2D ourTexture;
uniform vec3  viewPos;

// Material
uniform vec3  Ka, Kd, Ks;
uniform float Ns;
uniform bool  usaTextura;  // true=textura, false=cor Kd

// 3 luzes pontuais
uniform vec3  lightPos[3];
uniform vec3  lightColor[3];
uniform bool  lightOn[3];

const float kC=1.0, kL=0.09, kQ=0.032;

void main()
{
    vec3 norm    = normalize(fragNormal);
    vec3 baseCol = usaTextura ? vec3(texture(ourTexture, TexCoord)) : Kd;
    vec3 result  = vec3(0.0);

    for (int i = 0; i < 3; i++) {
        if (!lightOn[i]) continue;

        // Ambiente
        vec3 ambient = Ka * lightColor[i];

        // Difuso com atenuação
        vec3  lDir  = lightPos[i] - fragPos;
        float dist  = length(lDir);
        lDir = normalize(lDir);
        float atten = 1.0 / (kC + kL*dist + kQ*dist*dist);
        float diff  = max(dot(norm, lDir), 0.0);
        vec3 diffuse = Kd * diff * lightColor[i] * atten;

        // Especular (Phong)
        vec3 viewDir = normalize(viewPos - fragPos);
        vec3 reflDir = reflect(-lDir, norm);
        float spec   = pow(max(dot(viewDir, reflDir), 0.0), max(Ns, 1.0));
        vec3 specular = Ks * spec * lightColor[i] * atten;

        result += (ambient + diffuse + specular) * baseCol;
    }

    outColor = vec4(result, 1.0);
}
)glsl";

// main 
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, "GB - Projeto Final", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, WIN_W, WIN_H);

    // --- Compilar shaders ---
    auto compileShader = [](GLenum type, const GLchar* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, NULL);
        glCompileShader(s);
        return s;
    };
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint shader = glCreateProgram();
    glAttachShader(shader, vs); glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs); glDeleteShader(fs);

    // Carregar OBJs (todos usam Suzanne) 
    // Objetos 0 e 2: Suzanne normal. Objeto 1: SuzanneSubdiv1 para variar.
    const string objPaths[NUM_OBJS] = {
        "../assets/Modelos3D/Suzanne.obj",
        "../assets/Modelos3D/SuzanneSubdiv1.obj",
        "../assets/Modelos3D/Suzanne.obj",
    };

    GLuint VAO[NUM_OBJS];
    int    nVerts[NUM_OBJS];
    GLuint texID[NUM_OBJS];
    Material mat[NUM_OBJS];

    for (int i = 0; i < NUM_OBJS; i++) {
        string texName;
        VAO[i]   = loadSimpleOBJ(objPaths[i], nVerts[i], texName, &mat[i]);
        texID[i] = texName.empty() ? 0
                 : loadTexture("../assets/Modelos3D/" + texName);
    }

    // Posição das luzes (3 pontos em relação ao centro da cena)
    glm::vec3 lightPos[3] = {
        glm::vec3( 4.0f,  3.0f,  4.0f),   // Key
        glm::vec3(-4.0f,  1.0f,  3.0f),   // Fill
        glm::vec3( 0.0f,  3.0f, -4.0f),   // Back
    };
    glm::vec3 lightColor[3] = {
        glm::vec3(1.0f,  1.0f,  1.0f) * 1.0f,
        glm::vec3(0.85f, 0.9f,  1.0f) * 0.5f,
        glm::vec3(0.7f,  0.8f,  1.0f) * 0.7f,
    };

    // Trajetórias Bézier pré-definidas
    // Obj 0: laço em XZ
    bezier[0].velocidade = 0.6f;
    bezier[0].adicionarPonto({-3.0f, 0.0f,  0.0f});
    bezier[0].adicionarPonto({-3.0f, 2.0f,  3.0f});
    bezier[0].adicionarPonto({ 3.0f, 2.0f,  3.0f});
    bezier[0].adicionarPonto({ 3.0f, 0.0f,  0.0f});
    bezier[0].adicionarPonto({ 3.0f, 0.0f, -3.0f});
    bezier[0].adicionarPonto({-3.0f, 0.0f, -3.0f});
    bezier[0].adicionarPonto({-3.0f, 0.0f,  0.0f});  // fecha o ciclo

    // Obj 1: arco vertical
    bezier[1].velocidade = 0.8f;
    bezier[1].adicionarPonto({ 0.0f, 0.0f,  2.0f});
    bezier[1].adicionarPonto({ 0.0f, 4.0f,  2.0f});
    bezier[1].adicionarPonto({ 0.0f, 4.0f, -2.0f});
    bezier[1].adicionarPonto({ 0.0f, 0.0f, -2.0f});

    // Obj 2: zig-zag em X
    bezier[2].velocidade = 1.0f;
    bezier[2].adicionarPonto({-4.0f, 0.0f,  0.0f});
    bezier[2].adicionarPonto({-1.0f, 2.0f,  0.0f});
    bezier[2].adicionarPonto({ 1.0f,-2.0f,  0.0f});
    bezier[2].adicionarPonto({ 4.0f, 0.0f,  0.0f});

    cout << "\n========= GB - Projeto Final =========" << endl;
    cout << "  TAB          Selecionar objeto" << endl;
    cout << "  ← → ↑ ↓      Translação X/Z   |  I/K  Translação Y" << endl; 
    cout << "  X / Y / Z    Rotação           |  +/-  Escala" << endl;
    cout << "  T            Textura / Material" << endl;
    cout << "  1 / 2 / 3    Key / Fill / Back light" << endl;
    cout << "  P            Add ponto Bézier na câmera" << endl;
    cout << "  Enter        Inicia/pausa animação" << endl;
    cout << "  F5/F6        Velocidade da animação" << endl;
    cout << "  W/A/S/D + Mouse + Scroll = câmera" << endl;
    cout << "=======================================" << endl;

    float deltaTime = 0.0f, lastFrame = 0.0f;
    glUseProgram(shader);

    // Projeção é fixa (atualizada só se o FOV mudar, feito no loop via camera.fov)
    glm::mat4 projection = glm::perspective(glm::radians(camera.fov),
                                            (float)WIN_W / WIN_H, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    while (!glfwWindowShouldClose(window))
    {
        float cur = (float)glfwGetTime();
        deltaTime = cur - lastFrame;
        lastFrame = cur;

        glfwPollEvents();
        processInput(window, deltaTime);

        // Atualiza animação
        if (animacaoAtiva) {
            for (int i = 0; i < NUM_OBJS; i++) {
                if (bezier[i].numSegmentos() >= 1) {
                    bezier[i].atualizar(deltaTime);
                    posicao[i] = bezier[i].getPosicaoAtual();
                }
            }
        }

        // Atualiza título
        string tit = "GB | Obj:" + to_string(objSelecionado) +
                     " | " + (modoTextura ? "Textura" : "Material") +
                     " | Anim:" + (animacaoAtiva ? "ON" : "OFF") +
                     " | Pontos Bezier:" + to_string(bezier[objSelecionado].numPontos());
        glfwSetWindowTitle(window, tit.c_str());

        glClearColor(0.07f, 0.07f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        // Atualiza câmera e projeção (o FOV pode mudar pelo scroll)
        glm::mat4 view = camera.getViewMatrix();
        projection = glm::perspective(glm::radians(camera.fov), (float)WIN_W/WIN_H, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"),       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shader, "viewPos"),          1, glm::value_ptr(camera.position));

        // Luzes
        for (int i = 0; i < 3; i++) {
            glUniform3fv(glGetUniformLocation(shader, ("lightPos["  +to_string(i)+"]").c_str()), 1, glm::value_ptr(lightPos[i]));
            glUniform3fv(glGetUniformLocation(shader, ("lightColor["+to_string(i)+"]").c_str()), 1, glm::value_ptr(lightColor[i]));
            glUniform1i( glGetUniformLocation(shader, ("lightOn["   +to_string(i)+"]").c_str()), luzAtiva[i]);
        }

        glUniform1i(glGetUniformLocation(shader, "usaTextura"), modoTextura ? 1 : 0);

        // Renderiza os objetos
        for (int i = 0; i < NUM_OBJS; i++)
        {
            // Material
            glUniform3fv(glGetUniformLocation(shader, "Ka"), 1, glm::value_ptr(mat[i].ka));
            glUniform3fv(glGetUniformLocation(shader, "Kd"), 1, glm::value_ptr(mat[i].kd));
            glUniform3fv(glGetUniformLocation(shader, "Ks"), 1, glm::value_ptr(mat[i].ks));
            glUniform1f( glGetUniformLocation(shader, "Ns"), mat[i].ns);

            // Textura
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texID[i]);
            glUniform1i(glGetUniformLocation(shader, "ourTexture"), 0);

            // Model matrix: TRS
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, posicao[i]);
            model = glm::rotate(model, rotacao[i].x, glm::vec3(1,0,0));
            model = glm::rotate(model, rotacao[i].y, glm::vec3(0,1,0));
            model = glm::rotate(model, rotacao[i].z, glm::vec3(0,0,1));
            model = glm::scale(model, glm::vec3(escala[i]));

            // Destaca o objeto selecionado com leve escala adicional
            if (i == objSelecionado)
                model = glm::scale(model, glm::vec3(1.05f));

            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(VAO[i]);
            glDrawArrays(GL_TRIANGLES, 0, nVerts[i]);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// Funções de callback 

void processInput(GLFWwindow* window, float dt)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.mover(0, dt);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.mover(1, dt);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.mover(2, dt);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.mover(3, dt);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Repetição aceita para transformações contínuas
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    const float pT = 0.15f;                    // passo translação
    const float pR = glm::radians(5.0f);       // passo rotação
    const float pE = 0.05f;                    // passo escala
    int s = objSelecionado;

    // Translação
    if (key == GLFW_KEY_LEFT)  posicao[s].x -= pT;
    if (key == GLFW_KEY_RIGHT) posicao[s].x += pT;
    if (key == GLFW_KEY_UP)    posicao[s].z -= pT;
    if (key == GLFW_KEY_DOWN)  posicao[s].z += pT;
    if (key == GLFW_KEY_I)     posicao[s].y += pT;
    if (key == GLFW_KEY_K)     posicao[s].y -= pT;

    // Rotação
    if (key == GLFW_KEY_X) rotacao[s].x += pR;
    if (key == GLFW_KEY_Y) rotacao[s].y += pR;
    if (key == GLFW_KEY_Z) rotacao[s].z += pR;

    // Escala
    if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
        escala[s] += pE;
    if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
        escala[s] = max(0.05f, escala[s] - pE);

    // Apenas PRESS a partir daqui
    if (action != GLFW_PRESS) return;

    // Selecionar objeto
    if (key == GLFW_KEY_TAB) {
        objSelecionado = (objSelecionado + 1) % NUM_OBJS;
        cout << "Obj selecionado: " << objSelecionado << endl;
    }

    // Textura / Material
    if (key == GLFW_KEY_T) {
        modoTextura = !modoTextura;
        cout << "Modo: " << (modoTextura ? "Textura" : "Material (Kd)") << endl;
    }

    // Luzes
    if (key == GLFW_KEY_1) { luzAtiva[0]=!luzAtiva[0]; cout<<"Key Light: " <<(luzAtiva[0]?"ON":"OFF")<<endl; }
    if (key == GLFW_KEY_2) { luzAtiva[1]=!luzAtiva[1]; cout<<"Fill Light: "<<(luzAtiva[1]?"ON":"OFF")<<endl; }
    if (key == GLFW_KEY_3) { luzAtiva[2]=!luzAtiva[2]; cout<<"Back Light: "<<(luzAtiva[2]?"ON":"OFF")<<endl; }

    // Bézier: adicionar ponto
    if (key == GLFW_KEY_P) {
        bezier[s].adicionarPonto(camera.position);
        cout << "Ponto Bezier Obj" << s << ": ("
             << camera.position.x << ", "
             << camera.position.y << ", "
             << camera.position.z << ")  total="
             << bezier[s].numPontos() << endl;
    }
    if (key == GLFW_KEY_BACKSPACE) {
        bezier[s].removerUltimoPonto();
        cout << "Ponto removido Obj" << s << "  total=" << bezier[s].numPontos() << endl;
    }

    // Animação
    if (key == GLFW_KEY_ENTER) {
        animacaoAtiva = !animacaoAtiva;
        cout << "Animação: " << (animacaoAtiva ? "ON" : "OFF") << endl;
    }
    if (key == GLFW_KEY_F5) {
        bezier[s].velocidade += 0.2f;
        cout << "Velocidade Obj" << s << ": " << bezier[s].velocidade << endl;
    }
    if (key == GLFW_KEY_F6) {
        bezier[s].velocidade = max(0.1f, bezier[s].velocidade - 0.2f);
        cout << "Velocidade Obj" << s << ": " << bezier[s].velocidade << endl;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) { lastX=(float)xpos; lastY=(float)ypos; firstMouse=false; }
    float xOff =  (float)xpos - lastX;
    float yOff =  lastY - (float)ypos;
    lastX = (float)xpos; lastY = (float)ypos;
    camera.rotacionar(xOff, yOff);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.zoom((float)yoffset);
}

GLuint loadTexture(const string& path)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_set_flip_vertically_on_load(true);
    int w, h, c;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 0);
    if (data) {
        GLenum fmt = (c==4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        cout << "Textura: " << path << " (" << w << "x" << h << ")" << endl;
    } else {
        cout << "Erro textura: " << path << endl;
    }
    return id;
}
