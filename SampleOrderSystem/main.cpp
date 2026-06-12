#include <windows.h>
#include "View/SplashView.h"
#include "View/MainView.h"

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    SplashView splash;
    splash.show();

    MainView mainView;
    mainView.run();

    return 0;
}
