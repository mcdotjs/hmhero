#include <windows.h>

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PSTR lpCmdLine,
    int nCmdShow
) {
    //define MessageBoxA or W ...this is macro based on your compiling mode (UTF8 or ansi)
    MessageBox(0, "Handmade Hero", "Mirko", MB_OK | MB_ICONINFORMATION);
    return 0;
}
