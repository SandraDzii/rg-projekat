#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void renderQuad(float tex);
void renderGlass();

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct ProgramState {
    bool ImGuiEnabled = false;
    Camera camera;
    bool dlight = false;
    bool slight = false;
    bool light1 = false;
    bool light2_1 = false;
    bool light2_2 = false;
    bool light3 = false;
    bool light4 = false;
    bool light5 = false;
    bool CameraMouseMovementUpdateEnabled = true;

    ProgramState()
            : camera(glm::vec3(0.0f, 5.0f, 15.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        << dlight << '\n'
        << slight << '\n'
        << light1 << '\n'
        << light2_1 << '\n'
        << light2_2 << '\n'
        << light3 << '\n'
        << light4 << '\n'
        << light5 << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> dlight
           >> slight
           >> light1
           >> light2_1
           >> light2_2
           >> light3
           >> light4
           >> light5;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
    };
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));


    // configure MSAA framebuffer
    // --------------------------
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a multisampled color attachment texture
    unsigned int textureColorBufferMultiSampled;
    glGenTextures(1, &textureColorBufferMultiSampled);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);
    // create a (also multisampled) renderbuffer object for depth and stencil attachments
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;

    // shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader wallShader("resources/shaders/4.normal_mapping.vs", "resources/shaders/4.normal_mapping.fs");
    Shader glassShader("resources/shaders/glass.vs", "resources/shaders/glass.fs");
    Shader lightShader("resources/shaders/light.vs", "resources/shaders/light.fs");
    Shader screenShader("resources/shaders/screen.vs", "resources/shaders/screen.fs");

    // models
    // -----------
    stbi_set_flip_vertically_on_load(false);
    Model desk("resources/objects/desk/desk.obj");
    desk.SetShaderTextureNamePrefix("material.");
    Model glass("resources/objects/glass/glass.obj");
    glass.SetShaderTextureNamePrefix("material.");
    Model chair("resources/objects/chair/Patchwork chair.obj");
    chair.SetShaderTextureNamePrefix("material.");
    Model table("resources/objects/table/table.obj");
    table.SetShaderTextureNamePrefix("material.");
    Model table1("resources/objects/table1/table1.obj");
    table1.SetShaderTextureNamePrefix("material.");
    Model couch("resources/objects/couch/couch.obj");
    couch.SetShaderTextureNamePrefix("material.");
    Model laptop("resources/objects/laptop/laptop.obj");
    laptop.SetShaderTextureNamePrefix("material.");
    Model plant("resources/objects/plant/plant.obj");
    plant.SetShaderTextureNamePrefix("material.");
    Model plant1("resources/objects/plant1/plant1.obj");
    plant1.SetShaderTextureNamePrefix("material.");
    Model apples("resources/objects/apples/apples.obj");
    apples.SetShaderTextureNamePrefix("material.");
    Model bowl("resources/objects/bowl/bowl.obj");
    bowl.SetShaderTextureNamePrefix("material.");
    Model light1("resources/objects/light/light1.obj");
    light1.SetShaderTextureNamePrefix("material.");
    Model light2("resources/objects/light/light2.obj");
    light2.SetShaderTextureNamePrefix("material.");
    Model light3("resources/objects/light/light3.obj");
    light3.SetShaderTextureNamePrefix("material.");
    Model light4("resources/objects/light/light4.obj");
    light4.SetShaderTextureNamePrefix("material.");
    Model light5("resources/objects/light/light5.obj");
    light5.SetShaderTextureNamePrefix("material.");

    // texture Cube
    unsigned int diffuseMapWall = TextureFromFile("Stone_d.png", "resources/textures");
    unsigned int specularMapWall = TextureFromFile("Stone_s.png", "resources/textures");
    unsigned int normalMapWall = TextureFromFile("Stone_n.png", "resources/textures");
    unsigned int diffuseMapTop = TextureFromFile("White_d.png", "resources/textures");
    unsigned int specularMapTop = TextureFromFile("White_s.png", "resources/textures");
    unsigned int normalMapTop =  TextureFromFile("White_n.png", "resources/textures");
    unsigned int diffuseMapBottom = TextureFromFile("w_d.png", "resources/textures");
    unsigned int specularMapBottom = TextureFromFile("w_s.png", "resources/textures");
    unsigned int normalMapBottom = TextureFromFile("w_n.png", "resources/textures");
    unsigned int glassTexture = TextureFromFile("glass.png", "resources/textures");
    ourShader.use();
    ourShader.setInt("material.texture_diffuse1", 0);
    ourShader.setInt("material.texture_specular1", 1);
    wallShader.use();
    wallShader.setInt("material.texture_diffuse1", 0);
    wallShader.setInt("material.texture_specular1", 1);
    wallShader.setInt("material.texture_normal1", 2);
    glassShader.use();
    glassShader.setInt("texture1", 0);
    lightShader.use();
    lightShader.setInt("texture_diffuse1", 0);
    screenShader.use();
    screenShader.setInt("screenTexture", 0);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        float a = glm::distance(glm::vec3(-4.325f, 1.665f, 3.235f), programState->camera.Position);
        float b = glm::distance(glm::vec3(-1.0f, 2.77f, -4.0f), programState->camera.Position);
        bool distance = a > b;
        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. draw scene as normal in multisampled buffers
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);


        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("viewPos", programState->camera.Position);
        ourShader.setFloat("material.shininess", 128.0f);
        wallShader.use();
        wallShader.setMat4("projection", projection);
        wallShader.setMat4("view", view);
        wallShader.setVec3("viewPos", programState->camera.Position);
        wallShader.setFloat("material.shininess", 128.0f);

        // directional light
        if(programState->dlight) {
            ourShader.use();
            ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
            ourShader.setVec3("dirLight.ambient", 0.12f, 0.12f, 0.12f);
            ourShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
            ourShader.setVec3("dirLight.specular", 0.3f, 0.3f, 0.3f);
            wallShader.use();
            wallShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
            wallShader.setVec3("dirLight.ambient", 0.12f, 0.12f, 0.12f);
            wallShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
            wallShader.setVec3("dirLight.specular", 0.3f, 0.3f, 0.3f);
        } else {
            ourShader.use();
            ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
            ourShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
            ourShader.setVec3("dirLight.diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("dirLight.specular", 0.0f, 0.0f, 0.0f);
            wallShader.use();
            wallShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
            wallShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
            wallShader.setVec3("dirLight.diffuse", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("dirLight.specular", 0.0f, 0.0f, 0.0f);
        }
        // point light
        if (programState->light1) {
            ourShader.use();
            ourShader.setVec3("pointLights[0].position", -0.25f, 10.3f, 0.0f);
            ourShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
            ourShader.setVec3("pointLights[0].diffuse", 0.4f, 0.4f, 0.4f);
            ourShader.setVec3("pointLights[0].specular", 0.5f, 0.5f, 0.5f);
            ourShader.setFloat("pointLights[0].constant", 1.0f);
            ourShader.setFloat("pointLights[0].linear", 0.01f);
            ourShader.setFloat("pointLights[0].quadratic", 0.001f);

            ourShader.setVec3("pointLights[1].position", -2.075f, 10.3f, 0.0f);
            ourShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
            ourShader.setVec3("pointLights[1].diffuse", 0.4f, 0.4f, 0.4f);
            ourShader.setVec3("pointLights[1].specular", 0.5f, 0.5f, 0.5f);
            ourShader.setFloat("pointLights[1].constant", 1.0f);
            ourShader.setFloat("pointLights[1].linear", 0.01f);
            ourShader.setFloat("pointLights[1].quadratic", 0.001f);

            ourShader.setVec3("pointLights[2].position", 1.535f, 10.3f, 0.0f);
            ourShader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
            ourShader.setVec3("pointLights[2].diffuse", 0.4f, 0.4f, 0.4f);
            ourShader.setVec3("pointLights[2].specular", 0.5f, 0.5f, 0.5f);
            ourShader.setFloat("pointLights[2].constant", 1.0f);
            ourShader.setFloat("pointLights[2].linear", 0.01f);
            ourShader.setFloat("pointLights[2].quadratic", 0.001f);

            wallShader.use();
            wallShader.setVec3("lightPos[0]", -0.25f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[0].position", -0.25f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
            wallShader.setVec3("pointLights[0].diffuse", 0.4f, 0.4f, 0.4f);
            wallShader.setVec3("pointLights[0].specular", 0.5f, 0.5f, 0.5f);
            wallShader.setFloat("pointLights[0].constant", 1.0f);
            wallShader.setFloat("pointLights[0].linear", 0.01f);
            wallShader.setFloat("pointLights[0].quadratic", 0.001f);

            wallShader.setVec3("lightPos[1]", -2.075f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[1].position", -2.075f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
            wallShader.setVec3("pointLights[1].diffuse", 0.4f, 0.4f, 0.4f);
            wallShader.setVec3("pointLights[1].specular", 0.5f, 0.5f, 0.5f);
            wallShader.setFloat("pointLights[1].constant", 1.0f);
            wallShader.setFloat("pointLights[1].linear", 0.01f);
            wallShader.setFloat("pointLights[1].quadratic", 0.001f);

            wallShader.setVec3("lightPos[2]", 1.535f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[2].position", 1.535f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
            wallShader.setVec3("pointLights[2].diffuse", 0.4f, 0.4f, 0.4f);
            wallShader.setVec3("pointLights[2].specular", 0.5f, 0.5f, 0.5f);
            wallShader.setFloat("pointLights[2].constant", 1.0f);
            wallShader.setFloat("pointLights[2].linear", 0.01f);
            wallShader.setFloat("pointLights[2].quadratic", 0.001f);

        } else {
            ourShader.use();
            ourShader.setVec3("pointLights[0].position", -0.25f, 10.3f, 0.0f);
            ourShader.setVec3("pointLights[0].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[0].diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[0].specular", 0.0f, 0.0f, 0.0f);
            ourShader.setFloat("pointLights[0].constant", 1.0f);
            ourShader.setFloat("pointLights[0].linear", 0.01f);
            ourShader.setFloat("pointLights[0].quadratic", 0.001f);

            ourShader.setVec3("pointLights[1].position", -2.075f, 10.3f, 0.0f);
            ourShader.setVec3("pointLights[1].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[1].diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[1].specular", 0.0f, 0.0f, 0.0f);
            ourShader.setFloat("pointLights[1].constant", 1.0f);
            ourShader.setFloat("pointLights[1].linear", 0.01f);
            ourShader.setFloat("pointLights[1].quadratic", 0.001f);

            ourShader.setVec3("pointLights[2].position", 1.535f, 10.3f, 0.0f);
            ourShader.setVec3("pointLights[2].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[2].diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[2].specular", 0.0f, 0.0f, 0.0f);
            ourShader.setFloat("pointLights[2].constant", 1.0f);
            ourShader.setFloat("pointLights[2].linear", 0.01f);
            ourShader.setFloat("pointLights[2].quadratic", 0.001f);

            wallShader.use();
            wallShader.setVec3("lightPos[0]", -0.25f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[0].position", -0.25f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[0].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[0].diffuse", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[0].specular", 0.0f, 0.0f, 0.0f);
            wallShader.setFloat("pointLights[0].constant", 1.0f);
            wallShader.setFloat("pointLights[0].linear", 0.01f);
            wallShader.setFloat("pointLights[0].quadratic", 0.001f);

            wallShader.setVec3("lightPos[1]", -2.075f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[1].position", -2.075f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[1].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[1].diffuse", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[1].specular", 0.0f, 0.0f, 0.0f);
            wallShader.setFloat("pointLights[1].constant", 1.0f);
            wallShader.setFloat("pointLights[1].linear", 0.01f);
            wallShader.setFloat("pointLights[1].quadratic", 0.001f);

            wallShader.setVec3("lightPos[2]", 1.535f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[2].position", 1.535f, 10.3f, 0.0f);
            wallShader.setVec3("pointLights[2].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[2].diffuse", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[2].specular", 0.0f, 0.0f, 0.0f);
            wallShader.setFloat("pointLights[2].constant", 1.0f);
            wallShader.setFloat("pointLights[2].linear", 0.01f);
            wallShader.setFloat("pointLights[2].quadratic", 0.001f);
        }

        if (programState->light2_1) {
            ourShader.use();
            ourShader.setVec3("pointLights[3].position", 2.55f, 5.75f, -5.6f);
            ourShader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
            ourShader.setVec3("pointLights[3].diffuse", 0.4f, 0.4f, 0.4f);
            ourShader.setVec3("pointLights[3].specular", 0.6f, 0.6f, 0.6f);
            ourShader.setFloat("pointLights[3].constant", 1.0f);
            ourShader.setFloat("pointLights[3].linear", 0.03f);
            ourShader.setFloat("pointLights[3].quadratic", 0.016f);

            wallShader.use();
            wallShader.setVec3("lightPos[3]", 2.55f, 5.75f, -5.6f);
            wallShader.setVec3("pointLights[3].position", 2.55f, 5.75f, -5.6f);
            wallShader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
            wallShader.setVec3("pointLights[3].diffuse", 0.4f, 0.4f, 0.4f);
            wallShader.setVec3("pointLights[3].specular", 0.5f, 0.5f, 0.5f);
            wallShader.setFloat("pointLights[3].constant", 1.0f);
            wallShader.setFloat("pointLights[3].linear", 0.03f);
            wallShader.setFloat("pointLights[3].quadratic", 0.016f);
        } else {
            ourShader.use();
            ourShader.setVec3("pointLights[3].position", 2.55f, 5.75f, -5.6f);
            ourShader.setVec3("pointLights[3].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[3].diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[3].specular", 0.0f, 0.0f, 0.0f);
            ourShader.setFloat("pointLights[3].constant", 1.0f);
            ourShader.setFloat("pointLights[3].linear", 0.03f);
            ourShader.setFloat("pointLights[3].quadratic", 0.016f);

            wallShader.use();
            wallShader.setVec3("lightPos[3]", 2.55f, 5.75f, -5.6f);
            wallShader.setVec3("pointLights[3].position", 2.55f, 5.75f, -5.6f);
            wallShader.setVec3("pointLights[3].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[3].diffuse", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[3].specular", 0.0f, 0.0f, 0.0f);
            wallShader.setFloat("pointLights[3].constant", 1.0f);
            wallShader.setFloat("pointLights[3].linear", 0.03f);
            wallShader.setFloat("pointLights[3].quadratic", 0.016f);
        }

        if (programState->light2_2) {
            ourShader.use();
            ourShader.setVec3("pointLights[4].position", -3.05f, 5.75f, -5.6f);
            ourShader.setVec3("pointLights[4].ambient", 0.05f, 0.05f, 0.05f);
            ourShader.setVec3("pointLights[4].diffuse", 0.4f, 0.4f, 0.4f);
            ourShader.setVec3("pointLights[4].specular", 0.6f, 0.6f, 0.6f);
            ourShader.setFloat("pointLights[4].constant", 1.0f);
            ourShader.setFloat("pointLights[4].linear", 0.03f);
            ourShader.setFloat("pointLights[4].quadratic", 0.016f);

            wallShader.use();
            wallShader.setVec3("lightPos[4]", -3.05f, 5.75f, -5.6f);
            wallShader.setVec3("pointLights[4].position", -3.05f, 5.75f, -5.6f);
            wallShader.setVec3("pointLights[4].ambient", 0.05f, 0.05f, 0.05f);
            wallShader.setVec3("pointLights[4].diffuse", 0.4f, 0.4f, 0.4f);
            wallShader.setVec3("pointLights[4].specular", 0.5f, 0.5f, 0.5f);
            wallShader.setFloat("pointLights[4].constant", 1.0f);
            wallShader.setFloat("pointLights[4].linear", 0.03f);
            wallShader.setFloat("pointLights[4].quadratic", 0.016f);
        } else {
            ourShader.use();
            ourShader.setVec3("pointLights[4].position", -3.05f, 5.75f, -5.6f);
            ourShader.setVec3("pointLights[4].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[4].diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[4].specular", 0.0f, 0.0f, 0.0f);
            ourShader.setFloat("pointLights[4].constant", 1.0f);
            ourShader.setFloat("pointLights[4].linear", 0.03f);
            ourShader.setFloat("pointLights[4].quadratic", 0.016f);

            wallShader.use();
            wallShader.setVec3("lightPos[4]", -3.05f, 5.75f, -5.6f);
            wallShader.setVec3("pointLights[4].position", -3.05f, 5.75f, -5.6f);
            wallShader.setVec3("pointLights[4].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[4].diffuse", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[4].specular", 0.0f, 0.0f, 0.0f);
            wallShader.setFloat("pointLights[4].constant", 1.0f);
            wallShader.setFloat("pointLights[4].linear", 0.03f);
            wallShader.setFloat("pointLights[4].quadratic", 0.016f);
        }

        if (programState->light5) {
            ourShader.use();
            ourShader.setVec3("pointLights[5].position", -5.425f, 2.76f, -0.46f);
            ourShader.setVec3("pointLights[5].ambient", 0.05f, 0.05f, 0.05f);
            ourShader.setVec3("pointLights[5].diffuse", 0.4f, 0.4f, 0.4f);
            ourShader.setVec3("pointLights[5].specular", 0.6f, 0.6f, 0.6f);
            ourShader.setFloat("pointLights[5].constant", 1.0f);
            ourShader.setFloat("pointLights[5].linear", 0.03f);
            ourShader.setFloat("pointLights[5].quadratic", 0.016f);

            wallShader.use();
            wallShader.setVec3("lightPos[5]", -5.425f, 2.76f, -0.46f);
            wallShader.setVec3("pointLights[5].position", -5.425f, 2.76f, -0.46f);
            wallShader.setVec3("pointLights[5].ambient", 0.05f, 0.05f, 0.05f);
            wallShader.setVec3("pointLights[5].diffuse", 0.4f, 0.4f, 0.4f);
            wallShader.setVec3("pointLights[5].specular", 0.5f, 0.5f, 0.5f);
            wallShader.setFloat("pointLights[5].constant", 1.0f);
            wallShader.setFloat("pointLights[5].linear", 0.03f);
            wallShader.setFloat("pointLights[5].quadratic", 0.016f);
        } else {
            ourShader.use();
            ourShader.setVec3("pointLights[5].position", -5.425f, 2.76f, -0.46f);
            ourShader.setVec3("pointLights[5].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[5].diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("pointLights[5].specular", 0.0f, 0.0f, 0.0f);
            ourShader.setFloat("pointLights[5].constant", 1.0f);
            ourShader.setFloat("pointLights[5].linear", 0.03f);
            ourShader.setFloat("pointLights[5].quadratic", 0.016f);

            wallShader.use();
            wallShader.setVec3("lightPos[5]", -5.425f, 2.76f, -0.46f);
            wallShader.setVec3("pointLights[5].position", -5.425f, 2.76f, -0.46f);
            wallShader.setVec3("pointLights[5].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[5].diffuse", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("pointLights[5].specular", 0.0f, 0.0f, 0.0f);
            wallShader.setFloat("pointLights[5].constant", 1.0f);
            wallShader.setFloat("pointLights[5].linear", 0.03f);
            wallShader.setFloat("pointLights[5].quadratic", 0.016f);
        }


        // spotLight
        if(programState->slight) {
            ourShader.use();
            ourShader.setVec3("spotLights[0].position", programState->camera.Position);
            ourShader.setVec3("spotLights[0].direction", programState->camera.Front);
            ourShader.setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLights[0].diffuse", 1.0f, 1.0f, 1.0f);
            ourShader.setVec3("spotLights[0].specular", 1.0f, 1.0f, 1.0f);
            ourShader.setFloat("spotLights[0].constant", 1.0f);
            ourShader.setFloat("spotLights[0].linear", 0.09f);
            ourShader.setFloat("spotLights[0].quadratic", 0.032f);
            ourShader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(12.5f)));
            ourShader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(15.0f)));

            wallShader.use();
            wallShader.setVec3("spotLights[0].position", programState->camera.Position);
            wallShader.setVec3("spotLights[0].direction", programState->camera.Front);
            wallShader.setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("spotLights[0].diffuse", 1.0f, 1.0f, 1.0f);
            wallShader.setVec3("spotLights[0].specular", 1.0f, 1.0f, 1.0f);
            wallShader.setFloat("spotLights[0].constant", 1.0f);
            wallShader.setFloat("spotLights[0].linear", 0.09f);
            wallShader.setFloat("spotLights[0].quadratic", 0.032f);
            wallShader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(12.5f)));
            wallShader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(15.0f)));
        } else {
            ourShader.use();
            ourShader.setVec3("spotLights[0].position", programState->camera.Position);
            ourShader.setVec3("spotLights[0].direction", programState->camera.Front);
            ourShader.setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLights[0].diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLights[0].specular", 0.0f, 0.0f, 0.0f);
            ourShader.setFloat("spotLights[0].constant", 1.0f);
            ourShader.setFloat("spotLights[0].linear", 0.09f);
            ourShader.setFloat("spotLights[0].quadratic", 0.032f);
            ourShader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(12.5f)));
            ourShader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(15.0f)));

            wallShader.use();
            wallShader.setVec3("spotLights[0].position", programState->camera.Position);
            wallShader.setVec3("spotLights[0].direction", programState->camera.Front);
            wallShader.setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("spotLights[0].diffuse", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("spotLights[0].specular", 0.0f, 0.0f, 0.0f);
            wallShader.setFloat("spotLights[0].constant", 1.0f);
            wallShader.setFloat("spotLights[0].linear", 0.09f);
            wallShader.setFloat("spotLights[0].quadratic", 0.032f);
            wallShader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(12.5f)));
            wallShader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(15.0f)));
        }

        if(programState->light3) {
            ourShader.use();
            ourShader.setVec3("spotLights[1].position", -0.575f, 5.25f, -4.45f);
            ourShader.setVec3("spotLights[1].direction", 0.3, -0.9, 0.09);
            ourShader.setVec3("spotLights[1].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLights[1].diffuse", 1.0f, 1.0f, 1.0f);
            ourShader.setVec3("spotLights[1].specular", 1.0f, 1.0f, 1.0f);
            ourShader.setFloat("spotLights[1].constant", 1.0f);
            ourShader.setFloat("spotLights[1].linear", 0.09f);
            ourShader.setFloat("spotLights[1].quadratic", 0.032f);
            ourShader.setFloat("spotLights[1].cutOff", glm::cos(glm::radians(22.5f)));
            ourShader.setFloat("spotLights[1].outerCutOff", glm::cos(glm::radians(30.0f)));

            wallShader.use();
            wallShader.setVec3("spotLights[1].position", -0.575f, 5.25f, -4.45f);
            wallShader.setVec3("spotLights[1].direction", 0.3, -0.9, 0.09);
            wallShader.setVec3("spotLights[1].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("spotLights[1].diffuse", 1.0f, 1.0f, 1.0f);
            wallShader.setVec3("spotLights[1].specular", 1.0f, 1.0f, 1.0f);
            wallShader.setFloat("spotLights[1].constant", 1.0f);
            wallShader.setFloat("spotLights[1].linear", 0.09f);
            wallShader.setFloat("spotLights[1].quadratic", 0.032f);
            wallShader.setFloat("spotLights[1].cutOff", glm::cos(glm::radians(22.5f)));
            wallShader.setFloat("spotLights[1].outerCutOff", glm::cos(glm::radians(30.0f)));
        } else {
            ourShader.use();
            ourShader.setVec3("spotLights[1].position", -0.575f, 5.25f, -4.45f);
            ourShader.setVec3("spotLights[1].direction", 0.3, -0.9, 0.09);
            ourShader.setVec3("spotLights[1].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLights[1].diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLights[1].specular", 0.0f, 0.0f, 0.0f);
            ourShader.setFloat("spotLights[1].constant", 1.0f);
            ourShader.setFloat("spotLights[1].linear", 0.09f);
            ourShader.setFloat("spotLights[1].quadratic", 0.032f);
            ourShader.setFloat("spotLights[1].cutOff", glm::cos(glm::radians(22.5f)));
            ourShader.setFloat("spotLights[1].outerCutOff", glm::cos(glm::radians(30.0f)));

            wallShader.use();
            wallShader.setVec3("spotLights[1].position", -0.575f, 5.25f, -4.45f);
            wallShader.setVec3("spotLights[1].direction", 0.3, -0.9, 0.09);
            wallShader.setVec3("spotLights[1].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("spotLights[1].diffuse", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("spotLights[1].specular", 0.0f, 0.0f, 0.0f);
            wallShader.setFloat("spotLights[1].constant", 1.0f);
            wallShader.setFloat("spotLights[1].linear", 0.09f);
            wallShader.setFloat("spotLights[1].quadratic", 0.032f);
            wallShader.setFloat("spotLights[1].cutOff", glm::cos(glm::radians(22.5f)));
            wallShader.setFloat("spotLights[1].outerCutOff", glm::cos(glm::radians(30.0f)));
        }

        if(programState->light4) {
            ourShader.use();
            ourShader.setVec3("spotLights[2].position", 3.56f, 4.25f, 0.85f);
            ourShader.setVec3("spotLights[2].direction", -0.3f, -0.9f, 0.0f);
            ourShader.setVec3("spotLights[2].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLights[2].diffuse", 1.0f, 1.0f, 1.0f);
            ourShader.setVec3("spotLights[2].specular", 1.0f, 1.0f, 1.0f);
            ourShader.setFloat("spotLights[2].constant", 1.0f);
            ourShader.setFloat("spotLights[2].linear", 0.09f);
            ourShader.setFloat("spotLights[2].quadratic", 0.032f);
            ourShader.setFloat("spotLights[2].cutOff", glm::cos(glm::radians(40.5f)));
            ourShader.setFloat("spotLights[2].outerCutOff", glm::cos(glm::radians(60.0f)));

            wallShader.use();
            wallShader.setVec3("spotLights[2].position", 3.56f, 4.25f, 0.85f);
            wallShader.setVec3("spotLights[2].direction", -0.3f, -0.9f, 0.0f);
            wallShader.setVec3("spotLights[2].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("spotLights[2].diffuse", 1.0f, 1.0f, 1.0f);
            wallShader.setVec3("spotLights[2].specular", 1.0f, 1.0f, 1.0f);
            wallShader.setFloat("spotLights[2].constant", 1.0f);
            wallShader.setFloat("spotLights[2].linear", 0.09f);
            wallShader.setFloat("spotLights[2].quadratic", 0.032f);
            wallShader.setFloat("spotLights[2].cutOff", glm::cos(glm::radians(40.5f)));
            wallShader.setFloat("spotLights[2].outerCutOff", glm::cos(glm::radians(60.0f)));
        } else {
            ourShader.use();
            ourShader.setVec3("spotLights[2].position", 3.56f, 4.25f, 0.85f);
            ourShader.setVec3("spotLights[2].direction", -0.3f, -0.9f, 0.0f);
            ourShader.setVec3("spotLights[2].ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLights[2].diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLights[2].specular", 0.0f, 0.0f, 0.0f);
            ourShader.setFloat("spotLights[2].constant", 1.0f);
            ourShader.setFloat("spotLights[2].linear", 0.09f);
            ourShader.setFloat("spotLights[2].quadratic", 0.032f);
            ourShader.setFloat("spotLights[2].cutOff", glm::cos(glm::radians(40.5f)));
            ourShader.setFloat("spotLights[2].outerCutOff", glm::cos(glm::radians(60.0f)));

            wallShader.use();
            wallShader.setVec3("spotLights[2].position", 3.56f, 4.25f, 0.85f);
            wallShader.setVec3("spotLights[2].direction", -0.3f, -0.9f, 0.0f);
            wallShader.setVec3("spotLights[2].ambient", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("spotLights[2].diffuse", 0.0f, 0.0f, 0.0f);
            wallShader.setVec3("spotLights[2].specular", 0.0f, 0.0f, 0.0f);
            wallShader.setFloat("spotLights[2].constant", 1.0f);
            wallShader.setFloat("spotLights[2].linear", 0.09f);
            wallShader.setFloat("spotLights[2].quadratic", 0.032f);
            wallShader.setFloat("spotLights[2].cutOff", glm::cos(glm::radians(40.5f)));
            wallShader.setFloat("spotLights[2].outerCutOff", glm::cos(glm::radians(60.0f)));
        }

        // render Cube
        // face culling
        wallShader.use();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 6.0f, -6.0f));
        model = glm::scale(model, glm::vec3(6.0f));
        wallShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapWall);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMapWall);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalMapWall);
        renderQuad(2.0f);

        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 6.0f, 6.0f));
        model = glm::rotate(model, glm::radians(180.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
        model = glm::scale(model, glm::vec3(6.0f));
        wallShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapWall);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMapWall);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalMapWall);
        renderQuad(2.0f);

        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-6.0f, 6.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
        model = glm::scale(model, glm::vec3(6.0f));
        wallShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapWall);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMapWall);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalMapWall);
        renderQuad(2.0f);

        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(6.0f, 6.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
        model = glm::rotate(model, glm::radians(180.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
        model = glm::scale(model, glm::vec3(6.0f));
        wallShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapWall);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMapWall);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalMapWall);
        renderQuad(2.0f);
        glDisable(GL_CULL_FACE);

        // Bottom
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(1.0, 0.0, 0.0)));
        model = glm::rotate(model, glm::radians(180.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
        model = glm::scale(model, glm::vec3(6.0f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapBottom);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, diffuseMapBottom);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalMapBottom);
        wallShader.setMat4("model", model);
        renderQuad(5.0f);

        // Top
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 12.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(1.0, 0.0, 0.0)));
        model = glm::scale(model, glm::vec3(6.0f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapTop);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMapTop);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalMapTop);
        wallShader.setMat4("model", model);
        renderQuad(1.0f);

        glEnable(GL_CULL_FACE);
        // desk
        ourShader.use();
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.965f, -4.6f));
        model = glm::rotate(model, glm::radians(180.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
        model = glm::rotate(model, glm::radians(0.4f), glm::normalize(glm::vec3(0.0, 0.0, 1.0)));
        model = glm::scale(model, glm::vec3(5.5f));
        ourShader.setMat4("model", model);
        desk.Draw(ourShader);

        // chair
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.5f, -0.235f, -2.0f));
        model = glm::rotate(model, glm::radians(93.0f), glm::normalize(glm::vec3(1.0, 0.0, 0.0)));
        model = glm::rotate(model, glm::radians(72.8f), glm::normalize(glm::vec3(0.0, 0.0, 1.0)));
        model = glm::scale(model, glm::vec3(0.7f));
        ourShader.setMat4("model", model);
        chair.Draw(ourShader);

        // table
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-3.65f, 0.01f, -3.8f));
        model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(0.0, -1.0, 0.0)));
        model = glm::scale(model, glm::vec3(0.8f));
        ourShader.setMat4("model", model);
        table.Draw(ourShader);

        // table1
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 4.0f));
        model = glm::rotate(model, glm::radians(70.0f), glm::normalize(glm::vec3(0.0, -1.0, 0.0)));
        model = glm::scale(model, glm::vec3(0.4f));
        ourShader.setMat4("model", model);
        table1.Draw(ourShader);

        // couch
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(3.8f, -0.2f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(0.0, -1.0, 0.0)));
        model = glm::scale(model, glm::vec3(0.9f));
        ourShader.setMat4("model", model);
        couch.Draw(ourShader);

        // laptop
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.0f, 2.813f, -5.0f));
        model = glm::rotate(model, glm::radians(75.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
        ourShader.setMat4("model", model);
        laptop.Draw(ourShader);

        // plant
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.0f, 2.791f, -4.5f));
        model = glm::rotate(model, glm::radians(15.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
        model = glm::scale(model, glm::vec3(0.45f));
        ourShader.setMat4("model", model);
        plant.Draw(ourShader);

        // plant1
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-4.8f, 2.454, 4.2f));
        model = glm::rotate(model, glm::radians(40.0f), glm::normalize(glm::vec3(0.0, -1.0, 0.0)));
        ourShader.setMat4("model", model);
        plant1.Draw(ourShader);

        // apples
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-5.2f, 2.45f, 1.0f));
        model = glm::scale(model, glm::vec3(0.4f));
        ourShader.setMat4("model", model);
        apples.Draw(ourShader);

        // bowl
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.5f, 0.92f, 4.6f));
        model = glm::scale(model, glm::vec3(0.1f));
        ourShader.setMat4("model", model);
        bowl.Draw(ourShader);

        glEnable(GL_CULL_FACE);
        // light1
        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);
        if (programState->light1) {
            lightShader.use();
            glCullFace(GL_BACK);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 11.05f, 0.0f));
            model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
            model = glm::scale(model, glm::vec3(1.2f));
            lightShader.setMat4("model", model);
            light1.Draw(lightShader);
        } else {
            ourShader.use();
            glCullFace(GL_BACK);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 11.05f, 0.0f));
            model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
            model = glm::scale(model, glm::vec3(1.2f));
            ourShader.setMat4("model", model);
            light1.Draw(ourShader);
        }

        glDisable(GL_CULL_FACE);
        // light2_1
        if(programState->light2_1){
            lightShader.use();
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(2.8f, 5.0f, -5.99f));
            lightShader.setMat4("model", model);
            light2.Draw(lightShader);
        } else {
            ourShader.use();
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(2.8f, 5.0f, -5.99f));
            ourShader.setMat4("model", model);
            light2.Draw(ourShader);
        }
        // light2_1
        if(programState->light2_2) {
            lightShader.use();
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-2.8f, 5.0f, -5.99f));
            lightShader.setMat4("model", model);
            light2.Draw(lightShader);
        } else {
            ourShader.use();
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-2.8f, 5.0f, -5.99f));
            ourShader.setMat4("model", model);
            light2.Draw(ourShader);
        }

        glEnable(GL_CULL_FACE);
        // light3
        if(programState->light3) {
            lightShader.use();
            glCullFace(GL_BACK);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-1.4f, 2.786f, -5.2f));
            model = glm::rotate(model, glm::radians(145.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
            model = glm::scale(model, glm::vec3(0.04f));
            lightShader.setMat4("model", model);
            light3.Draw(lightShader);
        } else {
            ourShader.use();
            glCullFace(GL_BACK);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-1.4f, 2.786f, -5.2f));
            model = glm::rotate(model, glm::radians(145.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
            model = glm::scale(model, glm::vec3(0.04f));
            ourShader.setMat4("model", model);
            light3.Draw(ourShader);
        }

        // light4
        if(programState->light4) {
            lightShader.use();
            glCullFace(GL_BACK);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(4.2f, 0.0f, -3.5f));
            model = glm::rotate(model, glm::radians(95.0f), glm::normalize(glm::vec3(0.0, -1.0, 0.0)));
            model = glm::scale(model, glm::vec3(0.07f));
            lightShader.setMat4("model", model);
            light4.Draw(lightShader);
        } else {
            ourShader.use();
            glCullFace(GL_BACK);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(4.2f, 0.0f, -3.5f));
            model = glm::rotate(model, glm::radians(95.0f), glm::normalize(glm::vec3(0.0, -1.0, 0.0)));
            model = glm::scale(model, glm::vec3(0.07f));
            ourShader.setMat4("model", model);
            light4.Draw(ourShader);
        }

        // light5
        if(programState->light5) {
            lightShader.use();
            glCullFace(GL_BACK);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-4.8f, 2.66f, -1.0f));
            model = glm::rotate(model, glm::radians(55.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
            model = glm::scale(model, glm::vec3(1.4f));
            lightShader.setMat4("model", model);
            light5.Draw(lightShader);
        } else {
            ourShader.use();
            glCullFace(GL_BACK);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-4.8f, 2.66f, -1.0f));
            model = glm::rotate(model, glm::radians(55.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
            model = glm::scale(model, glm::vec3(1.4f));
            ourShader.setMat4("model", model);
            light5.Draw(ourShader);
        }

        glDisable(GL_CULL_FACE);
        // glass
        // blending
        glassShader.use();
        glassShader.setBool("light", (programState->dlight || programState->light1 || programState->light2_1 || programState->light2_2 || programState->light3 || programState->light5));
        glassShader.setMat4("projection", projection);
        glassShader.setMat4("view", view);

        if (distance) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-4.325f, 1.665f, 3.235f));
            model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
            model = glm::scale(model, glm::vec3(1.25f, 1.56f, 0.0f));
            glBindTexture(GL_TEXTURE_2D, glassTexture);
            glassShader.setMat4("model", model);
            renderGlass();

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-1.0f, 2.77f, -4.0f));
            glassShader.setMat4("model", model);
            glass.Draw(glassShader);
        } else {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-1.0f, 2.77f, -4.0f));
            glassShader.setMat4("model", model);
            glass.Draw(glassShader);

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-4.325f, 1.665f, 3.235f));
            model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
            model = glm::scale(model, glm::vec3(1.25f, 1.56f, 0.0f));
            glBindTexture(GL_TEXTURE_2D, glassTexture);
            glassShader.setMat4("model", model);
            renderGlass();
        }

        glEnable(GL_CULL_FACE);
        // -----------------------------------------------------------------------------

        // 2. now render quad with scene's visuals as its texture image
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        // draw Screen quad
        screenShader.use();
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled); // use multisampled texture
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// renders a 1x1 quad in NDC with manually calculated tangent vectors
// ------------------------------------------------------------------
unsigned int VAO = 0;
unsigned int VBO;
void renderQuad(float tex)
{
    if (VAO == 0)
    {
        // positions
        glm::vec3 pos1(-1.0f,  1.0f, 0.0f);
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3( 1.0f, -1.0f, 0.0f);
        glm::vec3 pos4( 1.0f,  1.0f, 0.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, tex);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(tex, 0.0f);
        glm::vec2 uv4(tex, tex);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


        float quadVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// render Glass
// ------------------------------------------------
unsigned int transparentVAO = 0;
unsigned int transparentVBO = 0;
void renderGlass()
{
    if(transparentVAO == 0)
    {
        float transparentVertices[] = {
                // positions         // texture Coords
                0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
                0.0f, -0.5f,  0.0f,  0.0f,  0.0f,
                1.0f, -0.5f,  0.0f,  1.0f,  0.0f,

                0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
                1.0f, -0.5f,  0.0f,  1.0f,  0.0f,
                1.0f,  0.5f,  0.0f,  1.0f,  1.0f

        };

        glGenVertexArrays(1, &transparentVAO);
        glGenBuffers(1, &transparentVBO);
        glBindVertexArray(transparentVAO);
        glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);
    }
    glBindVertexArray(transparentVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Camera mouse");
        ImGui::Checkbox("", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    {
        ImGui::Begin("ON/OFF");
        ImGui::Checkbox("Light1", &programState->light1);
        ImGui::Checkbox("Light2_1", &programState->light2_1);
        ImGui::Checkbox("Light2_2", &programState->light2_2);
        ImGui::Checkbox("Light3", &programState->light3);
        ImGui::Checkbox("Light4", &programState->light4);
        ImGui::Checkbox("Light5", &programState->light5);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        programState->dlight = !programState->dlight;
    }

    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        programState->slight = !programState->slight;
    }
}
