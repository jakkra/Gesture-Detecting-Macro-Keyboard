#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

winId1 = 0
winId2 = 0
winId3 = 0
winId4 = 0
winId5 = 0

openIfMinimizedHideIfOpen(winid)
{
    WinGet, activeWin ,, A
    if (winid == activeWin)
    {
        WinMinimize ahk_id %winid%
    }
    else
    {
        WinActivate ahk_id %winid%
    }
    return
}

^!a::  ; Ctrl+Alt+a
{
    openIfMinimizedHideIfOpen(winId1)
    return
}

^!b::  ; Ctrl+Alt+b
{
    openIfMinimizedHideIfOpen(winId2)
    return
}

^!c::  ; Ctrl+Alt+c
{
    openIfMinimizedHideIfOpen(winId3)
    return
}

^!d::  ; Ctrl+Alt+d
{
    openIfMinimizedHideIfOpen(winId4)
    return
}

^!e::  ; Ctrl+Alt+e
{
    openIfMinimizedHideIfOpen(winId5)
    return
}

; Config shortcuts
; Shortcut ^!a is configured by ^!f
; Shortcut ^!b is configured by ^!WinGet
; etc.

^!f::  ; Ctrl+Alt+f
{
    WinGet, winid ,, A ;
    winId1 = %winid%
    ;MsgBox, f %winid%
    return
}

^!g::  ; Ctrl+Alt+g
{
    WinGet, winid ,, A ;
    winId2 = %winid%
    ;MsgBox, g %winid%
    return
}

^!h::  ; Ctrl+Alt+h
{
    WinGet, winid ,, A ;
    winId3 = %winid%
    ;MsgBox, h %winid%
    return
}

^!i::  ; Ctrl+Alt+i
{
    WinGet, winid ,, A ;
    winId4 = %winid%
    ;MsgBox, i %winid%
    return
}

^!j::  ; Ctrl+Alt+j
{
    WinGet, winid ,, A ;
    winId5 = %winid%
    ;MsgBox, j %winid%
    return
}
