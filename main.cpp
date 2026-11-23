// week09: камера моделі, lookAt, WASD, delta time
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

// ---------------- Камера айнымалылары (жаһандық, цикл сыртында) ----------------
const glm::vec3 CAM_START_POS = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 camPos   = CAM_START_POS;
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f, 0.0f);

// ---------------- Шейдерлер ----------------
const char* vertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 vColor;
void main() {
    vColor = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentSrc = R"(#version 330 core
in vec3 vColor;
uniform vec3 tint;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor * tint, 1.0);
}
)";

// Куб: 6 жақ * 2 үшбұрыш * 3 төбе. Әр төбе: x y z r g b. Тәртіп — CCW (culling үшін)
const float s = 0.5f;
float cubeVertices[] = {
    // алдыңғы (+z) — қызыл
    -s,-s, s, 1,0.2f,0.2f,   s,-s, s, 1,0.2f,0.2f,   s, s, s, 1,0.2f,0.2f,
     s, s, s, 1,0.2f,0.2f,  -s, s, s, 1,0.2f,0.2f,  -s,-s, s, 1,0.2f,0.2f,
    // артқы (-z) — жасыл
     s,-s,-s, 0.2f,1,0.2f,  -s,-s,-s, 0.2f,1,0.2f,  -s, s,-s, 0.2f,1,0.2f,
    -s, s,-s, 0.2f,1,0.2f,   s, s,-s, 0.2f,1,0.2f,   s,-s,-s, 0.2f,1,0.2f,
    // оң (+x) — көк
     s,-s, s, 0.2f,0.2f,1,   s,-s,-s, 0.2f,0.2f,1,   s, s,-s, 0.2f,0.2f,1,
     s, s,-s, 0.2f,0.2f,1,   s, s, s, 0.2f,0.2f,1,   s,-s, s, 0.2f,0.2f,1,
    // сол (-x) — сары
    -s,-s,-s, 1,1,0.2f,  -s,-s, s, 1,1,0.2f,  -s, s, s, 1,1,0.2f,
    -s, s, s, 1,1,0.2f,  -s, s,-s, 1,1,0.2f,  -s,-s,-s, 1,1,0.2f,
    // жоғарғы (+y) — көгілдір
    -s, s, s, 0.2f,1,1,   s, s, s, 0.2f,1,1,   s, s,-s, 0.2f,1,1,
     s, s,-s, 0.2f,1,1,  -s, s,-s, 0.2f,1,1,  -s, s, s, 0.2f,1,1,
    // төменгі (-y) — қызғылт
    -s,-s,-s, 1,0.2f,1,   s,-s,-s, 1,0.2f,1,   s,-s, s, 1,0.2f,1,
     s,-s, s, 1,0.2f,1,  -s,-s, s, 1,0.2f,1,  -s,-s,-s, 1,0.2f,1,
};

// Бес куб: z = 0-ден z = -4-ке дейін
glm::vec3 cubePositions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 1.8f,  0.6f, -1.0f),
    glm::vec3(-1.8f, -0.6f, -2.0f),
    glm::vec3( 1.5f, -1.0f, -3.0f),
    glm::vec3(-1.2f,  1.0f, -4.0f),
};
glm::vec3 cubeTints[] = {
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(0.9f, 0.8f, 0.7f),
    glm::vec3(0.7f, 0.9f, 0.8f),
    glm::vec3(0.8f, 0.7f, 0.9f),
    glm::vec3(1.0f, 0.9f, 0.6f),
};

void framebufferSizeCallback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

GLuint compileShader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(sh, 512, nullptr, log); std::cerr << "Shader error: " << log << "\n"; }
    return sh;
}

int main() {
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "week09: camera", nullptr, nullptr);
    if (!window) { std::cerr << "Window failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cerr << "GLAD failed\n"; return -1; }

    glEnable(GL_DEPTH_TEST);  // терендік буфері
    glEnable(GL_CULL_FACE);   // culling
    glCullFace(GL_BACK);

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLint modelLoc = glGetUniformLocation(prog, "model");
    GLint viewLoc  = glGetUniformLocation(prog, "view");
    GLint projLoc  = glGetUniformLocation(prog, "projection");
    GLint tintLoc  = glGetUniformLocation(prog, "tint");

    float lastTime = (float)glfwGetTime();
    bool pWasPressed = false;

    while (!glfwWindowShouldClose(window)) {
        // ---------- delta time ----------
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

        // ---------- пернетақта (dt есептелгеннен кейін, цикл ішінде) ----------
        float speed = 2.5f * dt;
        // 3-тапсырма: жүгіру режимі (dt-ге көбейтілген speed-ті екі есе арттырамыз)
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 2.0f;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camPos -= glm::normalize(glm::cross(camFront, camUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camPos += glm::normalize(glm::cross(camFront, camUp)) * speed;

        // 2-тапсырма: жоғары-төмен. Камераның төбесі бойымен (camUp) — ұшақ сияқты
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)      camPos += speed * camUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camPos -= speed * camUp;
        // Әлемнің Y осі бойымен нұсқа (FPS-тегі адам үшін):
        // camPos += speed * glm::vec3(0.0f, 1.0f, 0.0f);

        // Қосымша: R — бастапқы орынға қайту
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) camPos = CAM_START_POS;

        // Қосымша: P — координатты консольге шығару (бір басуға бір рет)
        bool pPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (pPressed && !pWasPressed)
            std::cout << camPos.x << ", " << camPos.y << ", " << camPos.z << "\n";
        pWasPressed = pPressed;

        // Қосымша: камера сахнадан тым алыс кетпесін
        if (glm::length(camPos) > 20.0f) camPos = glm::normalize(camPos) * 20.0f;

        // ---------- сурет салу ----------
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog);

        int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);
        float aspect = fbh > 0 ? (float)fbw / (float)fbh : 1.0f;
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

        // view цикл ІШІНДЕ, әр кадрда құрылады.
        // Дұрыс: camPos + camFront — мақсат нүктесі камерамен бірге жылжиды (камера ұшады).
        // Қате:  glm::vec3(0.0f) — камера әрқашан координат басына қарайды (орбита).
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(vao);
        for (int i = 0; i < 5; i++) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(tintLoc, 1, glm::value_ptr(cubeTints[i]));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);
    glfwTerminate();
    return 0;
}
