
= = = BREADTEXT = = =

NOTE: This program is not yet finished!

INSTALLATION

To install:

make
sudo mv ./breadtext /usr/local/bin/breadtext

On Linux you must install xclip for clipboard functionality:

sudo apt-get install xclip

CONTROLS

T = Text-entry mode
, + , (again) or Escape = Command mode
IJKL = Scroll one character
Shift + IJKL = Scroll 10 characters
[] = Scroll to beginning or end of line
{} = Scroll to beginning or end of file
/ = Enter command
S = Save
Q = Quit
Shift + Q = Quit without saving
M = Play macro
Shift + M = Start or stop recording macro
H = Character highlight mode
Shift + H = Line highlight mode
W = Word highlight mode
D = Delete
C = Copy
Shift + C = Cut
P = Paste after cursor
Shift + P = Paste before cursor
U = Undo
Shift + U = Redo
<> = Indent
Tab or Shift + Tab = Indent
N = Find next instance
Shift + N = Find previous instance

COMMANDS

/gotoLine (line number)
/find (pattern)
/replace (pattern) (text)
/setMark (mark number)
/gotoMark (mark number)
/set (config variable) (value)
/help

CONFIGURATION VARIABLES

colorScheme: 0 is black on white, 1 is white on black.
shouldUseHardTabs: 0 means no, 1 means yes.
indentationWidth: The number of spaces to use for soft tabs.

On start-up, breadtext looks for the file ~/.breadtextrc to read configuration variables. Each line of .breadtextrc contains a variable name and a value separated by a space.

Example contents of .breadtextrc file:

colorScheme 0
shouldUseHardTabs 0
indentationWidth 4


