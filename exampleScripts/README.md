
# Example BreadText Scripts

## Set-up

To try out the scripts, create `~/.breadtextrc.btsl` and put in something like this:

```
# This will depend on where you put the BreadText repository.
dec commonPath = "PATH TO REPO/exampleScripts"

# Put as many names here as you want.
dec scriptList = ["mandelbrot.btsl", "tetromino.btsl", "utilities.btsl"]

dec index = 0
while index < len(scriptList)
    dec path = commonPath + "/" + scriptList[index]
    import path
    end
    index += 1
end
```

After you create this config file, any new BreadText instance will load the scripts upon start-up.

## Script Descriptions

Each script adds one or more commands.

**asciiArt.btsl**

* `/asciiArt (text)`: Draws ASCII art text. Only compatible with letters and spaces.

**calc.btsl**

* `/calc`: Evaluates the arithmetic expression in the same line as the cursor. Writes the result underneath the same line. Compatible with the four basic operators and parentheses.

**conwayLife.btsl**

* `/conwayLife`: Runs Conway's game of life. Press space to advance. Press q to quit.

**cubePuzzle.btsl**

* `/cubePuzzle`: Runs a cube puzzle game. Press the arrow keys to move your cursor. Press space to rotate the selected face. Press q to quit.

**mandelbrot.btsl**

* `/mandelbrot`: Generates an ASCII art Mandelbrot set.

**maze.btsl**

* `/maze`: Generates a maze which you can navigate. Press the arrow keys to move. Press q to quit.

**mineDuster.btsl**

* `/mineDuster`: Plays an explosive puzzle game. Press the arrow keys to move your cursor. Press space to uncover a tile. Press f to flag a tile. Press q to quit.

**slidePuzzle.btsl**

* `/slidePuzzle (size?)`: Plays a sliding tile puzzle game. Press the arrow keys to move tiles. Press q to quit.

**solitaire.btsl**

* `/solitaire`: Plays a lonely card game. Press the arrow keys to move your cursor. Press space to move cards. Press f to flip through the deck. Press q to quit.

**sovietBlok.btsl**

* `/sovietBlok`: Plays a blok-based puzzle game. Press the arrow keys to move bloks. Place 3 of the same blok horizontally adjacent to clear them. Press q to quit.

**tetromino.btsl**

* `/tetromino`: Plays a block-based puzzle game. Press the arrow keys to move blocks. Fill a row with blocks to clear them. Press q to quit.

**utilties.btsl**

* `/toggleCase`: Switches highlighted text between `camelCase` and `snake_case`.
* `/measureLine`: Displays the length of the line containing the cursor.
* `/fizzBuzz (count?)`: Age-old interview question.
* `/sortLines`: Sorts highlighted lines by ASCII order. 
* `/beer`: 99 bottles of beer song.
* `/encode (number)`: Adds the given number to all highlighted letters.
* `/decode (number)`: Subtracts the given number from all highlighted letters.
* `/factor (number?)`: Displays the prime factorization of the argument or highlighted number.
* `/convertBase (sourceBase) (destinationBase)`: Changes the base of the highlighted text. A base may be `bin`, `oct`, `dec`, `hex`, or a numeric value.

