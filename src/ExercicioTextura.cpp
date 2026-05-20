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

GLuint loadTexture(string filePath);

const GLchar* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 position;
    layout (location = 1) in vec3 color;
    layout (location = 2) in vec2 texCoord;
    out vec3 ourColor;
    out vec2 TexCoord;
    uniform mat4 model, view, projection;
    void main() {
        gl_Position = projection * view * model * vec4(position, 1.0);
        ourColor = color;
        TexCoord = texCoord;
    }
)glsl";

const GLchar* fragmentShaderSource = R"glsl(
    #version 330 core
    in vec3 ourColor;
    in vec2 TexCoord;
    out vec4 color;
    uniform sampler2D ourTexture;
    void main() {
        color = texture(ourTexture, TexCoord); 
    }
)glsl";

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "Projeto Textura", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER); glShaderSource(vs, 1, &vertexShaderSource, NULL); glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fs, 1, &fragmentShaderSource, NULL); glCompileShader(fs);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);  
    glAttachShader(shaderProgram, fs); 
    glLinkProgram(shaderProgram);

    int nVertices;
    string texName;
    GLuint VAO = loadSimpleOBJ("C:/AtividadesCGPedro/assets/Modelos3D/Suzanne.obj", nVertices, texName);
    GLuint textureID = loadTexture("C:/AtividadesCGPedro/assets/Modelos3D/" + texName);

  glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(shaderProgram);

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

GLuint loadTexture(string filePath) {
    GLuint id; glGenTextures(1, &id); glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    stbi_set_flip_vertically_on_load(true);
    int w, h, c;
    unsigned char *data = stbi_load(filePath.c_str(), &w, &h, &c, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, (c==4?GL_RGBA:GL_RGB), w, h, 0, (c==4?GL_RGBA:GL_RGB), GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        cout << "Textura carregada: " << filePath << endl;
    } else cout << "Erro ao carregar: " << filePath << endl;
    return id;
}