# keystroke_player
Linux X11 Keyboard/Mouse Macro program script player. This is inspired by click4ever from https://github.com/daniel-araujo/click4ever. I just added keyboard support and scripting support for personal scripting uses.
Note: Still developing things for this program. TODO: Adding variables to the script.
# Usage
Run the program. It is a command line program where you can edit configs, and compile/run scripts.
Uses X11 Keygrabs as keybinds to go through menus. Press the escape key to toggle disabling keybind functionality.
It runs keyboard/macro macro scripts that can stop executing the program completely if the q key is pressed, it reaches the end, or it exits via `exit;`
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

JumpFrom command = `JF>[A-Za-z0-9\+\_]+;`

    A JumpTo will jump to this command. There can be one and only one JumpFrom.
    Otherwise, the program will not compile if there are no JumpFroms or more than one with the same name.

JumpToStore command = `JTS>[A-Za-z0-9\+\_]+;`
JumpBack command = `JB>;`

    Same as JumpTo, but stores its index to a stack to jump to later, like a function.
    JumpBack command pops any index stored in the stack.
    Will throw an error if the stack is empty.

PRINT command = `PRINT>>(Command)`

    This prints any command prefixed with PRINT>>. Prints similar to setting debug_print_type=1 in configs.
    Can't be shown for debug_print_type=2.

There are Query Commands that will skip the next command if false, or not skip if true. They are prefixed with a `?` and end with a `?`. These should be used next to a JumpTo command. For example:

`?(query_command)?JT>ThisQueryIsTrue;(Commands here if false); ... JF>ThisQueryIsTrue;(Commands here if true);`

QueryComparePixel command = `?pxc=[0-9]+,[0-9]+,[0-9]+,[0-9]+?`

    QueryComparePixel checks if the pixel at it's current mouse position is true. Valid numbers should be from 0 to 255.
    They are formatted by (rc,gc,bc,threshold) (Pixel to compare), where threshold will check any pixel colors close to rm,gm,bm (Pixel by mouse)
    (Formula for query is true if abs(rc-rm)<=threshold&&abs(gc-gm)<=threshold&&abs(bc-bm)<=threshold)
    For example:
        If ?pxc=128,128,128,20; is the command, and
        the mouse pixel is r,g,b=108,148,128 the query is true since 108, 148, and 128 is within 20.
        If the mouse pixel is r,g,b=255,255,255, the query is false since 255 is not within 20.

QueryCompareCoords command = `?coords=[xy][<>]=?[0-9]?`

    Compares either the x or y coordinate of the mouse. Supports
    >, >=, <, and <= only.
        Examples: ?coords=x>=100? compares if x is
        greater than or equal to 100.
        ?coords=y<500? compares if y is less than 500.

QueryCoordsWithin command = `?within=[0-9]+,[0-9]+,[0-9]+,[0-9]+?`

    Check if mouse is within the boxed coordinates xl,yl,xh,yh.
    xl,yl are the top left coordinates.
    xh,yh are the bottom right coordinates.

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
# Build
source tasks.conf (In project folder)

Requires [xdotool library](https://github.com/jordansissel/xdotool).

Requires X11 library.

(Optional) Testing requires [Check library](https://github.com/libcheck/check).

Note: This has been built using Visual Studio Code under Arch Linux. Repositories for these libraries are in Arch Linux. It may also be readily available in other distributions of Linux.