#include <stdint.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

struct win32_offscreen_buffer {
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
};

struct win32_window_dimension {
  int Width;
  int Height;
};

// TODO: This is a global for now
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;

internal void RenderWeirdGradient32(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
  uint8_t *Row = (uint8_t *)Buffer.Memory;
  for (int Y = 0; Y < Buffer.Height; Y++) {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < Buffer.Width; X++) {
      uint8_t Blue = X + XOffset;
      uint8_t Green = Y + YOffset;
      uint8_t Red = 0;
      *Pixel++ = (Red << 16) | (Green << 8) | Blue;
    }
    Row += Buffer.Pitch;
  }
}

win32_window_dimension Win32GetWindowDimension(HWND Window)
{
  win32_window_dimension Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);

  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return Result;
}

// DIB = Device Independent Bitmap
internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
  // TODO: Bulletproof this
  // Maybe don't free first, free after, then free first if that fails

  if (Buffer->Memory) {
    // NOTE: If dwSize is 0 it will free up all the memory
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;  // Negative height -> top-down DIB
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;  // NOTE: For alignment reasons, RGB is 24
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BytesPerPixel = 4;
  Buffer->Pitch = Buffer->Width * BytesPerPixel;

  int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight,
                                         win32_offscreen_buffer Buffer)
{
  // TODO: Aspect ratio correction
  StretchDIBits(DeviceContext,
                /* X, Y, Width, Height, X, Y, Width, Height, */
                0, 0, WindowWidth, WindowHeight, 0, 0, Buffer.Width, Buffer.Height, Buffer.Memory,
                &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result = 0;

  switch (Message) {
    case WM_DESTROY: {
      // TODO: Handle this with an error?
      GlobalRunning = false;
    } break;

    case WM_CLOSE: {
      // TODO: Handle this with a message to the user?
      GlobalRunning = false;
    } break;

    case WM_ACTIVATEAPP: {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;

    case WM_PAINT: {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
                                 GlobalBackBuffer);
      EndPaint(Window, &Paint);
    } break;

    default: {
      /* Let Windows catch these messages */
      Result = DefWindowProc(Window, Message, wParam, lParam);
    } break;
  }

  return Result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  /* Initializes all the elements of the struct with 0 */
  WNDCLASS WindowClass = {};

  Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

  WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = hInstance;
  // TODO: Set hIcon on WindowClass
  WindowClass.lpszClassName = "PiEngineWindowClass";

  if (RegisterClassA(&WindowClass)) {
    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Pi Engine",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

    if (Window) {
      // NOTE: Since we specified CS_OWNDC, we only need to get the Device Context once
      HDC DeviceContext = GetDC(Window);

      int XOffset = 0;
      int YOffset = 0;

      GlobalRunning = true;
      while (GlobalRunning) {
        MSG Message;
        // NOTE: Window is 0 so we can retrieve messages from any of our windows
        while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
          if (Message.message == WM_QUIT) {
            GlobalRunning = false;
          }
          TranslateMessage(&Message);
          DispatchMessageA(&Message);
        }

        RenderWeirdGradient32(GlobalBackBuffer, XOffset, YOffset);
        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
                                   GlobalBackBuffer);
        ReleaseDC(Window, DeviceContext);

        ++XOffset;
        ++YOffset;
      }
    } else {
      // TODO: Logging if CreateWindowEx fails
    }
  } else {
    // TODO: Logging if registering fails
  }

  return 0;
}