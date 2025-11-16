#include <windows.h>
#include <wingdi.h>

// TODO: this is global now
// NOTE: CONFUSING
// it means something different, based on where you put it

#define internal static
// NOTE: local to the file that is in
//(cannot be called from other files)

#define local_persist static // locally scoped, but persisting
#define global_variable static
global_variable bool       Running; // this is initialized to 0
global_variable BITMAPINFO BitmapInfo;
global_variable void      *BitmapMemory;
global_variable HBITMAP    BitmapHandle;
global_variable HDC        BitmapDeviceContext;

// d3_1
internal void Win32ResizeDIBSection(int Width, int Height)
{

    // TODO: bolletproof this
    // maybe dont free first, free after, then free first if that fails
    // free our DIBSection
    if (BitmapHandle) {
        DeleteObject(BitmapHandle);
    }
    if (!BitmapDeviceContext) {
        // TODO: should we recreate these under some special circumstantes?
        // unplug monitor...
        BitmapDeviceContext = CreateCompatibleDC(0);
    }

    BitmapInfo.bmiHeader.biSize        = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth       = Width;
    BitmapInfo.bmiHeader.biHeight      = Height;
    BitmapInfo.bmiHeader.biPlanes      = 1;
    BitmapInfo.bmiHeader.biBitCount    = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;
    // NOTE: due to intializing as static this stuff are initialized as 0
    // so there is no need to initialize
    //  BitmapInfo.bmiHeader.biSizeImage     = 0;
    //  BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    //  BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    //  BitmapInfo.bmiHeader.biClrUsed       = 0;
    //  BitmapInfo.bmiHeader.biClrImportant  = 0;

    BitmapHandle = CreateDIBSection(
        BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}

internal void Win32UpdateWindow(
    HDC DeviceContext, int X, int Y, int Width, int Height)
{
    StretchDIBits(DeviceContext,
                  X,
                  Y,
                  Width,
                  Height,
                  X,
                  Y,
                  Width,
                  Height,
                  BitmapMemory,
                  &BitmapInfo,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND   Window,
                                         UINT   Message,
                                         WPARAM WParam,
                                         LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message) {
    case WM_SIZE: {
        OutputDebugStringA("WM_SIZE");
        RECT ClientRect;
        GetClientRect(Window, &ClientRect);
        int Width  = ClientRect.right - ClientRect.left;
        int Height = ClientRect.bottom - ClientRect.top;

        // NOTE: Win32ResizeDIBSection sa vola vzdy pri resize
        //  you need free that memory
        Win32ResizeDIBSection(Width, Height);
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
        // d3_0
        PAINTSTRUCT PaintStruct;
        OutputDebugStringA("WM_PAINT");

        // break moze byt aj inside block
        HDC DeviceContext = BeginPaint(Window, &PaintStruct);
        EndPaint(Window, &PaintStruct);
        int X      = PaintStruct.rcPaint.left;
        int Y      = PaintStruct.rcPaint.right;
        int Width  = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left;
        int Height = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top;
        Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
        EndPaint(Window, &PaintStruct);
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
    WindowClass.lpfnWndProc   = Win32MainWindowCallback;
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
