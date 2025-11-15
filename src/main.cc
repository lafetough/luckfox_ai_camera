#include "app.h"


int main(int argc, char *argv[]) {

    App app(720, 480, 554);

    if (!app.init()) {
        printf("ERROR: cant initialize app\n");
        return -1;
    }

    return app.run();
}
