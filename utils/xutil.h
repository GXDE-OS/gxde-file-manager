// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <bernese/window_utils.h>
namespace QtX11::utils {
void MoveWindow(QWidget* widget, QMouseEvent* event, const QRect& dragableRect);
void ResizeWindow(QWidget* widget, QMouseEvent* event,
                  int border_inner_size = 6);
void ShowMaximizedWindow(QWidget* widget);
void ShowMinimizedWindow(QWidget* widget);
void ShowNormalWindow(QWidget* widget);
void ToggleMaximizedWindow(QWidget* widget);
bool setNormalCursor(QWidget* widget);
bool setHandleCursor(QWidget* widget);
bool setCursorByCursorType(QWidget* widget, unsigned int cursorType);
bool UpdateCursorShape(QWidget* widget, int x, int y,
                       int border_inner_size = 6);
}  // namespace QtX11::utils
