#include <windows.h>
#include <io.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

HWND window;
HFONT font;
DWORD vol=5;
DWORD msxdone=0;
DWORD msxskipped=0;

void DrawFrm(const wchar_t *string)
{
    HDC paintDC;
    int iwidth,iheight;
    RECT rct,trct;
    HBRUSH Br;
    BITMAPINFO bi;
    paintDC=GetDC(window);
    GetClientRect(window,&rct);

    iwidth = rct.right-rct.left;
    iheight = rct.bottom-rct.top;

    if (!(iwidth&&iheight))
        return;

    HDC iDC = CreateCompatibleDC(0);

    memset(&bi,0,sizeof(bi));
    bi.bmiHeader.biWidth=iwidth;
    bi.bmiHeader.biHeight=iheight;
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;

    HBITMAP iBitmap = CreateDIBSection(iDC, &bi, DIB_RGB_COLORS, 0, 0, 0);
    HGDIOBJ oldobj=SelectObject(iDC, iBitmap);

    Br = CreateSolidBrush(RGB(0, 0, 80));
    FillRect(iDC, &rct, Br);
    DeleteObject(Br);

    SelectObject (iDC, font) ;
    SetBkMode (iDC, TRANSPARENT) ;

    SetStretchBltMode(iDC, HALFTONE);

	// Szöveg rajzolás
    trct=rct;
    SetTextColor(iDC,0x40f0f0);/*
    int pos;
    DrawText(iDC,string,strlen(string),&rct,DT_CALCRECT);
    pos=(trct.bottom-trct.top-rct.bottom)/2;
    if (pos<0)
        pos=0;
    trct.top+=pos;
    trct.bottom=trct.top+rct.bottom;*/
    DrawTextW(iDC,string,wcslen(string),&trct,DT_LEFT);

    BitBlt(paintDC, 0, 0, iwidth, iheight, iDC, 0, 0, SRCCOPY);
    SelectObject(iDC, oldobj);
    DeleteObject(iBitmap);
    ReleaseDC(0,iDC);
    DeleteDC(iDC);
    ReleaseDC(window,paintDC);
}

//--------------------------------------------
class wchar
//--------------------------------------------
{
    wchar_t *m_str;
public:
    wchar_t *alloc(int size)
    {
        clear();
        m_str=new wchar_t[size];
        *m_str=0;
        return m_str;
    }
    wchar():m_str(0)
    {
    }
    void clear()
    {
        if (m_str)
            delete [] m_str;
        m_str=0;
    }
    const wchar &Format(const wchar_t *sFmtstr, ...)
    {
        if (sFmtstr)
        {
            clear();
            va_list arg_list;
            va_start(arg_list, sFmtstr);
            size_t len = _vsnwprintf(0, 0, sFmtstr, arg_list)+1;
            alloc(len+1);
            if (m_str)
                _vsnwprintf(m_str, len, sFmtstr, arg_list);
            va_end(arg_list);
        }
        return *this;
    }
    const wchar_t *operator+=(const wchar_t* par)
    {
        if (!m_str)
        {
            set(par);
            return m_str;
        }
        if (!par) return 0;
        int s1(wcslen(m_str)+1),s2(wcslen(par));
        s2+=s1;
        wchar_t *nstr=new wchar_t[s2];
        memcpy(nstr,m_str,s1*2);
        wcscat(nstr,par);
        clear();
        m_str=nstr;
        return m_str;
    }
    const wchar &operator =(const wchar &par)
    {
        set(par.m_str);
        return *this;
    }
    const wchar_t *operator =(const wchar_t *par)
    {
        set(par);
        return m_str;
    }
    wchar &set(const wchar_t *par)
    {
        if (par)
        {
            int size((int)wcslen(par)+1);
            wchar_t *nstr=new wchar_t[size];
            memcpy(nstr,par,size*2);
            clear();
            m_str=nstr;
        }
        else
        {
            clear();
        }
        return *this;
    }
    wchar_t *get() const
    {
        return m_str;
    }
    operator wchar_t *() const
    {
        return m_str;
    }
};

//--------------------------------------------
template <class TEMPLATE>
class tVec
//--------------------------------------------
{
    int m_count,m_size;
    TEMPLATE **m_ppArray;
public:
    typedef TEMPLATE *tpItem;
    tVec()
    {
        m_ppArray=0;
        m_count=0;
        m_size=0;
    }
    void Alloc(int count)
    {
        if (count>=m_size)
        {
            m_size=1+count+(count>>1);
            TEMPLATE **pp=new tpItem[m_size];
            if (m_ppArray&&pp)
            {
                memcpy(pp,m_ppArray,m_count*sizeof(tpItem));
                delete[] m_ppArray;
            }
            m_ppArray=pp;
        }
    }
    void Clear()
    {
        int i;
        for (i=0;i<m_count;i++)
            delete m_ppArray[i];
        delete m_ppArray;
        m_ppArray=0;
        m_count=0;
        m_size=0;
    }
    ~tVec()
    {
        Clear();
    }
    TEMPLATE &New()
    {
        Alloc(m_count+1);
        return *(m_ppArray[m_count++]=new TEMPLATE);
    }
    inline int GetCount()
    {
        return m_count;
    }
    TEMPLATE &Get(int id)
    {
        if (id<m_count) return *m_ppArray[id];
        else return *(TEMPLATE*)0;
    }
    void Del(int id)
    {
        if (id<m_count)
        {
            if (m_count-id-1)
            {
                delete m_ppArray[id];
                memmove(&m_ppArray[id],&m_ppArray[id+1],sizeof(tpItem)*(m_count-id-1));
            }
            --m_count;
        }
    }
    void operator=(tVec &param)
    {
        Clear();
        int i;
        for (i=0;i<m_count;i++)
            Add(param.Get(i));
    }
};

tVec<wchar> items;
wchar curfilename;

//--------------------------------------------
BOOL RegRemoveKey(HKEY hKeyParent, LPCTSTR pszSubKey)
//--------------------------------------------
{
     HKEY   hKeyTarget;
     BOOL   bResult = TRUE;
     char   *pszNameBuffer = NULL;
     DWORD  dwLen, dwMaxNameLen, dwSubkeyCount;
     LRESULT    lResult;

     lResult = RegOpenKeyEx(hKeyParent, pszSubKey, 0, KEY_READ, &hKeyTarget);
     if (lResult != ERROR_SUCCESS)
          return FALSE;

     lResult = RegQueryInfoKey(hKeyTarget, NULL, NULL, NULL, &dwSubkeyCount, &dwMaxNameLen, NULL, NULL, NULL, NULL, NULL, NULL);
     if (lResult != ERROR_SUCCESS)
     {
          RegCloseKey(hKeyTarget);
          return FALSE;
     }

     if (dwSubkeyCount > 0)
     {
          ++dwMaxNameLen;
          pszNameBuffer = (char *) malloc(dwMaxNameLen);
          if (pszNameBuffer == NULL)
          {
               RegCloseKey(hKeyTarget);
               return FALSE;
          }
          while (TRUE)
          {
               dwLen = dwMaxNameLen;
               lResult = RegEnumKeyEx(hKeyTarget, 0, pszNameBuffer, &dwLen, NULL, NULL, NULL, NULL);
               if (lResult == ERROR_NO_MORE_ITEMS)
                    break;
               else if (lResult == ERROR_SUCCESS)
               {
                    if (!RegRemoveKey(hKeyTarget, pszNameBuffer))
                    {
                         bResult = FALSE;
                         break;
                    }
               } else {
                    bResult = FALSE;
                    break;
               }
          }
     }
     RegCloseKey(hKeyTarget);
     free(pszNameBuffer);

     if (bResult)
     {
          lResult = RegDeleteKey(hKeyParent, pszSubKey);
          if (lResult != ERROR_SUCCESS)
               bResult = FALSE;
     }

     return bResult;
}

//--------------------------------------------
void ClearSettings()
//--------------------------------------------
{
    HKEY key;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS)
    {
        RegRemoveKey(key,"volfix");
        RegCloseKey(key);
    }
}

//--------------------------------------------
void SetDWORD(const char* keyID, DWORD value)
//--------------------------------------------
{
    HKEY key;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\volfix", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0) == ERROR_SUCCESS)
    {
        RegSetValueEx(key, keyID, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
        RegCloseKey(key);
    }
}

//--------------------------------------------
DWORD GetDWORD(const char* keyID, DWORD &dwval)
//--------------------------------------------
{
    HKEY key;
    DWORD retval=0;
    DWORD dwKeyType;
    DWORD dwSize;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\volfix", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0) == ERROR_SUCCESS)
    {
        dwKeyType=REG_DWORD;
        dwSize=sizeof(DWORD);
        if (RegQueryValueEx(key, keyID, 0, &dwKeyType, (BYTE*)&dwval, &dwSize) == ERROR_SUCCESS)
        {
            retval=1;
        }
        RegCloseKey(key);
    }
    return retval;
}
//--------------------------------------------
wchar_t *GetOnlyFile(wchar_t *filename)
//--------------------------------------------
{
    wchar_t *pfilename=0;
    if (filename)
        pfilename=(wchar_t*)wcsrchr(filename,'\\');
    if (pfilename)
        pfilename++;
    else
        pfilename=filename;
    return pfilename;
}

void Draw(wchar_t *string=0,int precent=-1)
{
    wchar str, fm;
    str.Format(L"Vol:%d [%d:%d-%d]\n",vol,msxdone,items.GetCount()+(curfilename?1:0),msxskipped);
    if (string)
    {
        if (precent>=0)
        {
            fm.Format(L"%02d%% - %s ",precent,GetOnlyFile(string));
            str+=fm;
        } else str+=GetOnlyFile(string);
    }
    int i,cnt=items.GetCount();
    if (cnt>5) cnt=5;
    for (i=0;i<cnt;i++)
    {
        if (i||string)
            str+=L"\n";
        str+=GetOnlyFile(items.Get(i));
    }

    if (str)
        string=str;
    DrawFrm(string);
}

extern "C"
{
    int lprec=0;
    void precentdisplay(int precent)
    {
        if (lprec!=precent)
        {
            lprec=precent;
            Draw(curfilename,precent);
        }
    }

    int donorm(int argc, wchar_t **argv);
}

void fillitems(wchar_t *dir)
{
    /*
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    CHAR cFileName[MAX_PATH];
    */
    int len=GetCurrentDirectoryW(0,0);
    wchar_t *pdir=new wchar_t[len];
    GetCurrentDirectoryW(len,pdir);
    SetCurrentDirectoryW(dir);
    WIN32_FIND_DATAW FindFileData;
    HANDLE hFind;
    hFind = INVALID_HANDLE_VALUE;
    hFind = FindFirstFileW(L"*.*", &FindFileData);

    wchar actfile;

    if (hFind != INVALID_HANDLE_VALUE)
    {
        for (;;)
        {
            actfile.Format(L"%s\\%s",dir,FindFileData.cFileName);
            if (FindFileData.cFileName&&*FindFileData.cFileName!='.')
                items.New().set(actfile);
            if (!FindNextFileW(hFind, &FindFileData))
                break;
        }

        FindClose(hFind);
    }
    SetCurrentDirectoryW(pdir);
    delete [] pdir;
}

void doit(wchar_t *filename,int parvol)
{
    if (GetFileAttributesW(filename)==INVALID_FILE_ATTRIBUTES)
    {
        msxskipped++;
        return;
    } else if (GetFileAttributesW(filename)& FILE_ATTRIBUTE_DIRECTORY)
    {
        fillitems(filename);
        return;
    }

    const wchar_t *ext=wcsrchr(filename,'.');
    if (ext&&wcscmp(ext,L".mp3"))
        return;
    curfilename=filename;
    wchar vol;
    wchar_t *args[5]={0,(wchar_t*)L"/r",(wchar_t*)L"/c",0,0};
    vol.Format(L"/d%d",parvol);
    args[0]=(wchar_t*)L"exefile";
    args[3]=vol;
    args[4]=filename;
    wprintf(L"%s %s %s %s",args[0],args[1],args[2],args[3]);
    //Sleep(500);
    donorm(5, args);
    msxdone++;
    curfilename=0;
}

void Redraw()
{
            InvalidateRect(window,0,TRUE);
            UpdateWindow(window);
}

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC paintDC;

    switch (message)
    {
    case WM_ERASEBKGND:
    {
        if (items.GetCount()>0)
            Draw(0);
        else
            Draw((wchar_t*)L"Drop mp3 files");
    }
    break;
    case WM_PAINT:
        paintDC=BeginPaint(hwnd,&ps);
        EndPaint(hwnd,&ps);
        break;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            vol=wParam-'0';
            Redraw();
            break;
        case VK_ESCAPE:
            DestroyWindow(window);
            break;
        }
        break;
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        int i, count = DragQueryFile( hDrop, 0xFFFFFFFF, 0, 0 );
        for (i=0;i<count;i++)
        {
            wchar tfile;
            UINT iSize = DragQueryFile(hDrop, i, 0,0)+1;
            tfile.alloc(iSize);
            DragQueryFileW(hDrop, i, tfile.get(),iSize);
            items.New().set(tfile);
        }
        DragFinish(hDrop);
        SetForegroundWindow(window);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage (0);
        break;
    default:
        return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow)
{
    char szClassName[ ] = "V0lf1X";
    MSG msg;
    WNDCLASSEX wndclass;

    memset(&wndclass,0,sizeof(wndclass));
    wndclass.hInstance = hThisInstance;
    wndclass.lpszClassName = szClassName;
    wndclass.lpfnWndProc = WindowProcedure;
    wndclass.style = CS_DBLCLKS;
    wndclass.cbSize = sizeof (wndclass);

    wndclass.hIcon = LoadIcon (hThisInstance, MAKEINTRESOURCE(2000));
    wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) COLOR_DESKTOP;

    if (!RegisterClassEx (&wndclass))
        return 0;

    window = CreateWindowEx (0,szClassName,"V0lf1X",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,
                             700,200, HWND_DESKTOP, NULL, hThisInstance, NULL );

    font = CreateFontW(	30,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_PRECIS,
                        CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_DONTCARE|DEFAULT_PITCH,L"Arial");

    GetDWORD("volume",vol);
    DragAcceptFiles(window,true);
    ShowWindow (window, nCmdShow);

    bool bQuit=FALSE;

    while (!bQuit)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else if (items.GetCount()>0)
        {
            wchar str=items.Get(0);
            items.Del(0);
            doit(str,vol-5);
            Redraw();
            Sleep(1);
        }
    }

    DeleteObject(font);
    SetDWORD("volume",vol);

    return msg.wParam;
}
