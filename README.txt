
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



