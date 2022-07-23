# keystroke_player
Linux X11 Autoclicker that can also use the keyboard, and script support is added. This is inspired by click4ever from https://github.com/daniel-araujo/click4ever. I just added keyboard support and scripting support for personal scripting uses.
# Usage
Run the program. It is a command line program where you can edit config, compile/run scripts, or exit the program.
It runs autoclicker scripts that can stop executing the program completely if the mouse moves.
# Script
Some examples of the script commands are shown in example_scripts.
Syntax is as follows (In RegExp-ish form). Note that all commands must be terminated with a semicolon.

Keystroke = `[A-Za-z0-9\+]+=[UuDdCc];`
    
    Left Hand Side represents strings that is "X11 KeySym names separated by '+'"
    Example: "space" "w+a" "Alt_L+Tab"
    UuDdCc represents key up/down/click respectively.

Mouse = `m[1-5]=[UuDdCc];`

    1=Left, 2=Middle, 3=Right, 4=Wheel Up, 5=Wheel Down
    UuDdCc also applies here too.

Delay = `.[ums]?[0-9]+;`

    u/m/s represents microseconds/milliseconds/seconds respectively. 
    Without prefix is defaulted to microseconds.

LoopStart = `\([A-Za-z0-9]+;`

    ( with Loop name is used to jump back to this command from a LoopEnd.

LoopEnd = `\)[A-Za-z0-9]+(=[0-9]+)?;`

    ) with Loop name is used to jump back to a LoopStart.
    Numbers represent how many loops it would do.
    0 or without (=[0-9]+) means it will loop forever.

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

(Optional) Testing requires [Check library](https://github.com/libcheck/check).

Note: This has been built using Visual Studio Code under Arch Linux. Repositories for xdotool and check is in Arch Linux. It may also be readily available in other distributions of Linux.
