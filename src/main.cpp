#include"EngineApplication.h"
#include "../states/PlayState.h"

int main(int argc, char** argv) {
    EngineApplication engineApplication;
    PlayState playState;
    engineApplication.goToState(&playState);
    return engineApplication.run();
}
