#include <windows.h>
#include <winuser.h>

LRESULT CALLBACK MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
) {
    LRESULT Result = 0;
    switch (Message) {
        case WM_SIZE: {
            OutputDebugStringA("WM_SIZE");
        }
        break;
        case WM_DESTROY: {
            OutputDebugStringA("WM_DESTROY");
        }
        break;
        case WM_CLOSE: {
            OutputDebugStringA("WM_CLOSE");
        }
        break;
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP");
        }
        break;
        case WM_PAINT: {
            PAINTSTRUCT PaintStruct;
            OutputDebugStringA("WM_PAINT");
            //break moze byt aj inside block
            HDC DeviceContext = BeginPaint(
                Window, &PaintStruct
            );
            EndPaint(
                Window, &PaintStruct
            );
            int X = PaintStruct.rcPaint.left;
            int Y = PaintStruct.rcPaint.right;
            int Width = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left;
            int Height = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top;
            //static not using in prod
            static DWORD Operation = WHITENESS;
            PatBlt(DeviceContext, X, Y, Width, Height, Operation);
            if (Operation == WHITENESS) {
                Operation = BLACKNESS;
            } else {
                Operation = WHITENESS;
            }

            break;
        }
        default: {
            //OutputDebugStringA("WM_DESTROY");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        }
        break;
    }
    return Result;
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    PSTR CommandLine,
    int ShowCode
) {
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_OWNDC;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass)) {
        HWND WindowHandle =
                CreateWindowEx(
                    0,
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
                    0
                );
        if (WindowHandle) {
            MSG Message;
            for (;;) {
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0) {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                } else {
                    break;
                }
            }
        } else {
            //TODO: logging
        }
    } else {
        //TODO: logging?
    }
    return 0;
}
