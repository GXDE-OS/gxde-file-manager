// SPDX-License-Identifier: GPL-3.0-or-later
#include "xutil.h"
namespace QtX11::utils {
void MoveWindow(QWidget* widget, QMouseEvent* event,
                const QRect& dragableRect) {
  return bernese::window_utils::MoveWindow(widget, event, dragableRect);
}
void ResizeWindow(QWidget* widget, QMouseEvent* event, int border_inner_size) {
  return bernese::window_utils::ResizeWindow(widget, event, border_inner_size);
}
void ShowMaximizedWindow(QWidget* widget) {
  return bernese::window_utils::ShowMaximizedWindow(widget);
}
void ShowMinimizedWindow(QWidget* widget) {
  return bernese::window_utils::ShowMinimizedWindow(widget);
}
void ShowNormalWindow(QWidget* widget) {
  return bernese::window_utils::ShowNormalWindow(widget);
}
void ToggleMaximizedWindow(QWidget* widget) {
  return bernese::window_utils::ToggleMaximizedWindow(widget);
}
bool setNormalCursor(QWidget* widget) {
  return bernese::window_utils::SetNormalCursor(widget);
}
bool setHandleCursor(QWidget* widget) {
  return bernese::window_utils::SetHandleCursor(widget);
}
bool setCursorByCursorType(QWidget* widget, unsigned int cursorType) {
  return bernese::window_utils::SetCursorByCursorType(widget, cursorType);
}
bool UpdateCursorShape(QWidget* widget, int x, int y, int border_inner_size) {
  return bernese::window_utils::UpdateCursorShape(widget, x, y,
                                                  border_inner_size);
}
}  // namespace QtX11::utils
