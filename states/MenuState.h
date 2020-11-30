#ifndef NEW_BETENGAN_MENUSTATE_H
#define NEW_BETENGAN_MENUSTATE_H

#include "State.h"

class MenuState : public State{
public:
    void onEnter(Betngan::Mouse* mouse, Betngan::Keyboard* keyboard, GLFWwindow* window) override {

    }

    void onDraw(double deltaTime) override {

    }

    void onImmediateGui(ImGuiIO &io) override {

    }

    void onExit() override {

    }
};
#endif //NEW_BETENGAN_MENUSTATE_H
