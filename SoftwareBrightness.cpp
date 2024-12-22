#include <iostream>
#include <windows.h>
#include "EzCpp/EzTokens.h"
#include "EzCpp/EzHelper.h"

//Function Prototypes
void BrightnessUp();
void BrightnessDown();
int main(int argc, char** argv);

// Constants
constexpr BYTE BrightnessStep = 5;
constexpr wchar_t WindowClassName[] = L"SoftwareBrightnessWindowClass";

constexpr int BrightnessUpHotkeyID = 0;
constexpr int BrightnessDownHotkeyID = 1;

// Globals
HHOOK keyboardHook;
int brightness = 130;

int windowCount = 0;
HWND windows[16]; // Max of 16 monitors before this program dies.

void BrightnessUp() {
	brightness -= BrightnessStep;
	if (brightness < 0) {
		brightness = 0;
	}

	for (int i = 0; i < windowCount; i++)
	{
		SetLayeredWindowAttributes(windows[i], RGB(0, 0, 0), (BYTE)brightness, LWA_ALPHA);
	}

	std::cout << brightness << std::endl;
}
void BrightnessDown() {
	brightness += BrightnessStep;

	if (brightness > 255) {
		brightness = 255;
	}

	for (int i = 0; i < windowCount; i++)
	{
		SetLayeredWindowAttributes(windows[i], RGB(0, 0, 0), (BYTE)brightness, LWA_ALPHA);
	}

	std::cout << brightness << std::endl;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	// Create a window for this monitor
	HWND hwnd = CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, // Optional window styles
		WindowClassName, // Use the DefaultD2DClass which must be registerd in advance.
		L"Software Brightness Screen Cover Window", // Set the window title based on function input.
		WS_POPUP | WS_VISIBLE, // Window style
		lprcMonitor->left, // Set the X position of our window.
		lprcMonitor->top, // Set the Y position of our window.
		lprcMonitor->right - lprcMonitor->left, // Set the width of our window.
		lprcMonitor->bottom - lprcMonitor->top, // Set the height of our window.
		nullptr, // Set the parent window to nullptr.
		nullptr, // Set the target menu to nullptr.
		GetModuleHandle(nullptr), // Set the instance handle to the instance handle for the current process.
		nullptr // Set additional data to nullptr.
	);

	// Set the window to the current brightness level
	SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), (BYTE)brightness, LWA_ALPHA);

	// Make the window topmost and immovable
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// Show the window
	ShowWindow(hwnd, SW_SHOWNORMAL);

	// Add the window to the windows array
	windows[windowCount] = hwnd;
	windowCount++;

	return TRUE;
}

int main(int argc, char** argv) {
	HANDLE currentToken = EzOpenCurrentToken();
	BOOL isElevated = EzGetTokenElevation(currentToken);
	BOOL hasUIAccess = EzGetTokenUIAccess(currentToken);
	EzCloseHandleSafely(currentToken);

	if (!isElevated) {
		EzLaunchWithUAC();
		return 0;
	}
	if (!hasUIAccess) {
		HANDLE tokenWithUIAccess = EzDuplicateCurrentToken();
		EzGrantUIAccessToToken(tokenWithUIAccess);
		EzLaunchAsToken(tokenWithUIAccess);
		return 0;
	}

	// Register hotkeys for brightness up/down events.
	RegisterHotKey(NULL, BrightnessUpHotkeyID, MOD_CONTROL | MOD_ALT, VK_UP);
	RegisterHotKey(NULL, BrightnessDownHotkeyID, MOD_CONTROL | MOD_ALT, VK_DOWN);

	// Register window class for software brightness windows.
	WNDCLASS wc = {};

	wc.style = CS_HREDRAW | CS_VREDRAW; // Redraw windows with this class on horizontal or vertical changes.
	wc.lpfnWndProc = DefWindowProcW; // Define the window event handler.
	wc.cbClsExtra = 0; // Allocate 0 extra bytes after the class declaration.
	wc.cbWndExtra = 0; // Allocate 0 extra bytes after windows of this class.
	wc.hInstance = GetModuleHandle(nullptr); // The WndProc function is in the current hInstance.
	wc.hIcon = nullptr; // Use the default icon.
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW); // Load cursor with an HInstance of nullptr to load from system cursors.
	wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));; // A background brush of null specifies user implamented background painting.
	wc.lpszMenuName = nullptr; // Windows of this class have no default menu.
	wc.lpszClassName = WindowClassName; // Set the window class name to DefaultD2DClass.

	RegisterClass(&wc);

	// Enumerate through the list of monitors creating and showing a window for each.
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

	// Run message pump
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_HOTKEY)
		{
			if (msg.wParam == BrightnessUpHotkeyID) {
				BrightnessUp();
			}
			else if (msg.wParam == BrightnessDownHotkeyID) {
				BrightnessDown();
			}
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}