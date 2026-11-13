#include "Core/Input.h"

using namespace GEM;

void Input::SetKeyDown(WPARAM key)
{
	if (key < 256) {
		keys[key] = true;
	}
}

void Input::SetKeyUp(WPARAM key)
{
	if (key < 256) {
		keys[key] = false;
	}
}

void Input::Clear()
{
	for (bool& key : keys) {
		key = false;
	}

	mouse.left = false;
	mouse.right = false;
}

bool Input::isKeyDown(int key)
{
	return key >= 0 && key < 256 && keys[key];
}

void Input::setMouseX(int xPosition)
{
	mouse.x = xPosition;
}

void Input::setMouseY(int yPosition)
{
	mouse.y = yPosition;
}

int Input::getMouseX()
{
	return mouse.x;
}

int Input::getMouseY()
{
	return mouse.y;
}

void Input::setLeftMouse(bool down)
{
	mouse.left = down;
}

void Input::setRightMouse(bool down)
{
	mouse.right = down;
}

bool Input::isLeftMouseDown()
{
	return mouse.left;
}

bool Input::isRightMouseDown()
{
	return mouse.right;
}

void Input::setMouseActive(bool active)
{
	mouse.isActive = active;
}
bool Input::isMouseActive()
{
	return mouse.isActive;
}
