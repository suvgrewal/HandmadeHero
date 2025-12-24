#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel = 4;
    
};

// TODO: global for now
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

struct win32_window_dimension
{
    int Width;
    int Height;
};

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
    // TODO: See what optimizer does if passing by value

    // Bytes to move
    uint8* Row = (uint8 *)Buffer.Memory;

    for (int Y = 0;
        Y < Buffer.Height;
        Y++)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0;
            X < Buffer.Width;
            X++)
        {
            uint8 Blue  = (uint8)(X + XOffset);
            uint8 Green = (uint8)(Y + YOffset);
            uint8 Red   = 0;
            
            *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
        }

        Row += Buffer.Pitch;
    }
}

internal void 
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO: Bulletproof this.
    // Don't free first, free after, then free first if that fails.

    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;
    
    // NOTE: When biHeight field is negative, this is the clue to Windows to treat
    // this bitmap as top-down, not bottom-up, meaning that first three bytes of 
    // the image are the color 
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
	// StretchDIBits requires memory to be allocated by the caller, BitBlt allocates internally
    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	// TODO: Probably clear to black

    Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight,
                           win32_offscreen_buffer Buffer,
                           int X, int Y, int Width, int Height)
{
    // TODO: Aspect ratio correction
    StretchDIBits(DeviceContext,
        /*
        X, Y, Width, Height,
        X, Y, Width, Height,
        */
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Memory,
        &Buffer.Info,
        DIB_RGB_COLORS, SRCCOPY);

}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_SIZE:
        {
        } break;

        case WM_CLOSE:
        {
			// TODO: Handle this with a message to the user
            PostQuitMessage(0);
            Running = false;
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_DESTROY:
        {
            // TODO: Handle this as an error - recreate window

            Running = false;
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;

            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            
            Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
                                       GlobalBackbuffer, X, Y, Width, Height);

            EndPaint(Window, &Paint);
        } break;
        default:
        {
            OutputDebugStringA("Default\n");
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR     CommandLine,
    int       ShowCode
)
{
    //MessageBox(0, "This is Handmade Hero.", "Handmade Hero", MB_OK|MB_ICONINFORMATION);

    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClassA(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "Handmade Hero",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);

        if (Window)
        {
            Running = true;

            int XOffset = 0;
            int YOffset = 0;

            while (Running)
            {
                MSG Message;

                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    // process message and flush out queue
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);
                HDC DeviceContext = GetDC(Window);

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);

                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
                                           GlobalBackbuffer, 0, 0, Dimension.Width, Dimension.Height);

                ReleaseDC(Window, DeviceContext);

                XOffset++;
                YOffset++;
            }
        }
        else
        {
            // TODO: logging
        }
    }
    else
    {
        // TODO: Logging


    }

    return 0;
}