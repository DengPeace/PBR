//
//  main.cpp
//  TryGL — PBR Demo
//

#include "glframework/core.h"
#include "glframework/shader.h"

#include <iostream>
#include <string>

#include "wrapper/checkError.h"
#include "application/Application.h"

#define STB_IMAGE_IMPLEMENTATION
#include "application/stb_image.h"

#include <unistd.h>
#include "glframework/texture.h"

#include "application/camera/perspectiveCamera.h"
#include "application/camera/gameCameraControl.h"

#include "glframework/geometry.h"
#include "glframework/material/pbrMaterial.h"

#include "glframework/mesh/mesh.h"
#include "glframework/renderer/renderer.h"
#include "glframework/light/pointLight.h"
#include "glframework/light/spotLight.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "glframework/scene.h"
#include "application/assimpLoader.h"
#include "glframework/framebuffer/framebuffer.h"
#include "glframework/framebuffer/shadowMap.h"

#include "timer.h"

/*
 * 目   标: PBR Demo — Cook-Torrance BRDF
   拆分目标:
        1 创建 PBR shader（pbr.vert / pbr.frag）
        2 创建 PBRMaterial 类
        3 修改 Renderer 支持 PBR 渲染
        4 main.cpp 加载 bag 模型 + PBR 材质 + 多光源
*/

Renderer* renderer = nullptr;
Scene* scene = nullptr;

unsigned int WIDTH = 1280;
unsigned int HEIGHT = 720;

// 点光源
glm::vec3 lightPositions[4] = {
    glm::vec3(2.0f, 3.0f, 2.0f),
    glm::vec3(-2.0f, 2.0f, 1.0f),
    glm::vec3(0.0f, 4.0f, -2.0f),
    glm::vec3(-1.0f, 1.0f, -3.0f)
};
glm::vec3 lightColors[4] = {
    glm::vec3(30.0f, 30.0f, 30.0f),
    glm::vec3(20.0f, 10.0f, 10.0f),
    glm::vec3(10.0f, 10.0f, 20.0f),
    glm::vec3(15.0f, 20.0f, 15.0f)
};

DirectionalLight* dirLight = nullptr;
AmbientLight* ambLight = nullptr;

PerspectiveCamera* camera = nullptr;
GameCameraControl* cameraControl = nullptr;

glm::vec3 clearColor{0.1, 0.1, 0.15};

// 材质参数（供 ImGui 调整）
float roughnessOverride = -1.0f;  // -1 = 使用贴图
float metallicOverride = -1.0f;
float envLightIntensity = 0.3f;

void OnResize(int width, int height) {
    GL_CALL(glViewport(0,0,width, height));
}

void OnKey(int key, int action, int mods) {
    cameraControl -> onKey(key, action, mods);
}

void OnMouse(int button, int action, int mods) {
    double x,y;
    glApp -> getCursorPosition(&x, &y);
    cameraControl -> onMouse(button, action, x, y);
}

void OnCursor(double xpos, double ypos) {
    cameraControl -> onCursor(xpos, ypos);
}

void OnScroll(double offset){
    cameraControl -> onScroll(offset);
}

Object* loadPBRModel(const std::string& path, PBRMaterial* material) {
    auto model = AssimpLoader::load(path);
    return model;
}

void prepare() {
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    
    renderer = new Renderer();
    scene = new Scene();
    
    // ===== 加载 bag 模型 =====
    PBRMaterial* bagMat = new PBRMaterial();
    bagMat -> mAlbedoMap = new Texture("assets/fbx/bag/diffuse.jpg", 0);
    bagMat -> mNormalMap = new Texture("assets/fbx/bag/normal.png", 1);
    bagMat -> mMetallicMap = new Texture("assets/fbx/bag/specular.jpg", 2);  // specular → metallic
    bagMat -> mRoughnessMap = new Texture("assets/fbx/bag/roughness.jpg", 3);
    bagMat -> mAoMap = new Texture("assets/fbx/bag/ao.jpg", 4);
    bagMat -> mEnvMap = new Texture("assets/texture/sphericalMap.png", 5);
    
    auto bagModel = AssimpLoader::load("assets/fbx/bag/backpack.obj");
    
    // 递归设置 PBR 材质
    std::function<void(Object*)> setPBRMat = [&](Object* obj) {
        if (obj -> getType() == ObjectType::Mesh) {
            Mesh* mesh = (Mesh*)obj;
            mesh -> mMaterial = bagMat;
        }
        auto children = obj -> getChildren();
        for (auto child : children) {
            setPBRMat(child);
        }
    };
    setPBRMat(bagModel);
    
    scene -> addChild(bagModel);
    
    // ===== 光源 =====
    dirLight = new DirectionalLight();
    dirLight -> mDirection = glm::vec3(-0.5f, -1.0f, -0.3f);
    dirLight -> mColor = glm::vec3(1.0f, 0.95f, 0.9f);  // 暖色方向光
    dirLight -> mIntensity = 1.0f;
    dirLight -> mSpecularIntensity = 0.5f;
    
    ambLight = new AmbientLight();
    ambLight -> mColor = glm::vec3(0.05f);
}

void prepareCamera() {
    camera = new PerspectiveCamera(
        45.0f,
        (float) glApp -> getWidth() / (float) glApp -> getHeigth(),
        0.1f,
        100.0f
    );
    
    cameraControl = new GameCameraControl();
    cameraControl -> setCamera(camera);
    cameraControl -> setSensitivity(0.4f);
    cameraControl -> setSpeed(0.05f);
}

void initIMGUI() {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOther(glApp -> getWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void renderIMGUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("PBR Controls");
    ImGui::Text("Lighting");
    ImGui::SliderFloat("Dir Light Intensity", &dirLight -> mIntensity, 0.0f, 5.0f);
    ImGui::SliderFloat("Env Light", &envLightIntensity, 0.0f, 1.0f);
    ImGui::Separator();
    ImGui::Text("Camera Pos: %.2f, %.2f, %.2f", camera -> mPosition.x, camera -> mPosition.y, camera -> mPosition.z);
    ImGui::End();
    
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(glApp -> getWindow(), &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main() {
    if (!glApp -> init(WIDTH, HEIGHT)) {
        return -1;
    }
    
    glApp -> setResizeCallback(OnResize);
    glApp -> setKeyBoardCallback(OnKey);
    glApp -> setMouseCallback(OnMouse);
    glApp -> setCursorCallback(OnCursor);
    glApp -> setScrollCallback(OnScroll);
    
    GL_CALL(glViewport(0, 0, WIDTH, HEIGHT));
    GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));

    prepareCamera();
    prepare();
    initIMGUI();
    
    while(glApp -> update()) {
        Timer a;
        cameraControl -> update();
        
        // 每帧更新点光源位置（让它们缓慢旋转）
        float time = glfwGetTime();
        lightPositions[0].x = 3.0f * cos(time * 0.3f);
        lightPositions[0].z = 3.0f * sin(time * 0.3f);
        lightPositions[1].x = 2.5f * cos(time * 0.5f + 1.0f);
        lightPositions[1].z = 2.5f * sin(time * 0.5f + 1.0f);
        
        renderer -> setPointLights(lightPositions, lightColors, 4);
        renderer -> setClearColor(clearColor);
        renderer -> render(scene, camera, dirLight, ambLight);
        
        renderIMGUI();
    }
    
    glApp -> destroy();
    return 0;
}
