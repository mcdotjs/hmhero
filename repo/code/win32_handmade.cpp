#include <windows.h>

// TODO: this is global now
// NOTE: CONFUSING
// it means something different, based on where you put it

#define internal static
//NOTE: local to the file that is in
//(cannot be called from other files)

#define local_persist static // locally scoped, but persisting
#define global_variable static
global_variable bool Running; // this is initialized to 0

LRESULT CALLBACK MainWindowCallback(HWND   Window,
                                    UINT   Message,
                                    WPARAM WParam,
                                    LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message) {
    case WM_SIZE: {
        OutputDebugStringA("WM_SIZE");
    } break;

    case WM_DESTROY: {
        // TODO: handle this with message to user
        OutputDebugStringA("WM_DESTROY");
    } break;

    case WM_CLOSE: {
        // PostQuitMessage(0);
        Running = false;
        // TODO: handle this with error - recreate window?
        OutputDebugStringA("WM_CLOSE");
    } break;

    case WM_ACTIVATEAPP: {
        OutputDebugStringA("WM_ACTIVATEAPP");
    } break;

    case WM_PAINT: {
        PAINTSTRUCT PaintStruct;
        OutputDebugStringA("WM_PAINT");

        // break moze byt aj inside block
        HDC DeviceContext = BeginPaint(Window, &PaintStruct);
        EndPaint(Window, &PaintStruct);
        int X      = PaintStruct.rcPaint.left;
        int Y      = PaintStruct.rcPaint.right;
        int Width  = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left;
        int Height = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top;

        // static not using in prod
        local_persist DWORD Operation = WHITENESS;
        PatBlt(DeviceContext, X, Y, Width, Height, Operation);

        if (Operation == WHITENESS) {
            Operation = BLACKNESS;
        }
        else {
            Operation = WHITENESS;
        }

        break;
    }
    default: {
        // OutputDebugStringA("WM_DESTROY");
        Result = DefWindowProc(Window, Message, WParam, LParam);
    } break;
    }
    return Result;
}

int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     PSTR      CommandLine,
                     int       ShowCode)
{
    WNDCLASS WindowClass      = {};
    WindowClass.style         = CS_OWNDC;
    WindowClass.lpfnWndProc   = MainWindowCallback;
    WindowClass.hInstance     = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass)) {
        HWND WindowHandle = CreateWindowEx(0,
                                           WindowClass.lpszClassName,
                                           "HandmadeHeroWindowClass",
                                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           0,
                                           0,
                                           Instance,
                                           0);
        if (WindowHandle) {
            MSG Message;
            Running = true;
            while (Running) {
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0) {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else {
                    // NOTE: we don't to have to clean memory
                    // to use RAII, windows will do it for us
                    //... slow closing
                    break;
                }
            }
        }
        else {
            // TODO: logging
        }
    }
    else {
        // TODO: logging?
    }
    return 0;
}
