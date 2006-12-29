/************************************************************************ 
 * This file is part of PDCurses. PDCurses is public domain software;	*
 * you may use it for any purpose. This software is provided AS IS with	*
 * NO WARRANTY whatsoever.						*
 *									*
 * If you use PDCurses in an application, an acknowledgement would be	*
 * appreciated, but is not mandatory. If you make corrections or	*
 * enhancements to PDCurses, please forward them to the current		*
 * maintainer for the benefit of other users.				*
 *									*
 * See the file maintain.er for details of the current maintainer.	*
 ************************************************************************/

#include <curspriv.h>
#include <string.h>

RCSID("$Id: insch.c,v 1.38 2006/12/29 08:50:51 wmcbrine Exp $");

/*man-start**************************************************************

  Name:								insch

  Synopsis:
	int insch(chtype ch);
	int winsch(WINDOW *win, chtype ch);
	int mvinsch(int y, int x, chtype ch);
	int mvwinsch(WINDOW *win, int y, int x, chtype ch);

	int insrawch(chtype ch);
	int winsrawch(WINDOW *win, chtype ch);
	int mvinsrawch(int y, int x, chtype ch);
	int mvwinsrawch(WINDOW *win, int y, int x, chtype ch);

	int ins_wch(const cchar_t *wch);
	int wins_wch(WINDOW *win, const cchar_t *wch);
	int mvins_wch(int y, int x, const cchar_t *wch);
	int mvwins_wch(WINDOW *win, int y, int x, const cchar_t *wch);

  X/Open Description:
	The routine insch() inserts the character ch into the default
	window at the current cursor position and the window cursor is
	advanced.  The character is of the type chtype as containing
	both data and attributes.

	The routine winsch() inserts the character ch into the specified
	window at the current cursor position.  The cursor position is
	advanced.

	The routine mvinsch() moves the cursor to the specified (y, x)
	position and inserts the character ch into the default window.
	The cursor position is advanced after the character has been
	inserted.

	The routine mvwinsch() moves the cursor to the specified (y, x)
	position and inserts the character ch into the specified
	window.  The cursor position is advanced after the character
	has been inserted.

	All these routines are similar to putchar().  The following
	information applies to all the routines.

	If the cursor moves on to the right margin, an automatic
	newline is performed.  If scrollok is enabled, and a character
	is added to the bottom right corner of the screen, the
	scrolling region will be scrolled up one line.  If scrolling
	is not allowed, ERR will be returned.

	If ch is a tab, newline, or backspace, the cursor will be
	moved appropriately within the window.  If ch is a newline,
	the clrtoeol routine is called before the cursor is moved to
	the beginning of the next line.  If newline mapping is off,
	the cursor will be moved to the next line, but the x
	coordinate will be unchanged.  If ch is a tab the cursor is
	moved to the next tab position within the window.  If ch is
	another control character, it will be drawn in the ^X
	notation.  Calling the inch() routine after adding a control
	character returns the representation of the control character,
	not the control character.

	Video attributes can be combined with a character by ORing
	them into the parameter.  This will result in these attributes
	being set.  The intent here is that text, including
	attributes, can be copied from one place to another using inch()
	and insch().

  PDCurses Description:
	insrawch() etc. are wrappers for insch() etc. that disable the 
	translation of control characters.

  X/Open Return Value:
	All functions return OK on success and ERR on error.

  Portability				     X/Open    BSD    SYS V
	insch					Y	Y	Y
	winsch					Y	Y	Y
	mvinsch					Y	Y	Y
	mvwinsch				Y	Y	Y
	insrawch				-	-	-
	winsrawch				-	-	-
	ins_wch					Y
	wins_wch				Y
	mvins_wch				Y
	mvwins_wch				Y

**man-end****************************************************************/

int winsch(WINDOW *win, chtype ch)
{
	int x, y;
	chtype attr;
	bool xlat;

	PDC_LOG(("winsch() - called: win=%p ch=%x (text=%c attr=0x%x)\n",
		win, ch, ch & A_CHARTEXT, ch & A_ATTRIBUTES));

	if (!win)
		return ERR;

	x = win->_curx;
	y = win->_cury;

	if (y > win->_maxy || x > win->_maxx || y < 0 || x < 0)
		return ERR;

	xlat = !SP->raw_out && !(ch & A_ALTCHARSET);
	attr = ch & A_ATTRIBUTES;
	ch &= A_CHARTEXT;

	if (xlat && (ch < ' ' || ch == 0x7f))
	{
		int x2;

		switch (ch)
		{
		case '\t':
			for (x2 = ((x / TABSIZE) + 1) * TABSIZE; x < x2; x++)
			{
				if (winsch(win, attr | ' ') == ERR)
					return ERR;
			}
			return OK;

		case '\n':
			wclrtoeol(win);
			break;

		case 0x7f:
			if (winsch(win, attr | '?') == ERR)
				return ERR;

			return winsch(win, attr | '^');

		default:
			/* handle control chars */

			if (winsch(win, attr | (ch + '@')) == ERR)
				return ERR;

			return winsch(win, attr | '^');
		}
	}
	else
	{
		int maxx;
		chtype *temp;

		/* If the incoming character doesn't have its own
		   attribute, then use the current attributes for
		   the window. If it has attributes but not a color
		   component, OR the attributes to the current
		   attributes for the window. If it has a color
		   component, use the attributes solely from the
		   incoming character. */

		if (!(attr & A_COLOR))
			attr |= win->_attrs;

		/* wrs (4/10/93): Apply the same sort of logic for the
		   window background, in that it only takes precedence
		   if other color attributes are not there and that the
		   background character will only print if the printing
		   character is blank. */

		if (!(attr & A_COLOR))
			attr |= win->_bkgd & A_ATTRIBUTES;
		else
			attr |= win->_bkgd & (A_ATTRIBUTES ^ A_COLOR);

		if (ch == ' ')
			ch = win->_bkgd & A_CHARTEXT;

		/* Add the attribute back into the character. */

		ch |= attr;

		maxx = win->_maxx;
		temp = &win->_y[y][x];

		memmove(temp + 1, temp, (maxx - x - 1) * sizeof(chtype));

		win->_lastch[y] = maxx - 1;

		if ((win->_firstch[y] == _NO_CHANGE) || (win->_firstch[y] > x))
			win->_firstch[y] = x;

		*temp = ch;
	}

	PDC_sync(win);

	return OK;
}

int insch(chtype ch)
{
	PDC_LOG(("insch() - called\n"));

	return winsch(stdscr, ch);
}

int mvinsch(int y, int x, chtype ch)
{
	PDC_LOG(("mvinsch() - called\n"));

	if (move(y, x) == ERR)
		return ERR;

	return winsch(stdscr, ch);
}

int mvwinsch(WINDOW *win, int y, int x, chtype ch)
{
	PDC_LOG(("mvwinsch() - called\n"));

	if (wmove(win, y, x) == ERR)
		return ERR;

	return winsch(win, ch);
}

int winsrawch(WINDOW *win, chtype ch)
{
	PDC_LOG(("winsrawch() - called: win=%p ch=%x "
		"(char=%c attr=0x%x)\n", win, ch,
		ch & A_CHARTEXT, ch & A_ATTRIBUTES));

	if ((ch & A_CHARTEXT) < ' ' || (ch & A_CHARTEXT) == 0x7f)
		ch |= A_ALTCHARSET;

	return winsch(win, ch);
}

int insrawch(chtype ch)
{
	PDC_LOG(("insrawch() - called\n"));

	return winsrawch(stdscr, ch);
}

int mvinsrawch(int y, int x, chtype ch)
{
	PDC_LOG(("mvinsrawch() - called\n"));

	if (move(y, x) == ERR)
		return ERR;

	return winsrawch(stdscr, ch);
}

int mvwinsrawch(WINDOW *win, int y, int x, chtype ch)
{
	PDC_LOG(("mvwinsrawch() - called\n"));

	if (wmove(win, y, x) == ERR)
		return ERR;

	return winsrawch(win, ch);
}

#ifdef PDC_WIDE
int wins_wch(WINDOW *win, const cchar_t *wch)
{
	PDC_LOG(("wins_wch() - called\n"));

	return wch ? winsch(win, *wch) : ERR;
}

int ins_wch(const cchar_t *wch)
{
	PDC_LOG(("ins_wch() - called\n"));

	return wins_wch(stdscr, wch);
}

int mvins_wch(int y, int x, const cchar_t *wch)
{
	PDC_LOG(("mvins_wch() - called\n"));

	if (move(y, x) == ERR)
		return ERR;

	return wins_wch(stdscr, wch);
}

int mvwins_wch(WINDOW *win, int y, int x, const cchar_t *wch)
{
	PDC_LOG(("mvwins_wch() - called\n"));

	if (wmove(win, y, x) == ERR)
		return ERR;

	return wins_wch(win, wch);
}
#endif
