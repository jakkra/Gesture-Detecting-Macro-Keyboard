#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

winId1 = 0
winId2 = 0
winId3 = 0
winId4 = 0
winId5 = 0

; Swipe vertical
^!k::  ; Ctrl+Alt+l
{
    Send git fetch && git pull origin $(git rev-parse --abbrev-ref HEAD){Enter}
    return
}

; Swipe horizontal
^!l::  ; Ctrl+Alt+l
{
    Send git fetch && git pull origin $(git rev-parse --abbrev-ref HEAD){Enter}
    return
}

; Swipe v
^!m::  ; Ctrl+Alt+l
{
    Send git fetch && git pull origin $(git rev-parse --abbrev-ref HEAD){Enter}
    return
}


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
; Shortcut ^!b is configured by ^!g
; etc.
; So to set the window connected to ^!a you will send ^!f
; and whatever window in focus will be connected to ^!a.

^!f::  ; Ctrl+Alt+f
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId1 = %winid%
    return
}

^!g::  ; Ctrl+Alt+g
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId2 = %winid%
    return
}

^!h::  ; Ctrl+Alt+h
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId3 = %winid%
    return
}

^!i::  ; Ctrl+Alt+i
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId4 = %winid%
    return
}

^!j::  ; Ctrl+Alt+j
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId5 = %winid%
    return
}
