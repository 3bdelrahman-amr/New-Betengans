#ifndef ENGINE
#define ENGINE

#include"Application/Application.h"
//////////////////////////////////////////////////////////////////////////////////////

class EngineApplication : public Betngan::Application {
    ////////////////////////////////////////////
    Betngan::WindowConfiguration getWindowConfiguration() override {
        return { "BETANGNAT ENGINE", {1280,720 }, false };
    }

};
#endif // !ENGINE
