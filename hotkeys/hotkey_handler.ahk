#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

winId1 = 0
winId2Git = 0
winId3 = 0
winId4 = 0
winId5 = 0

; Swipe horizontal
^!p::  ; Ctrl+Alt+p
{
    WinActivate ahk_id %winId2Git%
    Send git log{Enter}
    return
}

; Swipe vertical
^!q::  ; Ctrl+Alt+q
{
    WinActivate ahk_id %winId2Git%
    Send git diff{Enter}
    return
}

; Swipe Arrow down
^!r::  ; Ctrl+Alt+r
{
    WinActivate ahk_id %winId2Git%
    Send git fetch && git pull origin $(git rev-parse --abbrev-ref HEAD){Enter}
    return
}

; Swipe 'C' as in COM Ports
^!s::  ; Ctrl+Alt+s
{
    listComPorts()
    return
}

; Swipe Arrow Right
^!t::  ; Ctrl+Alt+t
{
    send {Media_Next}
    return
}

; Swipe Arrow Up
^!u::  ; Ctrl+Alt+u
{
    WinActivate ahk_id %winId2Git%
    Send git push origin $(git rev-parse --abbrev-ref HEAD)
    return
}

; Swipe S
^!v::  ; Ctrl+Alt+v
{
    WinActivate ahk_id %winId2Git%
    Send git status{Enter}
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
