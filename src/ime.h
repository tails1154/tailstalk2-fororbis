#pragma once

#include <string>

// Open the console's on-screen keyboard to edit a single line of text.
// `text` is used as the starting value and receives the result.
// Returns false when the user cancels.
bool EditTextWithKeyboard(const wchar_t* title, bool hidden, std::string& text, int maxLength);
