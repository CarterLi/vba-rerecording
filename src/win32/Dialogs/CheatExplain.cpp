// CheatExplain.cpp : turns parsed cheat lines back into words.
//
// The engine already decoded every code when it was added: CheatsData carries
// the instruction type in `size` and the operands in `address` / `value`.  So
// nothing here decrypts anything; it only names what the interpreter does with
// those operands (see the switch in gba/V8/GBACheats.cpp, cheatsCheckKeys).

#include "../stdafx.h"
#include "../resource.h"
#include "CheatExplain.h"
#include "../WinResUtil.h"
#include "../WinText.h"

#include "../../gba/GBACheats.h"
#include "../../gba/GBACheatTypes.h"

namespace
{

CString winExplainString(UINT id)
{
	return winResLoadString(id);
}

// "the value at 0x02037368, 16-bit" - the thing an instruction reads or writes.
CString winExplainTarget(u32 address, int bits)
{
	CString text;
	text.Format(winExplainString(IDS_CHEAT_EXPLAIN_TARGET), address, bits);
	return text;
}

CString winExplainPointerTarget(u32 address, int bits)
{
	CString text;
	text.Format(winExplainString(IDS_CHEAT_EXPLAIN_POINTER_TARGET), address, bits);
	return text;
}

// A value written the width of the instruction, so it lines up with the digits
// of the code the user is looking at.
CString winExplainValue(u32 value, int bits)
{
	CString text;
	if (bits == 8)
		text.Format(_T("0x%02X"), value & 0xFF);
	else if (bits == 16)
		text.Format(_T("0x%04X"), value & 0xFFFF);
	else
		text.Format(_T("0x%08X"), value);
	return text;
}

CString winExplainFamily(int code)
{
	if (code == 512)
		return winExplainString(IDS_CHEAT_EXPLAIN_TYPE_CODEBREAKER);
	if (code == 256 || code == 257)
		return winExplainString(IDS_CHEAT_EXPLAIN_TYPE_GAMESHARK);
	return winExplainString(IDS_CHEAT_EXPLAIN_TYPE_CODE);
}

// GBA key masks, in the order a player reads them.
CString winExplainKeys(u32 mask)
{
	static const u32    bits[]  = { 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200 };
	static const TCHAR *names[] = { _T("A"), _T("B"), _T("Select"), _T("Start"),
	                                _T("Right"), _T("Left"), _T("Up"), _T("Down"),
	                                _T("R"), _T("L") };

	CString text;
	for (int i = 0; i < (int)(sizeof(bits) / sizeof(bits[0])); i++)
	{
		if (!(mask & bits[i]))
			continue;
		if (!text.IsEmpty())
			text += _T("+");
		text += names[i];
	}

	if (text.IsEmpty())
		text = winExplainString(IDS_CHEAT_EXPLAIN_KEYS_OTHER);
	return text;
}

CString winExplainWrite(u32 address, int bits, u32 value)
{
	CString text;
	text.Format(winExplainString(IDS_CHEAT_EXPLAIN_WRITE),
	            (LPCTSTR)winExplainTarget(address, bits),
	            (LPCTSTR)winExplainValue(value, bits));
	return text;
}

// "if <target> <op> 0x1234, run the code below"
CString winExplainCondition(UINT id, u32 address, int bits, u32 value)
{
	CString text;
	text.Format(winExplainString(id), (LPCTSTR)winExplainTarget(address, bits),
	            (LPCTSTR)winExplainValue(value, bits));
	return text;
}

// Some conditionals do not skip a line, they switch the rest of the cheat off.
CString winExplainConditionTail(UINT id, u32 address, int bits, u32 value)
{
	CString text = winExplainCondition(id, address, bits, value);
	text += _T("\r\n");
	text += winExplainString(IDS_CHEAT_EXPLAIN_ELSE_REST);
	return text;
}

// Describes cheatsList[index] and reports how many of the lines after it are
// that instruction's data rather than instructions of their own.
CString winExplainOne(int index, int &extraLines)
{
	const CheatsData &cheat = cheatsList[index];
	extraLines = 0;

	switch (cheat.size)
	{
	// ---- plain writes ------------------------------------------------
	case INT_8_BIT_WRITE:
		return winExplainWrite(cheat.address, 8, cheat.value);
	case INT_16_BIT_WRITE:
		return winExplainWrite(cheat.address, 16, cheat.value);
	case INT_32_BIT_WRITE:
		return winExplainWrite(cheat.address, 32, cheat.value);

	// These two patch the ROM when the address is inside it.
	case CHEATS_16_BIT_WRITE:
	case CHEATS_32_BIT_WRITE:
	{
		if ((cheat.address >> 24) >= 0x08)
		{
			int bits = (cheat.size == CHEATS_16_BIT_WRITE) ? 16 : 32;
			CString text;
			text.Format(winExplainString(IDS_CHEAT_EXPLAIN_ROM_PATCH),
			            (LPCTSTR)winExplainTarget(cheat.address, bits),
			            (LPCTSTR)winExplainValue(cheat.value, bits));
			return text;
		}

		return winExplainWrite(cheat.address,
		                       cheat.size == CHEATS_16_BIT_WRITE ? 16 : 32,
		                       cheat.value);
	}

	// ---- writes gated on L and R -------------------------------------
	case GSA_8_BIT_GS_WRITE:
	case GSA_16_BIT_GS_WRITE:
	case GSA_32_BIT_GS_WRITE:
	{
		int bits = 8;
		if (cheat.size == GSA_16_BIT_GS_WRITE)
			bits = 16;
		else if (cheat.size == GSA_32_BIT_GS_WRITE)
			bits = 32;

		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_WRITE_KEY),
		            (LPCTSTR)winExplainTarget(cheat.address, bits),
		            (LPCTSTR)winExplainValue(cheat.value, bits));
		return text;
	}

	// The line below carries the value; the engine reads it as data.
	case GSA_8_BIT_GS_WRITE2:
	case GSA_16_BIT_GS_WRITE2:
	case GSA_32_BIT_GS_WRITE2:
		extraLines = 1;
		return winExplainString(IDS_CHEAT_EXPLAIN_WRITE_KEY2);

	case GSA_16_BIT_ROM_PATCH2C:
	case GSA_16_BIT_ROM_PATCH2D:
	case GSA_16_BIT_ROM_PATCH2E:
	case GSA_16_BIT_ROM_PATCH2F:
	{
		extraLines = 1;
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_ROM_PATCH2),
		            ((cheat.value & 0x00FFFFFF) << 1) | 0x08000000);
		return text;
	}

	case GSA_16_BIT_ROM_PATCH:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_ROM_PATCH),
		            (LPCTSTR)winExplainTarget(cheat.address, 16),
		            (LPCTSTR)winExplainValue(cheat.value, 16));
		return text;
	}

	// ---- conditions: equal / not equal -------------------------------
	case CBA_IF_TRUE:
	case GSA_16_BIT_IF_TRUE2:
	case GSA_16_BIT_IF_TRUE3:
		if (cheat.size == GSA_16_BIT_IF_TRUE3)
			return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_EQ, cheat.address, 16, cheat.value);
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_EQ, cheat.address, 16, cheat.value);

	case GSA_8_BIT_IF_TRUE:
	case GSA_8_BIT_IF_TRUE2:
	case GSA_8_BIT_IF_TRUE3:
		if (cheat.size == GSA_8_BIT_IF_TRUE3)
			return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_EQ, cheat.address, 8, cheat.value);
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_EQ, cheat.address, 8, cheat.value);

	case GSA_32_BIT_IF_TRUE:
	case GSA_32_BIT_IF_TRUE2:
	case GSA_32_BIT_IF_TRUE3:
		if (cheat.size == GSA_32_BIT_IF_TRUE3)
			return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_EQ, cheat.address, 32, cheat.value);
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_EQ, cheat.address, 32, cheat.value);

	case CBA_IF_FALSE:
	case GSA_16_BIT_IF_FALSE2:
	case GSA_16_BIT_IF_FALSE3:
		if (cheat.size == GSA_16_BIT_IF_FALSE3)
			return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_NE, cheat.address, 16, cheat.value);
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_NE, cheat.address, 16, cheat.value);

	case GSA_8_BIT_IF_FALSE:
	case GSA_8_BIT_IF_FALSE2:
	case GSA_8_BIT_IF_FALSE3:
		if (cheat.size == GSA_8_BIT_IF_FALSE3)
			return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_NE, cheat.address, 8, cheat.value);
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_NE, cheat.address, 8, cheat.value);

	case GSA_32_BIT_IF_FALSE:
	case GSA_32_BIT_IF_FALSE2:
	case GSA_32_BIT_IF_FALSE3:
		if (cheat.size == GSA_32_BIT_IF_FALSE3)
			return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_NE, cheat.address, 32, cheat.value);
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_NE, cheat.address, 32, cheat.value);

	// ---- conditions: greater / less ----------------------------------
	case CBA_GT:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_GT, cheat.address, 16, cheat.value);
	case CBA_LT:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_LT, cheat.address, 16, cheat.value);
	case GSA_16_BIT_IF_HIGHER_OR_EQ_U:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_GE, cheat.address, 16, cheat.value);
	case GSA_16_BIT_IF_LOWER_OR_EQ_U:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_LE, cheat.address, 16, cheat.value);

	case GSA_8_BIT_IF_HIGHER_U:
	case GSA_8_BIT_IF_HIGHER_S:
	case GSA_8_BIT_IF_HIGHER_U2:
	case GSA_8_BIT_IF_HIGHER_S2:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_GT, cheat.address, 8, cheat.value);
	case GSA_8_BIT_IF_HIGHER_U3:
	case GSA_8_BIT_IF_HIGHER_S3:
		return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_GT, cheat.address, 8, cheat.value);

	case GSA_16_BIT_IF_HIGHER_U:
	case GSA_16_BIT_IF_HIGHER_S:
	case GSA_16_BIT_IF_HIGHER_U2:
	case GSA_16_BIT_IF_HIGHER_S2:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_GT, cheat.address, 16, cheat.value);
	case GSA_16_BIT_IF_HIGHER_U3:
	case GSA_16_BIT_IF_HIGHER_S3:
		return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_GT, cheat.address, 16, cheat.value);

	case GSA_32_BIT_IF_HIGHER_U:
	case GSA_32_BIT_IF_HIGHER_S:
	case GSA_32_BIT_IF_HIGHER_U2:
	case GSA_32_BIT_IF_HIGHER_S2:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_GT, cheat.address, 32, cheat.value);
	case GSA_32_BIT_IF_HIGHER_U3:
	case GSA_32_BIT_IF_HIGHER_S3:
		return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_GT, cheat.address, 32, cheat.value);

	case GSA_8_BIT_IF_LOWER_U:
	case GSA_8_BIT_IF_LOWER_S:
	case GSA_8_BIT_IF_LOWER_U2:
	case GSA_8_BIT_IF_LOWER_S2:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_LT, cheat.address, 8, cheat.value);
	case GSA_8_BIT_IF_LOWER_U3:
	case GSA_8_BIT_IF_LOWER_S3:
		return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_LT, cheat.address, 8, cheat.value);

	case GSA_16_BIT_IF_LOWER_U:
	case GSA_16_BIT_IF_LOWER_S:
	case GSA_16_BIT_IF_LOWER_U2:
	case GSA_16_BIT_IF_LOWER_S2:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_LT, cheat.address, 16, cheat.value);
	case GSA_16_BIT_IF_LOWER_U3:
	case GSA_16_BIT_IF_LOWER_S3:
		return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_LT, cheat.address, 16, cheat.value);

	case GSA_32_BIT_IF_LOWER_U:
	case GSA_32_BIT_IF_LOWER_S:
	case GSA_32_BIT_IF_LOWER_U2:
	case GSA_32_BIT_IF_LOWER_S2:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_LT, cheat.address, 32, cheat.value);
	case GSA_32_BIT_IF_LOWER_U3:
	case GSA_32_BIT_IF_LOWER_S3:
		return winExplainConditionTail(IDS_CHEAT_EXPLAIN_IF_LT, cheat.address, 32, cheat.value);

	// ---- conditions: bit mask ----------------------------------------
	case GSA_8_BIT_IF_AND:
	case GSA_8_BIT_IF_AND2:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_AND, cheat.address, 8, cheat.value);
	case GSA_8_BIT_IF_AND3:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_AND_NOT, cheat.address, 8, cheat.value);

	case GSA_16_BIT_IF_AND:
	case GSA_16_BIT_IF_AND2:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_AND, cheat.address, 16, cheat.value);
	case GSA_16_BIT_IF_AND3:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_AND_NOT, cheat.address, 16, cheat.value);

	case GSA_32_BIT_IF_AND:
	case GSA_32_BIT_IF_AND2:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_AND, cheat.address, 32, cheat.value);
	case GSA_32_BIT_IF_AND3:
	case GSA_ALWAYS3:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_AND_NOT, cheat.address, 32, cheat.value);

	// ---- conditions: keys --------------------------------------------
	case CBA_IF_KEYS_PRESSED:
	{
		// The engine keeps only the comparison mode in the address: 0x10 means
		// every key has to be down, 0x20 means any one of them.
		UINT id = ((cheat.address & 0xF0) == 0x10) ? IDS_CHEAT_EXPLAIN_IF_KEY_ALL
		                                          : IDS_CHEAT_EXPLAIN_IF_KEY_ANY;
		CString text;
		text.Format(winExplainString(id), (LPCTSTR)winExplainKeys(cheat.value));
		return text;
	}

	// These skip a number of lines rather than one; the count lives in a field
	// the V7 core does not have, so the wording stays vague on purpose.
	case GSA_16_BIT_MIF_TRUE:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_MIF, cheat.address, 16, cheat.value);
	case GSA_16_BIT_MIF_FALSE:
		// Equal skips a number of lines instead of turning the rest off, so the
		// sentence stays on the condition and leaves the count out.
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_NE, cheat.address, 16, cheat.value);
	case GSA_16_BIT_MIF_LOWER_OR_EQ_U:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_LE, cheat.address, 16, cheat.value);
	case GSA_16_BIT_MIF_HIGHER_OR_EQ_U:
		return winExplainCondition(IDS_CHEAT_EXPLAIN_IF_GE, cheat.address, 16, cheat.value);

	case GSA_ALWAYS:
		return winExplainString(IDS_CHEAT_EXPLAIN_SKIP_NEXT);
	case GSA_ALWAYS2:
		return winExplainString(IDS_CHEAT_EXPLAIN_SKIP_TWO);

	// ---- arithmetic ---------------------------------------------------
	case CBA_ADD:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_ADD), cheat.value,
		            (LPCTSTR)winExplainTarget(cheat.address, (cheat.address & 1) ? 32 : 16));
		return text;
	}
	case CBA_OR:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_OR), cheat.value,
		            (LPCTSTR)winExplainTarget(cheat.address, 16));
		return text;
	}
	case CBA_AND:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_AND), cheat.value,
		            (LPCTSTR)winExplainTarget(cheat.address, 16));
		return text;
	}

	case GSA_8_BIT_ADD:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_ADD), cheat.value,
		            (LPCTSTR)winExplainTarget(cheat.address, 8));
		return text;
	}
	case GSA_16_BIT_ADD:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_ADD), cheat.value,
		            (LPCTSTR)winExplainTarget(cheat.address, 16));
		return text;
	}
	case GSA_32_BIT_ADD:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_ADD), cheat.value,
		            (LPCTSTR)winExplainTarget(cheat.address, 32));
		return text;
	}
	case GSA_32_BIT_ADD2:
	case GSA_32_BIT_SUB2:
	{
		extraLines = 1;
		CString text;
		text.Format(winExplainString(cheat.size == GSA_32_BIT_ADD2 ? IDS_CHEAT_EXPLAIN_ADD2
		                                                           : IDS_CHEAT_EXPLAIN_SUB2),
		            cheat.value);
		return text;
	}

	// ---- blocks of memory ---------------------------------------------
	case GSA_8_BIT_FILL:
	{
		// The engine writes (value >> 8) + 1 bytes: it ends with <= end.
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_FILL),
		            cheat.address, (LPCTSTR)winExplainValue(cheat.value & 0xFF, 8), (cheat.value >> 8) + 1);
		return text;
	}
	case GSA_16_BIT_FILL:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_FILL16),
		            cheat.address, (LPCTSTR)winExplainValue(cheat.value & 0xFFFF, 16), (cheat.value >> 16) + 1);
		return text;
	}

	case CBA_SLIDE_CODE:
	{
		// This line holds the start address and the first value; how far to
		// walk per step is packed into the address and value of the next line.
		// The engine runs x = 0 .. count, so count + 1 writes.
		extraLines = 1;

		u32 count = 0;
		u32 step  = 0;
		u32 delta = 0;
		if (index + 1 < cheatsNumber)
		{
			const CheatsData &data = cheatsList[index + 1];
			count = ((data.address - 1) & 0xFFFF) + 1;
			step  = (data.address >> 16) & 0xFFFF;
			delta = data.value;
		}

		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_SLIDE),
		            cheat.address, count, step, delta);
		return text;
	}

	case GSA_8_BIT_SLIDE:
	case GSA_16_BIT_SLIDE:
	case GSA_32_BIT_SLIDE:
	{
		// Same shape, but the start address sits in this line's value and the
		// step is counted in half-words or words.
		extraLines = 1;

		u32 count = 0;
		u32 step  = 0;
		u32 delta = 0;
		if (index + 1 < cheatsNumber)
		{
			const CheatsData &data = cheatsList[index + 1];
			count = (data.value >> 16) & 0xFF;
			delta = (data.value >> 24) & 0xFF;

			int unit = 1;
			if (cheat.size == GSA_16_BIT_SLIDE)
				unit = 2;
			else if (cheat.size == GSA_32_BIT_SLIDE)
				unit = 4;
			step = (data.value & 0xFFFF) * unit;
		}

		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_SLIDE),
		            cheat.value, count, step, delta);
		return text;
	}

	// ---- pointers ------------------------------------------------------
	case GSA_8_BIT_POINTER:
	case GSA_16_BIT_POINTER:
	case GSA_32_BIT_POINTER:
	{
		int bits = 8;
		u32 mask = 0xFF;
		if (cheat.size == GSA_16_BIT_POINTER)
		{
			bits = 16;
			mask = 0xFFFF;
		}
		else if (cheat.size == GSA_32_BIT_POINTER)
		{
			bits = 32;
			mask = 0xFFFFFFFF;
		}

		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_WRITE),
		            (LPCTSTR)winExplainPointerTarget(cheat.address, bits),
		            (LPCTSTR)winExplainValue(cheat.value & mask, bits));
		return text;
	}

	// ---- odds and ends --------------------------------------------------
	case CBA_SUPER:
	{
		extraLines = 1;
		// count = 2 * ((value - 1) & 0xFFFF) + 1, and the loop writes x = 0..count.
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_SUPER), cheat.address,
		            2 * ((cheat.value - 1) & 0xFFFF) + 2);
		return text;
	}
	case GSA_GROUP_WRITE:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_GROUP_WRITE),
		            (cheat.address & 0xFFFE) + 1);
		return text;
	}
	case GSA_16_BIT_WRITE_IOREGS:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_IO),
		            (LPCTSTR)winExplainValue(cheat.value & 0xFFFF, 16), cheat.address & 0x3FF);
		return text;
	}
	case GSA_32_BIT_WRITE_IOREGS:
	{
		CString text;
		text.Format(winExplainString(IDS_CHEAT_EXPLAIN_IO),
		            (LPCTSTR)winExplainValue(cheat.value, 32), cheat.address & 0x3FF);
		return text;
	}

	case GSA_SLOWDOWN:
		return winExplainString(IDS_CHEAT_EXPLAIN_SLOWDOWN);
	case GSA_CODES_ON:
		return winExplainString(IDS_CHEAT_EXPLAIN_CODES_ON);
	case MASTER_CODE:
		return winExplainString(IDS_CHEAT_EXPLAIN_MASTER);

	case UNKNOWN_CODE:
		// A gameshark identity line carries its marker in the value.
		if (cheat.value == 0x1DC0DE)
			return winExplainString(IDS_CHEAT_EXPLAIN_ID);
		break;

	default:
		break;
	}

	CString text;
	text.Format(winExplainString(IDS_CHEAT_EXPLAIN_UNKNOWN), cheat.size);
	return text;
}

// Writes `code`, then `text` indented under it, one output line per input line.
void winExplainAppend(CString &out, const CString &code, const CString &text)
{
	out += code;

	CString rest = text;
	int breakAt;
	while ((breakAt = rest.Find(_T("\r\n"))) >= 0)
	{
		out += _T("\r\n    ");
		out += rest.Left(breakAt);
		rest = rest.Mid(breakAt + 2);
	}

	out += _T("\r\n    ");
	out += rest;
	out += _T("\r\n");
}

} // namespace

CString winExplainCheats(int first, int count)
{
	CString out;

	if (first < 0 || count <= 0 || first + count > cheatsNumber)
		return out;

	const CheatsData &head = cheatsList[first];

	CString header;
	header.Format(winExplainString(IDS_CHEAT_EXPLAIN_HEADER),
	              (LPCTSTR)winExplainFamily(head.code), count);
	out += header;
	out += _T("\r\n");

	if (head.desc[0])
	{
		CString desc;
		desc.Format(winExplainString(IDS_CHEAT_EXPLAIN_CHEAT),
		            (LPCTSTR)winWideFromAnsi(head.desc));
		out += desc;
		out += _T("\r\n");
	}

	out += _T("\r\n");

	int end = first + count;
	for (int i = first; i < end; )
	{
		int extra = 0;
		CString text = winExplainOne(i, extra);
		winExplainAppend(out, winWideFromAnsi(cheatsList[i].codestring), text);

		// Those lines belong to the instruction above, but they are still lines
		// of the cheat: show them so the list matches what the user selected.
		for (int k = 1; k <= extra && i + k < cheatsNumber; k++)
		{
			out += winWideFromAnsi(cheatsList[i + k].codestring);
			out += _T("\r\n    ");
			out += winExplainString(IDS_CHEAT_EXPLAIN_DATA);
			out += _T("\r\n");
		}

		int next = i + 1 + extra;
		// A single selected line may still own a data line; show it too.
		if (next > end && next <= cheatsNumber)
			end = next;
		i = next;
	}

	return out;
}

/////////////////////////////////////////////////////////////////////////////
// CCheatExplainDlg

class CCheatExplainDlg : public CDialog
{
public:
	CCheatExplainDlg(const CString &text, CWnd *parent)
		: CDialog(CCheatExplainDlg::IDD, parent)
		, m_text(text)
	{
	}

	enum { IDD = IDD_CHEAT_EXPLAIN };

protected:
	virtual BOOL OnInitDialog()
	{
		CDialog::OnInitDialog();

		SetDlgItemText(IDC_EXPLAIN_TEXT, m_text);
		return TRUE;
	}

private:
	CString m_text;
};

void winShowCheatExplanation(CWnd *parent, int first, int count)
{
	CString text = winExplainCheats(first, count);
	if (text.IsEmpty())
		return;

	CCheatExplainDlg dlg(text, parent);
	dlg.DoModal();
}
