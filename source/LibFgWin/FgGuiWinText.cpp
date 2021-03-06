//
// Copyright (c) 2015 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Authors:     Andrew Beatty
// Created:     Oct 12, 2011
//

#include "stdafx.h"
#include "RichEdit.h"
#include "FgGuiApiText.hpp"
#include "FgGuiWin.hpp"
#include "FgThrowWindows.hpp"
#include "FgParse.hpp"

using namespace std;

struct  FgGuiWinTextRich : public FgGuiOsBase
{
    FgGuiApiText        m_api;
    HWND                hwndText;
    HWND                hwndThis;
    wstring             m_content;  // Assigned in constructor and in updateIfChanged()
    // Assigned in WM_CREATE and updated in updateIfChanged(). The min height is calculated using
    // m_maxMinWid below, regardless of actual width, to avoid the complexity of adjusting
    // Y-min dynamically with window width:
    FgVect2UI           m_minSize;
    // Long text will have a minimum width of at least the following:
    static const uint   m_maxMinWid = 300;
    static const uint   m_minHgt = 16;

    FgGuiWinTextRich(const FgGuiApiText & api) :
        m_api(api),
        // Default to something before we've had a chance to work out real vals:
        m_minSize(m_maxMinWid,m_minHgt)
    {
        static HMODULE hmRichEdit = LoadLibrary(L"RichEd20.dll");
        const FgString &        text = g_gg.getVal(m_api.content);
        g_gg.dg.update(m_api.updateFlagIdx);    // No need for redundant update
        m_content = text.as_wstring();
    }

    virtual void
    create(HWND parentHwnd,int ident,const FgString &,DWORD extStyle,bool visible)
    {
//fgout << fgnl << "FgGuiWinTextRich::create" << fgpush;
        FgCreateChild   cc;
        cc.extStyle = extStyle;
        cc.visible = visible;
        fgCreateChild(parentHwnd,ident,this,cc);
//fgout << fgpop;
    }

    virtual void
    destroy()
    {
        // Automatically destroys children (ie hwndText) first:
        DestroyWindow(hwndThis);
    }

    virtual FgVect2UI
    getMinSize() const
    {
        FGASSERT(m_minSize.volume() > 0);   // This shouldn't be called before it's been assigned.
        return m_minSize;
    }

    virtual FgVect2B
    wantStretch() const
    {return FgVect2B((m_minSize[0] > m_maxMinWid),false); }
    
    void
    updateText()
    {
        SetWindowTextW(hwndText,m_content.c_str());
        if (m_content.empty())      // Reserve a single line of max min width:
            m_minSize = FgVect2UI(m_maxMinWid,m_minHgt);
        else {
            // First get the size without wraparound. If the width of this is less than
            // m_maxMinWid then we can safely reduce with minimum width:
            RECT        r = {0,0,0,0};
            DrawText(GetDC(hwndText),m_content.c_str(),uint(m_content.size()),&r,DT_CALCRECT);
            // Just a guess but without this the calculated size is not quite large enough for the edit
            // box to actually render the text without wraparound:
            const uint  richEditBorder = 2;
            m_minSize[0] = uint(r.right - r.left) + richEditBorder;
            m_minSize[1] = uint(r.bottom - r.top) + richEditBorder;
            if (m_minSize[0] > m_maxMinWid) {   // Wraparound required for max min size:
                r.right = m_maxMinWid;
                DrawText(GetDC(hwndText),m_content.c_str(),uint(m_content.size()),&r,DT_CALCRECT | DT_WORDBREAK);
                m_minSize[0] = m_maxMinWid + richEditBorder;
                m_minSize[1] = r.bottom - r.top + richEditBorder;
            }
            if (m_minSize[0] < m_api.minWidth)
                m_minSize[0] = m_api.minWidth;
        }
    }

    virtual void
    updateIfChanged()
    {
        if (g_gg.dg.update(m_api.updateFlagIdx)) {
            const FgString &        text = g_gg.getVal(m_api.content);
            m_content = text.as_wstring(); 
            updateText();
            // TODO: if updateText() changes m_minSize we need to return a value (recursively)
            // so that main loop can call a resize on the whole window (which is potentially
            // affected).
        }
    }

    virtual void
    moveWindow(FgVect2I lo,FgVect2I sz)
    {MoveWindow(hwndThis,lo[0],lo[1],sz[0],sz[1],FALSE); }

    virtual void
    showWindow(bool s)
    {ShowWindow(hwndThis,s ? SW_SHOW : SW_HIDE); }

    LRESULT
    wndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
    {
        switch (message)
        {
            case WM_CREATE:
            {
//fgout << fgnl << "FgGuiWinTextRich::WM_CREATE";
                hwndThis = hwnd;
                hwndText = 
                    CreateWindowExW(0,
                        RICHEDIT_CLASSW,
                        L"HI",
                        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_READONLY,
                        0,0,0,0,
                        hwnd,
                        NULL,
                        s_fgGuiWin.hinst,
                        NULL);              // No WM_CREATE parameter
                FGASSERTWIN(hwndText != 0);
                SendMessage(hwndText,EM_SETBKGNDCOLOR,0,GetSysColor(COLOR_3DFACE));
                SendMessage(hwndText,EM_AUTOURLDETECT,TRUE,0);
                //uint mask = SendMessage(hwndText,EM_GETEVENTMASK,0,0);
                //mask |= ENM_LINK;
                SendMessage(hwndText,EM_SETEVENTMASK,0,ENM_LINK);
                // Call this here to ensure proper sizing from start:
                updateText();
                return 0;
            }
            case WM_SIZE:   // Sends new size of client area.
            {
                int     wid = LOWORD(lParam);
                int     hgt = HIWORD(lParam);
                if (wid*hgt > 0) {
                    MoveWindow(hwndText,0,0,wid,hgt,TRUE);
                }
                return 0;
            }
            case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                if (pnmh->code == EN_LINK) {
                    ENLINK *lnk = (ENLINK *)pnmh;
                    if (lnk->msg == WM_LBUTTONDOWN) {
                        SendMessage(pnmh->hwndFrom,EM_EXSETSEL,0,(LPARAM)&lnk->chrg);
                        vector<wchar_t> webLnk(lnk->chrg.cpMax - lnk->chrg.cpMin + 1);
                        SendMessage(pnmh->hwndFrom,EM_GETSELTEXT,0,(LPARAM)&webLnk[0]);
                        // Clear the selection:
                        CHARRANGE chrg;
                        chrg.cpMax = chrg.cpMin = lnk->chrg.cpMin;
                        SendMessage(pnmh->hwndFrom,EM_EXSETSEL,0,(LPARAM)&chrg);
                        // Goto the web link:
                        ShellExecuteW(hwnd,L"open",&webLnk[0],NULL,NULL,SW_SHOWNORMAL);
                        return TRUE;
                    }
                }
            }
        }
        return DefWindowProc(hwnd,message,wParam,lParam);
    }
};

FgSharedPtr<FgGuiOsBase>
fgGuiGetOsInstance(const FgGuiApiText & def)
{return FgSharedPtr<FgGuiOsBase>(new FgGuiWinTextRich(def)); }
