#include "app.h"

int main(int argc, char** argv)
{
    int width = 1600, height = 1200;
    if (argc >= 3) {
        width = std::atoi(argv[1]);
        height = std::atoi(argv[2]);
    }

    static App app(width, height);
    app.Run();

    return 0;
}
