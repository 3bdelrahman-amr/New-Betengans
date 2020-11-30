#ifndef NEW_BETENGAN_PLAYSTATE_H
#define NEW_BETENGAN_PLAYSTATE_H

#include "State.h"
#include<vector>
#include"../Systems/MeshRendererSystem.h"

class PlayState : public State{

    //////////////////////
    Entity CameraEntity;
    vector<Entity> entities;
    //////////////////////////////////////
    CameraSystem* CSys;
    shared_ptr<MeshRendererSystem> MSys;

public:
    void onEnter(Betngan::Mouse *mouse, Betngan::Keyboard *keyboard, GLFWwindow *window) override{
        Entity e = manager->CreateEntity();
        entities.push_back(e);
        MSys= manager->RegisterSystem<MeshRendererSystem>();
        MSys->Set_Mngr_ptr(manager);
        manager->RegisterComponent<MeshRendererr>();
        manager->AddComponent(e, MeshRendererr());
        //manager.RegisterComponent<ShaderProg>();
        manager->RegisterComponent<vector<Transform>>();
        manager->AddComponent(e, vector<Transform>());

        //MSys->Entities.insert(e);
        MSys->init(e);
        CSys = MSys->get_cam_ptr();
        CSys->SetEngineApp_ptr(mouse,keyboard,window);
        CameraEntity = manager->CreateEntity();
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        //////////////////////
        manager->RegisterComponent<Camera>();
        manager->AddComponent(CameraEntity, Camera());
        ////////////////
        CSys->setEyePosition({ 10, 10, 10 },CameraEntity); //position vector which is x axis
        CSys->setTarget({ 0, 0, 0 },CameraEntity); // target vector
        CSys->setUp({ 0, 1, 0 }, CameraEntity); // up vector
        /////////////////////////////////////////////////////////////////////////////////////////////////
        //CameraSystem::se
        CSys->setupPerspective(glm::pi<float>() / 2, static_cast<float>(width) / height, 0.1f, 100.0f,CameraEntity); //set perspective mode for the camera

        CSys->initializeController(CameraEntity);
        glClearColor(0, 0, 0, 0);
    }

    void onDraw(double deltaTime) override {
        MSys->Draw(deltaTime,CameraEntity);
    }

    void onImmediateGui(ImGuiIO &io) override {

    }

    void onExit() override {

    }
};
#endif //NEW_BETENGAN_PLAYSTATE_H
