#include <windows.h>

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result = 0;

  switch (Message) {
    case WM_SIZE: {
      OutputDebugStringA("WM_SIZE\n");
    } break;

    case WM_DESTROY: {
      OutputDebugStringA("WM_DESTROY\n");
    } break;

    case WM_CLOSE: {
      OutputDebugStringA("WM_CLOSE\n");
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
      PatBlt(DeviceContext, X, Y, Width, Height, WHITENESS);
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
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = hInstance;
  // TODO(horia): Set hIcon on WindowClass
  WindowClass.lpszClassName = "PiEngineWindowClass";

  if (RegisterClassA(&WindowClass)) {
    HWND WindowHandle = CreateWindowExA(
        0, WindowClass.lpszClassName, "Pi Engine", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

    if (WindowHandle) {
      for (;;) {
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
