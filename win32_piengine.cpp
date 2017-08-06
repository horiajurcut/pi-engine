#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

// TODO(horia): This is a global for now
global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

// DIB = Device Independent Bitmap
internal void Win32ResizeDIBSection(int Width, int Height)
{
  // TODO(horia): Bulletproof this
  // Maybe don't free first, free after, then free first if that fails

  // TODO(horia): Free it
  if (BitmapHandle) {
    DeleteObject(BitmapHandle);
  }

  if (!BitmapDeviceContext) {
    // TODO(horia): Should we recreate these under special circumstances?
    BitmapDeviceContext = CreateCompatibleDC(0);
  }

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = Width;
  BitmapInfo.bmiHeader.biHeight = Height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;  // NOTE: For alignment reasons, RGB is 24
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  BitmapHandle =
      CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}

internal void Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
  StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height, BitmapMemory, &BitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
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
      // TODO(horia): Handle this with an error?
      Running = false;
    } break;

    case WM_CLOSE: {
      // TODO(horia): Handle this with a message to the user?
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
      Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
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
  // TODO(horia): Set hIcon on WindowClass
  WindowClass.lpszClassName = "PiEngineWindowClass";

  if (RegisterClassA(&WindowClass)) {
    HWND WindowHandle = CreateWindowExA(
        0, WindowClass.lpszClassName, "Pi Engine", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

    if (WindowHandle) {
      Running = true;
      while (Running) {
        MSG Message;
        /* WindowHandle is 0 so we can retrieve messages from any of our windows */
        BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);

        if (MessageResult > 0) {
          TranslateMessage(&Message);
          DispatchMessageA(&Message);
        } else {
          break;
        }
      }
    } else {
      // TODO(horia): Logging if CreateWindowEx fails
    }
  } else {
    // TODO(horia): Logging if registering fails
  }

  return 0;
}
