#pragma once
#include <vcruntime_string.h>
class InputManager {
public:
	static void OnKeyDown(int key) { keysPressed[key] = true; }
	static void OnKeyUp(int key) { keysPressed[key] = false; }

	static bool IsKeyDown(const int key) { return keysPressed[key]; }
	static void ClearInputs() { memset(keysPressed, false, sizeof(keysPressed)); }
private:
	static bool keysPressed[349];
};