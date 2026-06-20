/* Visualizador de Cenários 3D - Pedro
 *
 * Controles de Interação:
 *   TAB       - Alterna o controle (Cubo 0 -> Cubo 1 -> Cubo 2 -> Malha OBJ -> Todos)
 *   W / S     - Move no eixo Z (Frente / Trás)
 *   A / D     - Move no eixo X (Esquerda / Direita)
 *   I / J     - Move no eixo Y (Cima / Baixo)
 *   ] / [     - Escala Uniforme (Aumentar / Diminuir)
 *   X / Y / Z - Rotação nos respectivos eixos
 *   ESC       - Sair
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// GLAD e GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM (Matemática para CG)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Constantes da Janela
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Número total de objetos: 3 cubos + 1 malha OBJ
const int NUM_OBJETOS = 4;
// modoSelecao: 0..3 = objeto individual, 4 = TODOS
const int MODO_TODOS = NUM_OBJETOS;

// Protótipos
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void atualizarTransformacoes(int indice, int tecla);
GLuint setupShader();
GLuint setupGeometry();
GLuint loadSimpleOBJ(const string& filePath, int& nVertices);

// Shaders básicos
const GLchar* vertexShaderSource =
    "#version 450\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 color;\n"
    "uniform mat4 model;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = model * vec4(position, 1.0);\n"
    "    finalColor = vec4(color, 1.0);\n"
    "}\0";

const GLchar* fragmentShaderSource =
    "#version 450\n"
    "in vec4 finalColor;\n"
    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    "    color = finalColor;\n"
    "}\n\0";

// --- VARIÁVEIS GLOBAIS DE ESTADO ---
int modoSelecao = 0;

// Índices 0-2: cubos, índice 3: malha OBJ
glm::vec3 posicoes[NUM_OBJETOS] = {
    glm::vec3(-0.65f,  0.0f, 0.0f),
    glm::vec3( 0.00f,  0.0f, 0.0f),
    glm::vec3( 0.65f,  0.0f, 0.0f),
    glm::vec3( 0.00f, -0.5f, 0.0f)  // malha OBJ abaixo dos cubos
};
float escalas[NUM_OBJETOS]     = { 0.3f, 0.3f, 0.3f, 0.3f };
glm::vec3 rotacoes[NUM_OBJETOS] = {
    glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
};

int nVerticesMalha = 0;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Visualizador 3D - Pedro", nullptr, nullptr);
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

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID  = setupShader();
    GLuint vaoCubo   = setupGeometry();
    GLuint vaoMalha  = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVerticesMalha);

    if (vaoMalha == 0) {
        cout << "Aviso: malha OBJ nao carregada. Verifique o caminho do arquivo." << endl;
    }

    glUseProgram(shaderID);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    // Loop Principal
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Título dinâmico
        string nomeSelecao;
        if      (modoSelecao == MODO_TODOS) nomeSelecao = "[TODOS]";
        else if (modoSelecao == 3)          nomeSelecao = "[Malha OBJ]";
        else                                nomeSelecao = "[Cubo " + to_string(modoSelecao) + "]";
        glfwSetWindowTitle(window, ("Projeto CG | " + nomeSelecao + " | TAB para trocar").c_str());

        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Renderiza os 3 cubos (VAO compartilhado, 36 vértices cada)
        glBindVertexArray(vaoCubo);
        for (int i = 0; i < 3; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, posicoes[i]);
            model = glm::rotate(model, rotacoes[i].x, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, rotacoes[i].y, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, rotacoes[i].z, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(escalas[i]));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Renderiza a malha OBJ (índice 3)
        if (vaoMalha != 0 && nVerticesMalha > 0)
        {
            glBindVertexArray(vaoMalha);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, posicoes[3]);
            model = glm::rotate(model, rotacoes[3].x, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, rotacoes[3].y, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, rotacoes[3].z, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(escalas[3]));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, nVerticesMalha);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &vaoCubo);
    if (vaoMalha != 0) glDeleteVertexArrays(1, &vaoMalha);
    glfwTerminate();
    return 0;
}

void atualizarTransformacoes(int indice, int tecla)
{
    float passoMover   = 0.1f;
    float passoEscala  = 0.05f;
    float passoRotacao = glm::radians(5.0f);

    if (tecla == GLFW_KEY_A) posicoes[indice].x -= passoMover;
    if (tecla == GLFW_KEY_D) posicoes[indice].x += passoMover;
    if (tecla == GLFW_KEY_W) posicoes[indice].z -= passoMover;
    if (tecla == GLFW_KEY_S) posicoes[indice].z += passoMover;
    if (tecla == GLFW_KEY_I) posicoes[indice].y += passoMover;
    if (tecla == GLFW_KEY_J) posicoes[indice].y -= passoMover;

    if (tecla == GLFW_KEY_RIGHT_BRACKET) escalas[indice] += passoEscala;
    if (tecla == GLFW_KEY_LEFT_BRACKET) {
        escalas[indice] -= passoEscala;
        if (escalas[indice] < 0.05f) escalas[indice] = 0.05f;
    }

    if (tecla == GLFW_KEY_X) rotacoes[indice].x += passoRotacao;
    if (tecla == GLFW_KEY_Y) rotacoes[indice].y += passoRotacao;
    if (tecla == GLFW_KEY_Z) rotacoes[indice].z += passoRotacao;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        modoSelecao++;
        if (modoSelecao > MODO_TODOS) modoSelecao = 0;
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (modoSelecao == MODO_TODOS) {
            for (int i = 0; i < NUM_OBJETOS; i++)
                atualizarTransformacoes(i, key);
        } else {
            atualizarTransformacoes(modoSelecao, key);
        }
    }
}

GLuint setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

GLuint setupGeometry()
{
    GLfloat vertices[] = {
        // Face Frontal (+Z) — Vermelho
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,

        // Face Traseira (-Z) — Verde
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,

        // Face Esquerda (-X) — Azul
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,

        // Face Direita (+X) — Amarelo
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

        // Face Superior (+Y) — Ciano
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,

        // Face Inferior (-Y) — Magenta
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

GLuint loadSimpleOBJ(const string& filePath, int& nVertices)
{
    vector<glm::vec3> positions;
    vector<GLfloat> vBuffer;
    glm::vec3 color(1.0f, 0.5f, 0.2f); // cor laranja para a malha OBJ

    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Erro: nao foi possivel abrir " << filePath << endl;
        nVertices = 0;
        return 0;
    }

    string line;
    while (getline(file, line)) {
        istringstream ss(line);
        string token;
        ss >> token;

        if (token == "v") {
            glm::vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (token == "f") {
            // Cada token de face pode ser: v, v/vt, v/vt/vn ou v//vn
            // Suporta faces triangulares e quads (fan triangulation)
            vector<int> faceIndices;
            string word;
            while (ss >> word) {
                int vi = stoi(word.substr(0, word.find('/'))) - 1;
                faceIndices.push_back(vi);
            }
            // Fan triangulation: (0,1,2), (0,2,3), ...
            for (int i = 1; i + 1 < (int)faceIndices.size(); i++) {
                int tris[3] = { faceIndices[0], faceIndices[i], faceIndices[i+1] };
                for (int vi : tris) {
                    vBuffer.push_back(positions[vi].x);
                    vBuffer.push_back(positions[vi].y);
                    vBuffer.push_back(positions[vi].z);
                    vBuffer.push_back(color.r);
                    vBuffer.push_back(color.g);
                    vBuffer.push_back(color.b);
                }
            }
        }
    }
    file.close();

    nVertices = (int)vBuffer.size() / 6;
    cout << "Malha OBJ carregada: " << filePath << " (" << nVertices << " vertices)" << endl;

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}
