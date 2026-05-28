#include "app.h"

int main()
{
    static App app(1600, 1200);
    //static App app(1280, 720);

    app.Run();

    return 0;
}
