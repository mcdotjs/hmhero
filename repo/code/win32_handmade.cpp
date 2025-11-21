#include <cstdint>
#include <stdint.h>
#include <windows.h>
#include <winuser.h>

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

struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void      *Memory;
    int        Width;
    int        Height;
    int        Pitch;
    int        BitesPerPixel;
};

global_variable bool                   Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

struct win32_window_dimension {
    int Width;
    int Height;
};

win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT                   WindowRect;

    GetClientRect(Window, &WindowRect);
    Result.Width  = WindowRect.right - WindowRect.left;
    Result.Height = WindowRect.bottom - WindowRect.top;

    return Result;
};

internal void renderWeirdGradient(win32_offscreen_buffer Buffer,
                                  int                    XOffset,
                                  int                    YOffset)
{
    // NOTE: let see what optimazer does, when buffer will be passed by value
    int Width = Buffer.Width;
    // NOTE: we need it for shifting byte by byte
    // because C it multiply by size of thing being pointed to
    // (if uint16...it will be moved by two )
    uint8 *Row = (uint8 *) Buffer.Memory;
    for (int Y = 0; Y < Buffer.Height; ++Y) {
        uint32 *Pixel = (uint32 *) Row;
        for (int X = 0; X < Buffer.Width; ++X) {
            // blue
            uint8 Blue  = (uint8) (X + XOffset);
            uint8 Green = (uint8) (Y + YOffset);
            *Pixel++    = ((Green << 8) | Blue);
        }
        Row += Buffer.Pitch;
        // OR
        // Row = (uint8 *) Pixel;
    }
}
internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer,
                                    int                     Width,
                                    int                     Height)
{

    if (Buffer->Memory) {
        // NOTE: when debugging use VirtualProtect with NO_ACCESS
        // then if buggy code want to touch that memory, you will see it
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Height = Height;
    Buffer->Width  = Width;

    Buffer->Info.bmiHeader.biSize        = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth       = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight      = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes      = 1;
    Buffer->Info.bmiHeader.biBitCount    = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    Buffer->BitesPerPixel                = 4;
    int BitmapMemorySize =
        (Buffer->Width * Buffer->Height) * Buffer->BitesPerPixel;
    Buffer->Memory =
        VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Buffer->Width * Buffer->BitesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC                    DeviceContext,
                                         int                    WindowWidth,
                                         int                    WindowHeight,
                                         win32_offscreen_buffer Buffer,
                                         int                    X,
                                         int                    Y,
                                         int                    Width,
                                         int                    Height)
{
    /*
     * NOTE:
     * we have 2D bitmap, but 1D representation of it
     * because memory is just series of bytes
     * so we need to store somthing 2D in 3D
     *
     * */
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
                  Buffer.Width,
                  Buffer.Height,
                  0,
                  0,
                  WindowWidth,
                  WindowHeight,
                  Buffer.Memory,
                  &Buffer.Info,
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
        win32_window_dimension Dimension = Win32GetWindowDimension(Window);

        // NOTE: Win32ResizeDIBSection sa vola vzdy pri resize
        //  you need free that memory
        Win32ResizeDIBSection(
            &GlobalBackBuffer, Dimension.Width, Dimension.Height);
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
        // NOTE: later
        // case WM_SETCURSOR: {
        //     SetCursor(0);
        // } break;
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

        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        Win32DisplayBufferInWindow(DeviceContext,
                                   Dimension.Width,
                                   Dimension.Height,
                                   GlobalBackBuffer,
                                   X,
                                   Y,
                                   Width,
                                   Height);
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
    WNDCLASS WindowClass = {};
    WindowClass.style =
        CS_HREDRAW | CS_VREDRAW; // when resizing window refresh whole window,
                                 // not just changed parts
    WindowClass.lpfnWndProc   = Win32MainWindowCallback;
    WindowClass.hInstance     = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass)) {
        HWND Window = CreateWindowEx(0,
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
        if (Window) {
            int XOffset = 0;
            int YOffset = 0;
            Running     = true;
            while (Running) {
                MSG Message;
                // NOTE: if (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
                // we have a lot of messages ... if to while
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT) {
                        Running = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                renderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);
                HDC                    DeviceContext = GetDC(Window);
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(DeviceContext,
                                           Dimension.Width,
                                           Dimension.Height,
                                           GlobalBackBuffer,
                                           0,
                                           0,
                                           Dimension.Width,
                                           Dimension.Height);
                ReleaseDC(Window, DeviceContext);
                ++XOffset;
                ++YOffset;
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
