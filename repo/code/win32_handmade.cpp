#include <cstdint>
#include <stdint.h>
#include <windows.h>

// TODO: this is global now
#define internal static
#define local_persist static // locally scoped, but persisting
#define global_variable static
// typedef unsigned char uint8; or use cstdint || stdint.h
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

global_variable bool       Running; // this is initialized to 0
global_variable BITMAPINFO BitmapInfo;
global_variable void      *BitmapMemory;
// TODO: temp!!
global_variable int BitmapWidth;
global_variable int BitmapHeight;

internal void Win32ResizeDIBSection(int Width, int Height)
{

    if (BitmapMemory) {
        // NOTE: when debugging use VirtualProtect with NO_ACCESS
        // then if buggy code want to touch that memory, you will see it
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapHeight = Height;
    BitmapWidth  = Width;

    BitmapInfo.bmiHeader.biSize     = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth    = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight   = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes   = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    // NOTE: bits per pixel r8, g8, b8 and padding
    //  four bite boundary
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BitesPerPixel    = 4;
    int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BitesPerPixel;
    // doesn't matter where allocate
    BitmapMemory =
        VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    int Pitch = Width * BitesPerPixel;
    // NOTE: we need it for shifting byte by byte
    // because C it multiply by size of thing being pointed to
    // (if uint16...it will be moved by two )
    uint8 *Row = (uint8 *) BitmapMemory;
    for (int Y = 0; Y < BitmapHeight; ++Y) {
        uint8 *Pixel = (uint8 *) Row;
        for (int X = 0; X < BitmapWidth; ++X) {
            /*
             * with uint8 PIXEL we are WRITING EACH BYTE IN MEMORY
             *                   Pixel+0  Pixel+1  Pixel+2  Pixel+3
             *                   0  1  2  3-th byte
             * Pixel in memory: 00 00 00 00
             *                  RR GG BB PP ... ???NOOOO!!!
             *                  LITTLE ENDIAN ARCHITECTURE!!!!
             *                  r and b are swapped
             *                  BB GG RR PP ... ???NOOOO!!!
             * */
            // blue
            *Pixel = (uint8) X;
            ++Pixel;
            // green
            *Pixel = (uint8) Y;
            ++Pixel;
            // red
            *Pixel = 0;
            ++Pixel;

            // padidng
            *Pixel = 0;
            ++Pixel;
        }
        Row += Pitch;
    }
}

internal void Win32UpdateWindow(
    HDC DeviceContext, RECT *WindowRect, int X, int Y, int Width, int Height)
{
    /*
     * NOTE:
     * we have 2D bitmap, but 1D representation of it
     * because memory is just series of bytes
     * so we need to store somthing 2D in 3D
     *
     * */
    int WindowWidth  = WindowRect->right - WindowRect->left;
    int WindowHeight = WindowRect->bottom - WindowRect->top;
    StretchDIBits(DeviceContext,
                  // X,
                  // Y,
                  // Width,
                  // Height,
                  // X,
                  // Y,
                  // Width,
                  // Height,
                  0,
                  0,
                  BitmapWidth,
                  BitmapHeight,
                  0,
                  0,
                  WindowWidth,
                  WindowHeight,
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

        RECT ClientRect;
        GetClientRect(Window, &ClientRect);

        Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
        EndPaint(Window, &PaintStruct);
    }
    default: {
        OutputDebugStringA("WM_Default");
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
