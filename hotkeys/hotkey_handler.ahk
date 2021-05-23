#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

winId1 = 0
winId2Git = 0
winId3 = 0
winId4 = 0
winId5 = 0

; Swipe vertical
^!k::  ; Ctrl+Alt+k
{
    WinActivate ahk_id %winId2Git%
    Send git log{Enter}
    return
}

; Swipe horizontal
^!l::  ; Ctrl+Alt+l
{
    WinActivate ahk_id %winId2Git%
    Send git status{Enter}
    return
}

; Swipe Arrow down
^!m::  ; Ctrl+Alt+m
{
    WinActivate ahk_id %winId2Git%
    Send git fetch && git pull origin $(git rev-parse --abbrev-ref HEAD){Enter}
    return
}

; Swipe 'C' as in COM Ports
^!n::  ; Ctrl+Alt+n
{
    listComPorts()
    return
}

; Swipe Arrow Right
^!o::  ; Ctrl+Alt+o
{
    send {Media_Next}
    return
}

; Swipe Arrow Up
^!p::  ; Ctrl+Alt+p
{
    WinActivate ahk_id %winId2Git%
    Send git push origin $(git rev-parse --abbrev-ref HEAD)
    return
}

; Swipe S
^!q::  ; Ctrl+Alt+q
{
    
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
    winId2Git = %winid%
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
