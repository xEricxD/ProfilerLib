#include "ImGuiExtended.h"

bool ImGui_ClipRect(ImVec2& rectStart, ImVec2& rectEnd, ImVec2 clipStart, ImVec2 clipEnd)
{
  // Full clip
  if (rectEnd.x < clipStart.x || rectStart.x > clipEnd.x)
    return false;
  if (rectEnd.y < clipStart.y || rectStart.y > clipEnd.y)
    return false;

  ImVec2 clippedRectStart = rectStart;
  ImVec2 clippedRectEnd = rectEnd;

  if (rectStart.x < clipStart.x) clippedRectStart.x = clipStart.x;
  if (rectStart.y < clipStart.y) clippedRectStart.y = clipStart.y;
  if (rectEnd.x > clipEnd.x) clippedRectEnd.x = clipEnd.x;
  if (rectEnd.y > clipEnd.y) clippedRectEnd.y = clipEnd.y;

  rectStart = clippedRectStart;
  rectEnd = clippedRectEnd;

  return true;
}

bool ImGui_IsItemHovered(const ImVec2& rectStart, const ImVec2& rectEnd)
{
  ImVec2 mp = ImGui::GetMousePos();

  if (mp.x > rectStart.x && mp.x < rectEnd.x &&
      mp.y > rectStart.y && mp.y < rectEnd.y)
    return true;

  return false;
}
