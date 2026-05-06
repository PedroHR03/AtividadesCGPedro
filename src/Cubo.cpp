/* Visualizador de Cenários 3D - Pedro
 *
 * Controles de Interação:
 *   TAB       - Alterna o controle (Cubo 0 -> Cubo 1 -> Cubo 2 -> Todos os Cubos)
 *   W / S     - Move no eixo Y (Cima / Baixo)
 *   A / D     - Move no eixo X (Esquerda / Direita)
 *   + / -     - Escala Uniforme (Aumentar / Diminuir)
 *   X / Y / Z - Rotação nos respectivos eixos
 *   ESC       - Sair
 */

#include <iostream>
#include <string>

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

// Protótipos
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void atualizarTransformacoes(int indiceCubo, int tecla);
int setupShader();
int setupGeometry();

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
// 0, 1 ou 2 controlam um cubo específico. 3 controla todos ao mesmo tempo.
int modoSelecao = 0; 

// Arrays paralelos armazenando as propriedades independentes de cada um dos 3 cubos
glm::vec3 posicoes[3] = { 
    glm::vec3(-0.65f, 0.0f, 0.0f), 
    glm::vec3( 0.00f, 0.0f, 0.0f), 
    glm::vec3( 0.65f, 0.0f, 0.0f) 
};
float escalas[3] = { 0.3f, 0.3f, 0.3f };
glm::vec3 rotacoes[3] = { 
    glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f) 
};

int main()
{
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Visualizador 3D - Pedro", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    GLuint VAO = setupGeometry();
    glUseProgram(shaderID);
    
    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    // Loop Principal
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Atualiza o título da janela dinamicamente para mostrar o modo atual
        string textoSelecao = (modoSelecao == 3) ? "[TODOS]" : "[Cubo " + to_string(modoSelecao) + "]";
        string titulo = "Projeto CG | Selecionado: " + textoSelecao + " | Pressione TAB para trocar";
        glfwSetWindowTitle(window, titulo.c_str());

        // Cor de fundo levemente alterada para destacar os objetos
        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(VAO);

        // Renderiza os 3 cubos
        for (int i = 0; i < 3; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);

            // 1. Translação (Move para a posição do mundo)
            model = glm::translate(model, posicoes[i]);

            // 2. Rotação (Aplica a rotação nos 3 eixos para este cubo específico)
            model = glm::rotate(model, rotacoes[i].x, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, rotacoes[i].y, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, rotacoes[i].z, glm::vec3(0.0f, 0.0f, 1.0f));

            // 3. Escala Uniforme
            model = glm::scale(model, glm::vec3(escalas[i]));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

// Função auxiliar para aplicar transformações ao cubo correto
void atualizarTransformacoes(int indiceCubo, int tecla) {
    float passoMover = 0.1f;
    float passoEscala = 0.05f;
    float passoRotacao = glm::radians(5.0f);

    // Movimentação XY
    if (tecla == GLFW_KEY_A) posicoes[indiceCubo].x -= passoMover;
    if (tecla == GLFW_KEY_D) posicoes[indiceCubo].x += passoMover;
    if (tecla == GLFW_KEY_W) posicoes[indiceCubo].y += passoMover;
    if (tecla == GLFW_KEY_S) posicoes[indiceCubo].y -= passoMover;

    // Escala (Teclas =/+ e -/_)
    if (tecla == GLFW_KEY_EQUAL || tecla == GLFW_KEY_KP_ADD) escalas[indiceCubo] += passoEscala;
    if (tecla == GLFW_KEY_MINUS || tecla == GLFW_KEY_KP_SUBTRACT) {
        escalas[indiceCubo] -= passoEscala;
        if (escalas[indiceCubo] < 0.05f) escalas[indiceCubo] = 0.05f; // Limite mínimo
    }

    // Rotação
    if (tecla == GLFW_KEY_X) rotacoes[indiceCubo].x += passoRotacao;
    if (tecla == GLFW_KEY_Y) rotacoes[indiceCubo].y += passoRotacao;
    if (tecla == GLFW_KEY_Z) rotacoes[indiceCubo].z += passoRotacao;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Lógica da Tecla TAB (Cicla entre 0, 1, 2 e 3)
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        modoSelecao++;
        if (modoSelecao > 3) modoSelecao = 0;
    }

    // Lógica de Movimento/Rotação/Escala (Aceita manter a tecla pressionada)
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (modoSelecao == 3) {
            // Se o modo for 3 (TODOS), aplica a alteração nos índices 0, 1 e 2
            for (int i = 0; i < 3; i++) {
                atualizarTransformacoes(i, key);
            }
        } else {
            // Aplica a alteração apenas no cubo selecionado
            atualizarTransformacoes(modoSelecao, key);
        }
    }
}

int setupShader()
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

int setupGeometry()
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
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}