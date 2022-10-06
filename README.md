# keystroke_player
Linux X11 Keyboard/Mouse Macro program script player. This is inspired by click4ever from https://github.com/daniel-araujo/click4ever and AutoHotkey https://github.com/Lexikos/AutoHotkey_L.

I added keyboard support and scripting support for personal scripting uses. The whole purpose was to create an autoclicker/keyboard macro for Linux to automate playing games by writing scripts.

Note: Still developing things for this program.

# Usage
Run the program. It is a command line program where you can edit configs, and compile/run scripts.
Uses X11 Keygrabs as keybinds to go through menus. Press the escape key to toggle disabling keybind functionality.
It runs keyboard/macro macro scripts that can stop executing the program completely if the escape key is pressed, it reaches the end, the program throws a runtime error, or it exits via `exit;`
.
# Script Commands and Queries
Some examples of the script commands are shown in example_scripts.
Syntax is as follows (In RegExp-ish form). Note that most commands terminate with a semicolon `;`.

Keystroke command = `[A-Za-z0-9\+\_]+=[UuDdCc];`
    
    Left Hand Side represents strings that is "X11 KeySym names separated by '+'"
    Example: "space" "w+a" "Alt_L+Tab"
    UuDdCc represents key up/down/click respectively.

Mouse command = `m[1-5]=[UuDdCc];`

    1=Left, 2=Middle, 3=Right, 4=Wheel Up, 5=Wheel Down
    UuDdCc also applies here too from Keystroke commands.

Delay command = `.[ums]?[0-9]+;`

    u/m/s represents microseconds/milliseconds/seconds respectively. 
    Without prefix, numbers are defaulted to microseconds (1/1000000 seconds).

RepeatStart command = `\([A-Za-z0-9\+\_]+;`

    ( with string name is used to jump back to this command from a RepeatEnd. There can only be one and at least 1 RepeatEnd is needed.

RepeatEnd command = `\)[A-Za-z0-9\+\_]+(=[0-9]+)?;`

    ) with string name is used to jump back to a RepeatStart.
    Numbers represent how many loops it would do.
    =0 or without (=[0-9]+)? means it will loop forever.

RepeatReset command = `rep_reset;`

    Resets all counters in RepeatStart back to 0. This is used when Jump commands have been used
    since it doesn't reset any counters back to 0.

MoveMouse command = `mm[ar]=[0-9\-]+,[0-9\-]+;`

    Moves mouse based on absolute (mma) or relative (mmr) of the screen.
    Example: mmr=100,-50; Moves 100 pixels to the right and 50 pixels upwards
    relative to the current location.

Exit command = `exit;`

    Immediately exits the macro script.
    Will be used because of JumpTo and JumpFrom commands.
    That can always jump.

Pass command = `pass;`

    Placeholder command for nothing.

SaveMouseCoords = `save_mma;`

LoadMouseCoords = `load_mma;`

    This saves/loads the mouse coordinates that was previously saved by a load_mma; command.
    When using `load_mma` without any previous `save_mma;` command, the default is (x,y)=(0,0)

JumpTo command = `JT>[A-Za-z0-9\+\_]+;`

    Will always jump to a JumpFrom.
    There can be more than one JumpTo and it can be before and/or after
    a JumpFrom.

JumpToIndex command = `JTI[AR]>[0-9\-]+;`

    Will jump based based on (A)bsolute or (R)elative position.
    Negative numbers are used for relative positions. The program will throw
    and error if numbers are out of bounds (It should be from 1 to the maximum number
    of commands currently in the program).

JumpFrom command = `JF>[A-Za-z0-9\+\_]+;`

    A JumpTo will jump to this command. There can be one and only one JumpFrom.
    Otherwise, the program will not compile if there are no JumpFroms or more than one with the same name.

JumpToStore command = `JTS>[A-Za-z0-9\+\_]+;`
JumpBack command = `JB>;`

    Same as JumpTo, but stores its index to a stack to jump to later, like a function.
    JumpBack command pops any index stored in the stack.
    Will throw an error if the stack is empty.

WaitUntilKey command = `!?wait_key=[A-Za-z0-9_]+;`

    This command blocks the macro program until the key has been held down.
    Appending ! to wait_key inverts so that the program checks if it is not held down.
    The key is based on the X11 KeySym names. 
    You cannot use this with GrabKey commands, as the macro will throw a runtime error.
    You cannot use the key 'escape' as that is the key to quit the macro.
    Note: This command slows down loops. It is advised to use these outside loops.


WaitUntilButton command = `!?wait_button=[1-5];`

WaitUntilButton command (clicked) = `wait_buttonc=[1-5];`

    This command blocks the macro program until the mouse button has been held down.
    wait_buttonc makes it so that next button clicks are required instead of holding down.
    Appending ! to wait_button inverts so that the program checks if it is not held down.
    1=Left, 2=Middle, 3=Right, 4=Wheel Up, 5=Wheel Down
    You cannot use this with GrabButton commands (TODO), as the macro will throw a runtime error.
    Note: This command slows down loops. It is advised to use these outside loops.

GrabKey command = `grab_key=[A-Za-z0-9_]+;`

UngrabKeyAll command = `ungrab_keys;`

    This command listens for key presses and releases,
    but also prevents future key presses from this key to be used normally.
    To undo this, or to ungrab all keys, use the UngrabKeyAll command.
    This is paired with the QueryKeyPress command. 
    The key is based on the X11 KeySym names.
    You cannot use the key 'escape' as that is the key to quit the macro.

Print command = `print=...;;`

Print command (with newline \n) = `println=...;;`

    You can print most characters to the terminal.
    Unlike other commands, this command requires double semicolons
    to print characters. These commands are similar to print
    functions in C, but with some differences
    You can output the value of an RPN string (see Variable Loading, Manipulation, and RPN
    for more information). Use parenthesis as normally and the RPN string to see.
    This command will output it as (number)(d/l/i/c), where d/l/i/c are types
    double/long/int/char respectively.
    Example: println=This will output 3l => (1,2,+);;
    will output "This will output 3l => 3l" in the program with a newline.
    You can use escape codes as described below.
    \a \b \e \f \n \r \t and \v. There are custom
    escape characters, \( and \;, which prints "(" and ";" respectively. 
    '\\n' (Or \ and pressing Enter) adds no characters just like in shell scripts.
    Note: `print=` commands should not be used with debug functions, as the program
    will warn against using them (when using `print=` and not `println=`)
    Warning: Don't use strings containing "%_rpn", as
    these internally will be replaced by the rpn string values.

PRINT command = `PRINT>>(Command)`

    This prints any command prefixed with PRINT>>. 
    Prints similar to setting debug_print_type=1 in configs.
    Can't be shown for debug_print_type=2.

DebugConfig command =

`debug,debug_print_type=[0-2];`

`debug,rpn_decimals=[0-9]{3};`

`debug,rpn_stack_debug=[0-1];`

    These temporarily change the values from the config file to be set in the macro.
    Valid numbers should be used. For rpn_decimals, it should be from 0 to 255 only.

There are Query Commands that will skip the next command if false, or not skip if true. They are prefixed with a `?` and end with a `?`. These should be used next to a JumpTo command. For example:

`?(query_command)?JT>ThisQueryIsTrue;(false command); ... JF>ThisQueryIsTrue;(true command);`

If queries are chained together, for example, `?q1??q2?q3?(true);(false);`, all queries will jump to the `(false);` command if either query is false. Otherwise, they will read the next query, or go to the `(true);` command if all queries are true.

All queries can be inverted by prefixing them with a `!` after `?` to skip the next command if true, or not if false (Example: `?!(query)?`. Chained queries with `!` also applies to the above.

QueryComparePixel command = `\?!?pxc=[0-9]+,[0-9]+,[0-9]+,[0-9]+\?`

    QueryComparePixel checks if the pixel at it's current mouse position is true. Valid numbers should be from 0 to 255.
    They are formatted by (rc,gc,bc,threshold) (Pixel to compare), where threshold will check any pixel colors close to rm,gm,bm (Pixel by mouse)
    (Formula for query is true if abs(rc-rm)<=threshold&&abs(gc-gm)<=threshold&&abs(bc-bm)<=threshold)
    For example:
        If ?pxc=128,128,128,20; is the command, and
        the mouse pixel is r,g,b=108,148,128 the query is true since 108, 148, and 128 is within 20.
        If the mouse pixel is r,g,b=255,255,255, the query is false since 255 is not within 20.

QueryCompareCoords command = `\?!?coords=[xy][<>]=?[0-9]+\?`

    Compares either the x or y coordinate of the mouse. Supports
    >, >=, <, and <= only.
        Examples: ?coords=x>=100? compares if x is
        greater than or equal to 100.
        ?coords=y<500? compares if y is less than 500.

QueryCoordsWithin command = `\?!?within=[0-9]+,[0-9]+,[0-9]+,[0-9]+\?`

    Check if mouse is within the boxed coordinates xl,yl,xh,yh.
    xl,yl are the top left coordinates.
    xh,yh are the bottom right coordinates.

QueryRPNEval command = `\?!?eval=\(RPN\)\?`

    For more information, see header Commands with Variable Loading and Manipulation

QueryKeyPress command = `\?!?key_press=[A-Za-z0-9_]+\?`

    This query checks if a key has been held down.
    The key is based on the X11 KeySym names.
    The GrabKey command for the key is required here to function, otherwise the command will warn that the key has not been initialized yet.
    You cannot use the key 'escape' as that is the key to quit the macro.

Comments/Tabs/Spaces/Newlines can be added after a semi-colon has been added to a command.

Here is an example script from example_scripts/autoclicker.kps:

    (A; #Start of loop with "A" string.
    m1=c;.m100; #1 = Left mouse click for 100 milliseconds.
    )A; #Repeat forever.
From example_scripts/run_in_circles.kps:

    (A;
    (M1;m1=c;w=d;.m100;w=u;)M1=2; #Use w/a/s/d while clicking with mouse left button.
    (M2;m1=c;w+a=d;.m100;w+a=u;)M2=2;
    (M3;m1=c;a=d;.m100;a=u;)M3=2;
    (M4;m1=c;a+s=d;.m100;a+s=u;)M4=2;
    (M5;m1=c;s=d;.m100;s=u;)M5=2;
    (M6;m1=c;s+d=d;.m100;s+d=u;)M6=2;
    (M7;m1=c;d=d;.m100;d=u;)M7=2;
    (M8;m1=c;d+w=d;.m100;d+w=u;)M8=2;
    )A; #Loop forever.

# Variable Loading, Manipulation, and RPN

Variables can be added to this program as a char, int, long int, or double. The program uses RPN notation (https://en.wikipedia.org/wiki/Reverse_Polish_notation) when writing expressions to load or save variables, or to add RPN notation to vary values for certain commands. RPN strings for this program start with `(` and end with `)`. All tokens are delimited with `,`. For example: `(1,1,+)` is a valid RPN string that, when evaluated, outputs `2`.

Variables can be added to this program as a char, int, long int, or double. Note: All variables are in the global scope only and should be initialized before using (With the InitVar command). The following functions can be used that has near similar functions to the c operations and the math.h library. Note: These names are reserved and cannot be used to name functions with the same name. The program also prefixes __c, __i, __l, and __d for the functions below, and they are also reserved names for char/int/long/double functions respectively. Example: You cannot name a variable `abs`, as well as `__cabs`, `__iabs`, `__labs`, and `__dabs`.

`abs`, `max/maxu`, `min/minu`, `random_c`, `as_c`, `random_i`, `as_i`, `random_l`, `as_l`, `random_d`, `as_d`, `exp`, `exp2`, `log`, `log2`, `log10`, `pow`, `sqrt`, `cbrt`, `hypot`, `sin/sind`, `cos/cosd`, `tan/tand`, `asin/asind`, `acos/acosd`, `atan/atand`, `ceil`, `floor`, `round`, `trunc`, `+`, `++`, `-`, `-m`, `--`, `*`, `/`, `/u`, `%`, `%u`, `&`, `|`, `~`, `^`, `<<`, `<<u`, `>>`, `>>u`, `==`, `==u`, `!=`, `!=u`, `>`, `>u`, `<`, `<u`, `>=`, `>=u`, `<=`, `<=u`, `!`, `&&`, `||`

Some differences when using these functions, compared to C, are listed below.

- int/char/long number types are all signed. You can append u for certain functions (`/u`, `%u`, `>=u`, etc.) to compare/operate numbers by its unsigned value. You cannot mix signedness, and you cannot use these functions with double types.
- Unlike in C, `++` and `--` doesn't increment/decrement any variable values after usage.
- For comparisons with unsigned integers (and not doubles), append u to `==`, `!-`, `>`, `<`, `>=`, `<=`, `/`, and `%`.
- `-m` is unary minus sign.
- Trigonometric functions can use degrees if appended with d (Ex: `sind`, `atand`...).
- `minu` and `maxu` compares min and max for unsigned integers.
- `random_(c/i/l)` outputs random numbers of their respective types anywhere from their minimum to maximum value.
`random_d` just outputs a double from 0 to 1.
- Functions `as_(c/i/l/d)` is used for type casting.
- Dividing by 0 with `/ /u` or `% %u` doesn't abort the program, but terminates the macro.
- Using a negative signed number for the second operation in `>> >>u` and `<< <<u` also terminates the macro.
Note that it does not abort the program in C, but mixed signedness is not implemented in this program.

# RPN Reserved Variables
There are also reserved variable names that can be used in RPN strings. The variable names are prefixed with `@`.
These should generally be used when starting the macro and not from testing equations from the program main menu. Here are the current list of defined reserved variables.

- `@mma_x` and `@mma_y` - When using the command `save_mma;`, it also saves the x and y coordinates respectively to these names.

- `@ci_now` - This is the current index, or the "program counter" of the command when used. Note that it starts from 1 to (Number of Total Commands). `@ci_prev` is the last index that was counted in the program. `@ci_last` is the last index of the last command in the program.

- `@time_s` and `@time_ns` - These count the current time since the macro program began, where _s counts the seconds and _ns counts the remaining nanoseconds.


# Commands with Variables 

Here are commands that uses RPN notation to load/save/manipulate variables.

InitVar command = `init,[cildr],[A-Za-z0-9_]=[cild]?(RPN);`
Note: This should be used before using any other commands, or the program will throw an error.

    Initializes a variable of type (c)har (i)nt (l)ong or (d)ouble.
    (r)pn is used to express the value in RPN notation.
    The rpn value is evaluated at compile time.
    Third parameter is the name of the variable.
    The value after = requires the type [cild] before the
    RPN expression if the r character is used.

    Valid command examples:
    init,i,ten_int=10;
    init,d,one_point_two=1.2;
    init,r,added_vars=i(ten_int,one_point_two,+);
    The third variable added_vars adds the two variables together and casts
    it to an integer.

EditVar command = `edit,[A-Za-z0-9_]=(RPN);`

    Edits a variable to a new value. The new value is automatically
    casted to its original type based on the InitVar command.

    Valid command examples:
    init,i,var1=1;
    edit,var1=(var1,++); #Adds 1 to var1. Store as int.
    edit,var1=(var1,random_d,10,*,+);
    #For a random double from 0 to 10, add it to var1.
    #Note: Even though it returns a double, the InitVar command of var1
    #with the i flag will always cast it to an int. 

QueryRPNEval command = `\?!?eval=\(RPN\)\?`

    Checks the RPN string that will go to the next command
    if the expression is non-zero. This is similar to
    an if statement in C. Ex: if(condition){}, run
    the statements inside the curly brackets if the
    condition is non-zero.
    Example of valid commands:
    ?eval=(zero_var,0,==)?(true);(false);
    ?eval=(zero_var,!)?(true);(false); #Similar to above.

The following commands mentioned in header Script Commands and Queries also supports RPN strings and variables for dynamic values:

`Delay`, `RepeatEnd`, `MoveMouse` (Absolute and Relative), `JumpToIndex` (Absolute and Relative), `QueryComparePixel`, `QueryCompareCoords`, and `QueryCoordsWithin`.

    Examples:
    init,i,delay_v=0;
    init,i,wait_c=40;
    (A;
    edit,delay_v=(delay_v,++);
    edit,wait_c=(wait_c,--);
    mmr=(delay_v),(wait_c); #Move mouse dynamically.
    .m(delay_v,50,*); #Wait 50 milliseconds more each loop.
    )A=(wait_c); #Decrease counter. Would stop at 20.

    #Note the parenthesis for these commands,
    #as they are RPN strings. You can mix values and
    #rpn strings at the same time for these commands.
    #Example below:
    mmr=10,(move_y);
    #Move right by 10 pixels and up/down depending on move_y.

# Text Substitution Macros and Macro Expansion
You can add text-substitution macros in the scripts. They are basically used to copy and paste code like in C. To make macros, they must be within these brackets `[!! !!]` at the start of the file. Each macro definition must be within `[! !]` and have a definition separator `:=`. It is of the format `[!MACRO_NAME:Var1:Var2:Var3:...:= (Macro Definition) !]`. Note that the macro definition can have whitespace, but it will be trimmed within the macro definition. To get the variable names for the macro definition so the macro call can substitute them, use `:(variable_name)` To call a macro in the code, just call it with the macro name and its arguments (if any). Example: The macro call `[!MACRO_CALL:abc:def:ghi!]`, where `[!MACRO_CALL:v1:v2:v3:= :v1+:v2*:v3 !]` is the definition of the macro becomes `abc+def*ghi`.

Macro Definition syntax
    
    \[![a-zA-Z0-9_]+(:[a-zA-Z0-9_])*:=[^!\]*]!\]
    where :[a-zA-Z0-9_]+ is to use a variable name in the r.h.s. of the macro definition.
    Whitespace can be used on the r.h.s. after := only.

Macro Call syntax

    \[![a-zA-Z0-9_]+(:[a-zA-Z0-9_])*!\], where the (:[a-zA-Z0-9_])* are the variable names used within the macro (if any).

Here is an example in example_scripts/macro_test.kps.

    [!!
    [!Mouse_D:=1085,1077!]
    [!L_X:=435!][!L_Y:=300!]
    [!H_X:=1294!][!H_Y:=1127!]
    [!X_v:=5!][!Y_v:=2!]
    !!]
    ?within=[!L_X!],[!L_Y!],[!H_X!],[!H_Y!]?JT>WithinBox;mma=[!Mouse_D!];JF>WithinBox;
    JF>UpLeft;mmr=-[!X_v!],-[!Y_v!];.m5;?coords=y<[!L_Y!]?JT>DownLeft;
        ?coords=x<[!L_X!]?JT>UpRight;
        JT>UpLeft;
    JF>UpRight;mmr=[!X_v!],-[!Y_v!];.m5;?coords=y<[!L_Y!]?  JT>DownRight;
        ?coords=x>[!H_X!]?JT>UpLeft;
        JT>UpRight;
    JF>DownLeft;mmr=-[!X_v!],[!Y_v!];.m5;?coords=y>[!H_Y!]?JT>UpLeft;
        ?coords=x<[!L_X!]?JT>DownRight;
        JT>DownLeft;
    JF>DownRight;mmr=[!X_v!],[!Y_v!];.m5;?coords=y>[!H_Y!]?JT>UpRight;
        ?coords=x>[!H_X!]?JT>DownLeft;
        JT>DownRight;

# Text Substitution Reserved Macros

These are macros that have `@` affixed to them similar to RPN Reserved Variables.

Here are the Current Reserved Macros and their specified arguments.

`[!@REP:"str":"num"!]` - Repeat string macro

    Macro that copies the "str" argument
    by number of "num" times. "num" must be
    an integer greater than 0.
    Example: "[!@REP:pass;:5!]" outputs "pass;pass;pass;pass;pass;"

# Build
source tasks.conf (In project folder)

Requires [xdotool library](https://github.com/jordansissel/xdotool).

Requires X11 library.

(Optional) Testing requires [Check library](https://github.com/libcheck/check).

Note: This has been built using Visual Studio Code under Arch Linux. Repositories for these libraries are in Arch Linux. It may also be readily available in other distributions of Linux.