#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.


; Read the value from the .ini file.
if FileExist("SavedVals.ini") {
    MsgBox, SavedVals.ini exists.
    IniRead, winId1, SavedVals.ini, SavedValsSection, winId1
    IniRead, winId2, SavedVals.ini, SavedValsSection, winId2
    IniRead, winId3, SavedVals.ini, SavedValsSection, winId3
    IniRead, winId4, SavedVals.ini, SavedValsSection, winId4
    IniRead, winId5, SavedVals.ini, SavedValsSection, winId5
    IniRead, winId6windows, SavedVals.ini, SavedValsSection, winId6windows
    IniRead, winId7gitlab, SavedVals.ini, SavedValsSection, winId7gitlab
    IniRead, winId8, SavedVals.ini, SavedValsSection, winId8
    IniRead, winId9Git, SavedVals.ini, SavedValsSection, winId9Git
    IniRead, winId10, SavedVals.ini, SavedValsSection, winId10
} else {
    winId1 = 0
    winId2 = 0
    winId3 = 0
    winId4 = 0
    winId5 = 0
    winId6windows = 0
    winId7gitlab = 0
    winId8 = 0
    winId9Git = 0
    winId10 = 0
}

; Set it to run code when the script ends, such as shutdown, clicking exit from the tray icon, reloading, etc.
OnExit, ExitProgram

DetectHiddenWindows, On

; Swipe horizontal
^!+1::  ; Ctrl+Alt+shift+1
{
    WinActivate ahk_id %winId9Git%
    Send git diff{Enter}
    return
}

; Swipe vertical
^!+2::  ; Ctrl+Alt+shift+2
{
    WinActivate ahk_id %winId9Git%
    Send git log{Enter}
    return
}

; Swipe Arrow down
^!+3::  ; Ctrl+Alt+shift+3
{
    WinActivate ahk_id %winId9Git%
    Send git fetch && git pull origin $(git rev-parse --abbrev-ref HEAD){Enter}
    return
}

; Swipe 'C' as in COM Ports
^!+4::  ; Ctrl+Alt+shift+4
{
    listComPorts()
    return
}

; Swipe Arrow Right
^!+5::  ; Ctrl+Alt+shift+5
{
    send {Media_Next}
    return
}

; Swipe Arrow Up
^!+6::  ; Ctrl+Alt+shift+6
{
    WinActivate ahk_id %winId9Git%
    Send git push origin $(git rev-parse --abbrev-ref HEAD)
    return
}

; Swipe S
^!+7::  ; Ctrl+Alt+shift+7
{
    WinActivate ahk_id %winId9Git%
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

^!f::  ; Ctrl+Alt+f
{
    Send #{tab} ;Win + tab (show all windows)
    ;openIfMinimizedHideIfOpen(winId6windows)
    return
}

^!g::  ; Ctrl+Alt+g
{
    Run, chrome.exe "https://git-sho-mlm.u-blox.net/" " --new-window "
    ;openIfMinimizedHideIfOpen(winId7gitlab)
    return
}

^!h::  ; Ctrl+Alt+h
{
    openIfMinimizedHideIfOpen(winId8)
    return
}

^!i::  ; Ctrl+Alt+i
{
    openIfMinimizedHideIfOpen(winId9Git)
    return
}

^!j::  ; Ctrl+Alt+j
{
    openIfMinimizedHideIfOpen(winId10)
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
    winId2 = %winid%
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
    winId6windows = %winid%
    return
}

^!q::  ; Ctrl+Alt+q
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId7gitlab = %winid%
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
    winId9Git = %winid%
    return
}

^!t::  ; Ctrl+Alt+t
{
    SoundBeep, 150, 250
    WinGet, winid ,, A ;
    winId10 = %winid%
    return
}

; return so that ExitProgram is not executed now
return

; The code to run when it exits.
ExitProgram:
	IniWrite, %winId1%, SavedVals.ini, SavedValsSection, winId1
    IniWrite, %winId2%, SavedVals.ini, SavedValsSection, winId2
    IniWrite, %winId3%, SavedVals.ini, SavedValsSection, winId3
    IniWrite, %winId4%, SavedVals.ini, SavedValsSection, winId4
    IniWrite, %winId5%, SavedVals.ini, SavedValsSection, winId5
    IniWrite, %winId6windows%, SavedVals.ini, SavedValsSection, winId6windows
    IniWrite, %winId7gitlab%, SavedVals.ini, SavedValsSection, winId7gitlab
    IniWrite, %winId8%, SavedVals.ini, SavedValsSection, winId8
    IniWrite, %winId9Git%, SavedVals.ini, SavedValsSection, winId9Git
    IniWrite, %winId10%, SavedVals.ini, SavedValsSection, winId10
    ExitApp
