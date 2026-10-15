#pragma once

namespace GEM{
    /// @brief Stores the current keyboard and mouse state for a window.
    class Input
    {
        /// Mouse struct, stores position, button state, and active status.
        struct Mouse
        {
            int x, y;
            bool left, right, isActive;
        };

    public:
        void SetKeyDown(WPARAM key);	///< Sets key down value for specified key
        void SetKeyUp(WPARAM key);		///< Sets key up value for specified key
        void Clear();				///< Clears all keyboard and mouse button state

        bool isKeyDown(int key);		///< Check if specified key is down
        void setMouseX(int xPosition);	///< Set mouse X-axis position
        void setMouseY(int yPosition);	///< Set mouse y-axis position
        int getMouseX();				///< Get mouse x-axis position
        int getMouseY();				///< Get mouse y-axis position
        void setLeftMouse(bool down);	///< Set left mouse click
        void setRightMouse(bool down);	///< Set right mouse click
        bool isLeftMouseDown();			///< Check left mouse click
        bool isRightMouseDown();		///< Check right mouse click
        void setMouseActive(bool active);	///< Set monuse in/active
        bool isMouseActive();			///< Check if mouse is in/active

    private:
        bool keys[256]{};			///< Array for storing key states
        Mouse mouse{ 0, 0, false, false, false };	///< Mouse state variable

    };
}

