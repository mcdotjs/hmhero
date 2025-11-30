#include <cstdint>
#include <stdint.h>
#include <windows.h>
#include <winuser.h>
#include <xinput.h>

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

// NOTE: pixels are always 32-bits wide, little endian 0x xx RR GG BB
// memory ordered BB GG RR xx
struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void      *Memory;
    int        Width;
    int        Height;
    int        Pitch;
};

global_variable bool                   GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;

struct win32_window_dimension {
    int Width;
    int Height;
};

#define X_INPUT_GET_STATE(name)                                                \
    DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
// this line ***
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return 0;
}

#define X_INPUT_SET_STATE(name)                                                \
    DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return 0;
}
// NOTE: hmmmm ... from xinput.h is just this two needed
// so we load them dinamically
// than you can access them trough pointer
//
// produce *** this line, so dont need it
// typedef DWORD WINAPI x_input_get_state(
//     DWORD         dwUserIndex, // Index of the gamer associated with the
//     device XINPUT_STATE *pState       // Receives the current state
// );
//
// again ***
// typedef DWORD WINAPI x_input_set_state(
//     DWORD dwUserIndex, // Index of the gamer associated with the device
//     XINPUT_VIBRATION
//         *pVibration // The vibration information to send to the controller
// );
// DAY6 29 min
//

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputGetState XInputGetState_;
#define XInputSetState XInputSetState_;
/*
 *
 * POINTER ALIASING
 *
 * when two pointers could point to same memory,
 * and the compiler doesn't know if a write to one of those
 * pointers might effect read from the other
 *
 *
 * */

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT                   WindowRect;

    GetClientRect(Window, &WindowRect);
    Result.Width  = WindowRect.right - WindowRect.left;
    Result.Height = WindowRect.bottom - WindowRect.top;

    return Result;
};

internal void RenderWeirdGradient(win32_offscreen_buffer Buffer,
                                  int                    XOffset,
                                  int                    YOffset)
{
    // NOTE: let see what optimazer does, when buffer will be passed by
    // value
    int Width = Buffer.Width;
    // NOTE: we need it for shifting byte by byte
    // because C it multiply by size of thing being pointed to
    // (if uint16...it will be moved by two )
    uint8 *Row = (uint8 *) Buffer.Memory; // get the row
    for (int Y = 0; Y < Buffer.Height; ++Y) {
        uint32 *Pixel = (uint32 *) Row; // get the pixel of the row
        for (int X = 0; X < Buffer.Width; ++X) {
            // xx RR GG BB xx RR GG BB
            uint8 Blue  = (uint8) (X + XOffset);
            uint8 Green = (uint8) (Y + YOffset);
            *Pixel++    = ((Blue << 8) | Green);
            // NOTE: *Pixel = *Pixel + 1*sizeof(uint32) // a that is 4

            // Playing
            // uint8 Blue  = X;
            // uint8 Green = Y;
            // uint8 Red   = 255;
            // *Pixel++    = ((X + Y) << 16) * XOffset;
        }
        // row is in bytes, Pitch is in bytes ... not in uint8
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

    Buffer->Info.bmiHeader.biSize  = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;

    // NOTE: when biHeight is negative, this is clue to Windows to treat
    // this bitmap
    //  as top-down ... meannig that first three bytes are RGB of top left
    //  pixel
    Buffer->Info.bmiHeader.biHeight      = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes      = 1;
    Buffer->Info.bmiHeader.biBitCount    = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    int BitesPerPixel                    = 4;
    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BitesPerPixel;
    Buffer->Memory =
        VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Buffer->Width * BitesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC                    DeviceContext,
                                         int                    WindowWidth,
                                         int                    WindowHeight,
                                         win32_offscreen_buffer Buffer)
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
                  WindowWidth,
                  WindowHeight,
                  0,
                  0,
                  Buffer.Width,
                  Buffer.Height,
                  Buffer.Memory,
                  &Buffer.Info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND   Window,
                                         UINT   Message,
                                         WPARAM WParam,
                                         LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message) {
        // case WM_SIZE: {
        // } break;

    case WM_DESTROY: {
        // TODO: handle this with message to user
        OutputDebugStringA("WM_DESTROY");
    } break;

    case WM_CLOSE: {
        GlobalRunning = false;
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
        PAINTSTRUCT PaintStruct;
        OutputDebugStringA("WM_PAINT");

        HDC DeviceContext = BeginPaint(Window, &PaintStruct);

        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        Win32DisplayBufferInWindow(
            DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer);
        // NOTE: povies windowsu ze vsetko co mal namalovat namaloval... all
        // was done
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
    // NOTE: stack overflow
    // uint8 BigOldHeavyBlockOfMemory[ 2 * 1024 * 1024] = {};

    WNDCLASS WindowClass = {};
    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
    WindowClass.style = CS_HREDRAW | CS_VREDRAW
                        | CS_OWNDC; // when resizing window refresh whole
                                    // window, not just changed parts
    WindowClass.lpfnWndProc   = Win32MainWindowCallback;
    WindowClass.hInstance     = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass)) {
        HWND Window = CreateWindowExA(0,
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
            // NOTE: (context1) this is usually not alloved by windows
            // unless you asked specifically for it
            // by this flag CS_OWNDC.. and we are not sharing context
            // with anyone
            HDC DeviceContext = GetDC(Window);
            int XOffset       = 0;
            int YOffset       = 0;
            GlobalRunning     = true;
            while (GlobalRunning) {
                MSG Message;
                // NOTE: if (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
                // we have a lot of messages ... if to while
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT) {
                        GlobalRunning = false;
                    }
                    // NOTE: actually translating keycodes
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                for (DWORD ControllerIndex = 0;
                     ControllerIndex < XUSER_MAX_COUNT;
                     ControllerIndex++) {

                    XINPUT_STATE ControllerState;
                    if (XInputGetState(ControllerIndex, &ControllerState)
                        == ERROR_SUCCESS) {
                        // NOTE: this controller is plugedin
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
                        bool Up   = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right =
                            (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back  = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftShoulder =
                            (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RithtShoulder =
                            (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;
                    }
                    else {
                        // NOTE: any other error... this controller is not
                        // available
                    }
                }

                // NOTE: (context2)  and you have to instatianed context in
                // every loop HDC DeviceContext = GetDC(Window);
                RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);
                win32_window_dimension Dimension =
                    Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(DeviceContext,
                                           Dimension.Width,
                                           Dimension.Height,
                                           GlobalBackBuffer);
                // NOTE: (context3) and you dont have to release it
                // ReleaseDC(Window, DeviceContext);
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
