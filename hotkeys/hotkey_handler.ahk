#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

winId1 = 0
winId2Git = 0
winId3 = 0
winId4 = 0
winId5 = 0
winId6 = 0
winId7 = 0
winId8 = 0
winId9 = 0


; Swipe horizontal
^!t::  ; Ctrl+Alt+t
{
    WinActivate ahk_id %winId2Git%
    Send git diff{Enter}
    return
}

; Swipe vertical
^!u::  ; Ctrl+Alt+u
{
    WinActivate ahk_id %winId2Git%
    Send git log{Enter}
    return
}

; Swipe Arrow down
^!v::  ; Ctrl+Alt+v
{
    WinActivate ahk_id %winId2Git%
    Send git fetch && git pull origin $(git rev-parse --abbrev-ref HEAD){Enter}
    return
}

; Swipe 'C' as in COM Ports
^!w::  ; Ctrl+Alt+w
{
    listComPorts()
    return
}

; Swipe Arrow Right
^!x::  ; Ctrl+Alt+x
{
    send {Media_Next}
    return
}

; Swipe Arrow Up
^!y::  ; Ctrl+Alt+u
{
    WinActivate ahk_id %winId2Git%
    Send git push origin $(git rev-parse --abbrev-ref HEAD)
    return
}

; Swipe S
^!z::  ; Ctrl+Alt+v
{
    WinActivate ahk_id %winId2Git%
    Send git status{Enter}
    return
}

openIfMinimizedHideIfOpen(winid)
{
    DetectHiddenWindows, On
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

listComPorts()
{
    COMPort := Object()
    COMPortName := Object()
    Num:=0
    Array := []
    Loop, HKLM, HARDWARE\DEVICEMAP\SERIALCOMM\
    {
        RegRead, OutputVar
        COMPort%Num%:=OutputVar
        COM_result:=COMPort%Num%
        Loop, HKLM, SYSTEM\CurrentControlSet\Enum, 1, 1
        {
            if (A_LoopRegName = "FriendlyName")
            {
                RegRead, Outputvar
                IfInString,Outputvar,%COM_result%
                {
                    ;MsgBox  %COM_result%
                    If InStr(COM_result, "COM")
                    {
                        Array.Push(COM_result)
                    }
                }
            }

        }
    }
    res := ""
    for index, element in Array
    {
        res .= element
        res .= "`n"
    }

    ;MsgBox  %res%
    TrayTip , , %res%, 5, (16 + 32)
}

^!a::  ; Ctrl+Alt+a
{
    openIfMinimizedHideIfOpen(winId1)
    return
}

^!b::  ; Ctrl+Alt+b
{
    openIfMinimizedHideIfOpen(winId2Git)
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

^!f::  ; Ctrl+Alt+f
{
    openIfMinimizedHideIfOpen(winId6)
    return
}

^!g::  ; Ctrl+Alt+g
{
    openIfMinimizedHideIfOpen(winId7)
    return
}

^!h::  ; Ctrl+Alt+h
{
    openIfMinimizedHideIfOpen(winId8)
    return
}

^!i::  ; Ctrl+Alt+i
{
    openIfMinimizedHideIfOpen(winId9)
    return
}

; Config shortcuts
; Shortcut ^!a is configured by ^!f
; Shortcut ^!b is configured by ^!g
; etc.
; So to set the window connected to ^!a you will send ^!f
; and whatever window in focus will be connected to ^!a.

^!k::  ; Ctrl+Alt+k
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId1 = %winid%
    return
}

^!l::  ; Ctrl+Alt+l
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId2Git = %winid%
    return
}

^!m::  ; Ctrl+Alt+m
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId3 = %winid%
    return
}

^!n::  ; Ctrl+Alt+n
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId4 = %winid%
    return
}

^!o::  ; Ctrl+Alt+o
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId5 = %winid%
    return
}

^!p::  ; Ctrl+Alt+p
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId6 = %winid%
    return
}

^!q::  ; Ctrl+Alt+q
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId7 = %winid%
    return
}

^!r::  ; Ctrl+Alt+r
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId8 = %winid%
    return
}

^!s::  ; Ctrl+Alt+s
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId9 = %winid%
    return
}
