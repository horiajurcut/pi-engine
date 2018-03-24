#include <stdint.h>
#include <windows.h>
#include <xinput.h>

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

// NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);

X_INPUT_GET_STATE(XInputGetStateStub)
{
  return 0;
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub)
{
  return 0;
}

global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void)
{
  HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");

  if (XInputLibrary) {
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
  }
}

internal void RenderWeirdGradient32(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
  uint8_t *Row = (uint8_t *)Buffer->Memory;
  for (int Y = 0; Y < Buffer->Height; Y++) {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < Buffer->Width; X++) {
      uint8_t Blue = X + XOffset;
      uint8_t Green = Y + YOffset;
      uint8_t Red = 0;
      *Pixel++ = (Red << 16) | (Green << 8) | Blue;
    }
    Row += Buffer->Pitch;
  }
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
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

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext,
                                         int WindowWidth, int WindowHeight)
{
  // TODO: Aspect ratio correction
  StretchDIBits(DeviceContext,
                /* X, Y, Width, Height, X, Y, Width, Height, */
                0, 0, WindowWidth, WindowHeight, 0, 0, Buffer->Width, Buffer->Height,
                Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
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

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      uint32_t VirtualKeyCode = wParam;
      bool WasDown = (lParam & (1 << 30)) != 0;
      bool IsDown = (lParam & (1 << 31)) == 0;

      if (VirtualKeyCode == 'W') {
        OutputDebugStringA("UP");
      } else if (VirtualKeyCode == 'S') {
        OutputDebugStringA("DOWN");
      } else if (VirtualKeyCode == 'A') {
        OutputDebugStringA("LEFT");
      } else if (VirtualKeyCode == 'D') {
        OutputDebugStringA("RIGHT");
      } else if (VirtualKeyCode == 'Q') {
        OutputDebugStringA("Q");
      } else if (VirtualKeyCode == 'E') {
        OutputDebugStringA("E");
      } else if (VirtualKeyCode == VK_UP) {
        OutputDebugStringA("ARROW UP");
      } else if (VirtualKeyCode == VK_DOWN) {
        OutputDebugStringA("ARROW DOWN");
      } else if (VirtualKeyCode == VK_LEFT) {
        OutputDebugStringA("ARROW LEFT");
      } else if (VirtualKeyCode == VK_RIGHT) {
        OutputDebugStringA("ARROW RIGHT");
      } else if (VirtualKeyCode == VK_ESCAPE) {
        OutputDebugStringA("ESCAPE");
      } else if (VirtualKeyCode == VK_SPACE) {
        OutputDebugStringA("SPACE: ");
        if (WasDown) {
          OutputDebugStringA("WasDown ");
        }
        if (IsDown) {
          OutputDebugStringA("IsDown ");
        }
        OutputDebugStringA("\n");
      }
    } break;

    case WM_PAINT: {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width,
                                 Dimension.Height);
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
  // NOTE: Load xinput library
  Win32LoadXInput();

  /* Initializes all the elements of the struct with 0 */
  WNDCLASSA WindowClass = {};

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

        // TODO: Should we pull this more frequently?
        DWORD ControllerStateResult;
        for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++) {
          XINPUT_STATE ControllerState;
          ZeroMemory(&ControllerState, sizeof(XINPUT_STATE));
          ControllerStateResult = XInputGetState(ControllerIndex, &ControllerState);
          if (ControllerStateResult == ERROR_SUCCESS) {
            // NOTE: Controller is connected
            XINPUT_GAMEPAD *GamePad = &ControllerState.Gamepad;

            bool DPadUp = GamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
            bool DPadDown = GamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
            bool DPadLeft = GamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
            bool DPadRight = GamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
            bool StartButton = GamePad->wButtons & XINPUT_GAMEPAD_START;
            bool BackButton = GamePad->wButtons & XINPUT_GAMEPAD_BACK;
            bool LeftShoulder = GamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
            bool RightShoulder = GamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
            bool AButton = GamePad->wButtons & XINPUT_GAMEPAD_A;
            bool BButton = GamePad->wButtons & XINPUT_GAMEPAD_B;
            bool XButton = GamePad->wButtons & XINPUT_GAMEPAD_X;
            bool YButton = GamePad->wButtons & XINPUT_GAMEPAD_Y;

            int16_t ThumbLX = GamePad->sThumbLX;
            int16_t ThumbLY = GamePad->sThumbLY;

            // NOTE: Small test to see if the controller code is working
            XOffset += ThumbLX >> 12;
            YOffset += ThumbLY >> 12;

            if (AButton) {
              YOffset += 2;
            }
          } else {
            // NOTE: Controller is not connected
          }
        }
        // XINPUT_VIBRATION Vibration;
        // Vibration.wLeftMotorSpeed = 60000;
        // Vibration.wRightMotorSpeed = 60000;
        // XInputSetState(0, &Vibration);

        RenderWeirdGradient32(&GlobalBackBuffer, XOffset, YOffset);
        win32_window_dimension Dimension = Win32GetWindowDimension(Window);

        // NOTE: Move the squares on screen
        ++XOffset;
        ++YOffset;

        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width,
                                   Dimension.Height);
      }
      // TODO: We may not need to release the DC at all?
      ReleaseDC(Window, DeviceContext);
    } else {
      // TODO: Logging if CreateWindowEx fails
    }
  } else {
    // TODO: Logging if registering fails
  }

  return 0;
}
