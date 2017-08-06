#include <stdint.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

// TODO: This is a global for now
global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void RenderWeirdGradient8(int XOffset, int YOffset)
{
  int Width = BitmapWidth;
  int Height = BitmapHeight;

  // NOTE: Draw some pixels on the screen
  int Pitch = Width * BytesPerPixel;
  // Strides don't always align at pixel boundaries?
  uint8_t *Row = (uint8_t *)BitmapMemory;
  for (int Y = 0; Y < BitmapHeight; Y++) {
    uint8_t *Pixel = (uint8_t *)Row;
    for (int X = 0; X < BitmapWidth; X++) {
      /* Pixel in memory: RR GG BB -- (HEX)
         Pixel in memory in Windows: BB GG RR --
         LITTLE ENDIAN ARCHITECTURE: 0x00BBGGRR */
      // NOTE: BLUE
      *Pixel = (uint8_t)(X + XOffset);
      ++Pixel;

      // NOTE: GREEN
      *Pixel = (uint8_t)(Y + YOffset);
      ++Pixel;

      // NOTE: RED
      *Pixel = 0;
      ++Pixel;

      // NOTE: PADDING
      *Pixel = 0;
      ++Pixel;
    }
    Row += Pitch;
  }
}

internal void RenderWeirdGradient32(int XOffset, int YOffset)
{
  int Width = BitmapWidth;
  int Height = BitmapHeight;
  int Pitch = Width * BytesPerPixel;

  uint8_t *Row = (uint8_t *)BitmapMemory;
  for (int Y = 0; Y < BitmapHeight; Y++) {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < BitmapWidth; X++) {
      uint8_t Blue = X + XOffset;
      uint8_t Green = Y + YOffset;
      uint8_t Red = 0;
      *Pixel++ = (Red << 16) | (Green << 8) | Blue;
    }
    Row += Pitch;
  }
}

// DIB = Device Independent Bitmap
internal void Win32ResizeDIBSection(int Width, int Height)
{
  // TODO: Bulletproof this
  // Maybe don't free first, free after, then free first if that fails

  if (BitmapMemory) {
    // NOTE: If dwSize is 0 it will free up all the memory
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = -BitmapHeight;  // Negative height -> top-down DIB
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;  // NOTE: For alignment reasons, RGB is 24
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  // NOTE: Difference between StretchDIBits and BitBlt
  // DeviceContext is not needed any more
  int BytesPerPixel = 4;
  int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  // TODO: Probably want to clear to #000000
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width,
                                int Height)
{
  // NOTE: Pitch
  // Value you would add to a pointer to move it from the current row to the next row
  int WindowWidth = ClientRect->right - ClientRect->left;
  int WindowHeight = ClientRect->bottom - ClientRect->top;

  StretchDIBits(DeviceContext,
                /* X, Y, Width, Height, X, Y, Width, Height, */
                0, 0, BitmapWidth, BitmapHeight, 0, 0, WindowWidth, WindowHeight, BitmapMemory,
                &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result = 0;

  switch (Message) {
    case WM_SIZE: {
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;
      Win32ResizeDIBSection(Width, Height);
    } break;

    case WM_DESTROY: {
      // TODO: Handle this with an error?
      Running = false;
    } break;

    case WM_CLOSE: {
      // TODO: Handle this with a message to the user?
      Running = false;
    } break;

    case WM_ACTIVATEAPP: {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;

    case WM_PAINT: {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);

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

  /* We don't really need to set the style */
  // WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = hInstance;
  // TODO: Set hIcon on WindowClass
  WindowClass.lpszClassName = "PiEngineWindowClass";

  if (RegisterClassA(&WindowClass)) {
    HWND WindowHandle = CreateWindowExA(
        0, WindowClass.lpszClassName, "Pi Engine", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

    if (WindowHandle) {
      Running = true;
      int XOffset = 0;
      int YOffset = 0;

      while (Running) {
        MSG Message;
        // NOTE: WindowHandle is 0 so we can retrieve messages from any of our windows
        while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
          if (Message.message == WM_QUIT) {
            Running = false;
          }
          TranslateMessage(&Message);
          DispatchMessageA(&Message);
        }

        RenderWeirdGradient32(XOffset, 0);

        HDC DeviceContext = GetDC(WindowHandle);
        RECT ClientRect;
        GetClientRect(WindowHandle, &ClientRect);
        int WindowWidth = ClientRect.right - ClientRect.left;
        int WindowHeight = ClientRect.bottom - ClientRect.top;
        Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
        ReleaseDC(WindowHandle, DeviceContext);

        ++XOffset;
      }
    } else {
      // TODO: Logging if CreateWindowEx fails
    }
  } else {
    // TODO: Logging if registering fails
  }

  return 0;
}