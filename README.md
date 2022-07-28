# keystroke_player
Linux X11 Autoclicker that can also use the keyboard, and script support is added. This is inspired by click4ever from https://github.com/daniel-araujo/click4ever. I just added keyboard support and scripting support for personal scripting uses.
# Usage
Run the program. It is an autoclicker command line program where you can edit configs, and compile/run scripts.
It runs autoclicker scripts that can stop executing the program completely if the mouse moves, it reaches the end, or it exits via `exit;`
.
# Script
Some examples of the script commands are shown in example_scripts.
Syntax is as follows (In RegExp-ish form). Note that all commands must be terminated with a semicolon.

Keystroke command = `[A-Za-z0-9\+\_]+=[UuDdCc];`
    
    Left Hand Side represents strings that is "X11 KeySym names separated by '+'"
    Example: "space" "w+a" "Alt_L+Tab"
    UuDdCc represents key up/down/click respectively.

Mouse command = `m[1-5]=[UuDdCc];`

    1=Left, 2=Middle, 3=Right, 4=Wheel Up, 5=Wheel Down
    UuDdCc also applies here too.

Delay command = `.[ums]?[0-9]+;`

    u/m/s represents microseconds/milliseconds/seconds respectively. 
    Without prefix is defaulted to microseconds.

LoopStart command = `\([A-Za-z0-9\+\_]+;`

    ( with Loop name is used to jump back to this command from a LoopEnd. There can only be one.

LoopEnd command = `\)[A-Za-z0-9\+\_]+(=[0-9]+)?;`

    ) with Loop name is used to jump back to a LoopStart.
    Numbers represent how many loops it would do.
    0 or without (=[0-9]+)? means it will loop forever.
    There can only be one.

MouseMove command = `mm[ar]=[0-9\-]+,[0-9\-]+;`

    Moves mouse based on absolute (mma) or relative (mmr) of the screen.
    Example: mmr=100,-50; Moves 100 pixels to the right and 50 pixels upwards
    relative to the current location.

Exit command = `exit;`

    Immediately exits the autoclicker program.
    Will be used because of JumpTo and JumpFrom commands.
    That can always jump.

JumpTo command = `JT([A-Za-z0-9\+\_]+;`

    Will always jump to a JumpFrom.
    There can be more than one JumpTo and it can be before and/or after
    a JumpFrom.

JumpFrom command = `JF)[A-Za-z0-9\+\_]+;`

    A JumpTo will jump to this command. There can be one and only one JumpFrom.
    Otherwise, the program will not compile if there are no JumpFroms or more than one with the same name.

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

# Build
source tasks.conf (In project folder)

Requires [xdotool library](https://github.com/jordansissel/xdotool).

Requires X11 library.

(Optional) Testing requires [Check library](https://github.com/libcheck/check).

Note: This has been built using Visual Studio Code under Arch Linux. Repositories for these libraries are in Arch Linux. It may also be readily available in other distributions of Linux.

# TODO
Adding functionality of jumping towards different commands depending on the color of the pixel of a mouse.