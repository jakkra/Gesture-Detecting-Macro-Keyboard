#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.
;Title := WinGetTitle(WinTitle, WinText, ExcludeTitle, ExcludeText)
;MsgBox "The active window is '" Title "'."
;MsgBox "The active window is '" WinGetTitle("A") "'."

winId1 = 0
winId2 = 0
winId3 = 0
winId4 = 0
winId5 = 0

^!a::  ; Ctrl+Alt+a
{
    WinActivate ahk_id %winId1%
    return
}

^!b::  ; Ctrl+Alt+b
{
    WinActivate ahk_id %winId2%
    return
}

^!c::  ; Ctrl+Alt+c
{
    WinActivate ahk_id %winId3%
    return
}

^!d::  ; Ctrl+Alt+d
{
    WinActivate ahk_id %winId4%
    return
}

^!e::  ; Ctrl+Alt+e
{
    WinActivate ahk_id %winId5%
    return
}

^!f::  ; Ctrl+Alt+N
{
    WinGet, winid ,, A ;
    Input, UserInput, T10 L1 M

    Switch UserInput
    {
    Case 1:
        MsgBox, You entered 1
        winId1 = %winid%
    Case 2:
        MsgBox, You entered 2
        winId2 = %winid%
    Case 3:
        MsgBox, You entered 3
        winId3 = %winid%
    Case 4:
        MsgBox, You entered 4
        winId4 = %winid%
    Case 5:
        MsgBox, You entered 5
        winId5 = %winid%
    Default:
        MsgBox, Invalid character, 1-5 accepted
        return
    }

    WinGetTitle, Title, ahk_id %winid%
    MsgBox, Switch "%UserInput%" connected to window "%Title%".

    return
}