#ifndef VBA_WIN32_CHEAT_EXPLAIN_H
#define VBA_WIN32_CHEAT_EXPLAIN_H

#pragma once

class CWnd;

// Turns the parsed cheat lines [first, first + count) back into words: what
// each address means, which value is written, and which key or memory value
// gates it.  Returns an empty string when the range is out of bounds.
extern CString winExplainCheats(int first, int count);

// Shows the explanation in a read only window.
extern void winShowCheatExplanation(CWnd *parent, int first, int count);

#endif // VBA_WIN32_CHEAT_EXPLAIN_H
