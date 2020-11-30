#ifndef NEW_BETENGAN_STATE_H
#define NEW_BETENGAN_STATE_H

#include <imgui.h>
#include <Application/input/mouse.h>
#include <Application/input/keyboard.h>

//this is abstract class for state
class State {
protected:
    Manager *manager;
public:
    void setManager(Manager* m){
        manager=m;
    }
    virtual void onEnter(Betngan::Mouse* mouse, Betngan::Keyboard* keyboard, GLFWwindow* window) = 0;

    virtual void onDraw(double deltaTime) = 0;

    virtual void onImmediateGui(ImGuiIO &io) = 0;

    virtual void onExit() = 0;

};

#endif //NEW_BETENGAN_STATE_H
