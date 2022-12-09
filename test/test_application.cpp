#include "../src/Routn.h"
#include "../src/Application.h"

int main(int argc, char** argv){
    Routn::Application app;
    if(app.init(argc, argv)) {
        return app.run();
    }
    return 0;
}