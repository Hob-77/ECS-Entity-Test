
class Renderer {
public:
    GLuint shaderProgram;
    GLuint VAO, VBO, EBO, instanceVBO;

    const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aOffset;
layout (location = 2) in vec2 aScale;
layout (location = 3) in vec4 aColor;

out vec4 vertexColor;

uniform mat4 projection;

void main() {
   vec2 worldPos = aPos * aScale + aOffset;
   gl_Position = projection * vec4(worldPos, 0.0, 1.0);
   vertexColor = aColor;
})";

    const char* fragmentShaderSource = R"(
#version 330 core
in vec4 vertexColor;
out vec4 FragColor;

void main() {
   FragColor = vertexColor;
})";

    float projectionMatrix[16];

    float boxVertices[8] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };

    unsigned int indices[6] = {
        0, 1, 2,
        2, 3, 0
    };

    void Init(int width, int height) {
        // Compile shaders
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // Setup geometry
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glGenBuffers(1, &instanceVBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Orthographic projection
        projectionMatrix[0] = 2.0f / width;
        projectionMatrix[5] = -2.0f / height;
        projectionMatrix[10] = -1.0f;
        projectionMatrix[12] = -1.0f;
        projectionMatrix[13] = 1.0f;
        projectionMatrix[15] = 1.0f;

        glUseProgram(shaderProgram);
        GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix);
    }

    void DrawInstanced(float* positions, float* scales, float* colors, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, count * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

        glBufferSubData(GL_ARRAY_BUFFER, 0, count * 2 * sizeof(float), positions);
        glBufferSubData(GL_ARRAY_BUFFER, count * 2 * sizeof(float), count * 2 * sizeof(float), scales);
        glBufferSubData(GL_ARRAY_BUFFER, count * 4 * sizeof(float), count * 4 * sizeof(float), colors);

        glBindVertexArray(VAO);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glVertexAttribDivisor(1, 1);

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(count * 2 * sizeof(float)));
        glVertexAttribDivisor(2, 1);

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(count * 4 * sizeof(float)));
        glVertexAttribDivisor(3, 1);

        glUseProgram(shaderProgram);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, count);
    }
};