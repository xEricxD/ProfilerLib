#ifndef _IMGUI_EXTENDED_H
#define _IMGUI_EXTENDED_H
#include "imgui\imgui.h"

// This class contains some extra clipping / rendering functions for imgui

/*
  * Clips a rectangle
  * returns:  true if rectangle needs to be drawn
  *           false if rectangle is completely out of clipping rect
*/
bool ImGui_ClipRect(ImVec2& rectStart, ImVec2& rectEnd, ImVec2 clipStart, ImVec2 clipEnd);

/*
* Check if an item rect is hovered by the cursor
*/
bool ImGui_IsItemHovered(const ImVec2& rectStart, const ImVec2& rectEnd);

#endif