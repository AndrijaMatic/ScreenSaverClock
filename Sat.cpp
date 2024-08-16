 
#include <windows.h>
#include <scrnsave.h>
#include <math.h>
#include <tchar.h>//Za _tcslen funkciju
#include <stdio.h>
#include <commdlg.h>
 

#include "resource.h"
#pragma warning( disable : 4244 )//Onesposobljava upozorena o pretvaranj float u int
								 //i double u float. Moguè gubitak podataka. 23 upozorenja
//---------------------------------------------------------------------------
#define TIMER_ID 100
#define RAD	     0.017453293
#define PI	     3.141592654
 
//---------------------------------------------------------------------------
HWND hWnd;
HINSTANCE hInstance;
HDC  hDC;
RECT rc;
//Bitmape za analogni sat
HBITMAP hbmpDvanest;
HBITMAP hbmpTri;
HBITMAP hbmpSest;
HBITMAP hbmpDevet;
HBITMAP hbmpOstali;

HDC hDCMem;
HBRUSH hBrushRed ; //Brush za digitalni
//Penovi za analogni
HPEN hPenZuti;
HPEN hPenZutiDebel;
HPEN hPenCrni;
int visina;
int sirina;	  
int    cxClient, cyClient ;
static BOOL fSat; //Koju vrtu sata prikazuje
static BOOL fZvono;//Dali æu zvoniti?
char buffer[256]="";
int Sati,Minute;//U koliko æu zvoniti
char imezvuka[MAX_PATH];

 /*------Funkcije za citanja i pisanje po registriju-------------- 

BOOL CitajRegisteyBool(char varijabla[])
BOOL ZapisiRegistryBool(BOOL flag,char varijabla[])
char* CitajRegistryString(char varijabla[])
BOOL ZapisiRegistryString(char vrijednost[],char varijabla[])
-----------------------------------------------------------------*/

BOOL CitajRegisteyBool(char varijabla[])
{
	                          
    HKEY hKey;
 	DWORD res;
	BOOL fBool;
	DWORD BoolSize=sizeof(BOOL);
	 
	res=RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\andmatic",0,KEY_ALL_ACCESS,&hKey);
	if(res!=ERROR_SUCCESS){
	res=RegCreateKey(HKEY_CURRENT_USER,"Software\\andmatic", &hKey); 
	RegCloseKey(hKey);}
	res=RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\andmatic",0,KEY_ALL_ACCESS,&hKey);
	res=RegQueryValueEx(hKey,varijabla,NULL,NULL,(BYTE*)&fBool,&BoolSize);
   	RegCloseKey(hKey);
	
	return fBool;
}

BOOL ZapisiRegistryBool(BOOL flag,char varijabla[])
{
	HKEY hKey;
 	DWORD res;
  
	res=RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\andmatic",0,KEY_SET_VALUE,&hKey);
	if(res!=ERROR_SUCCESS){
		MessageBox(NULL,"Greska 1","",MB_OK);
		return FALSE;}
	res=RegSetValueEx(hKey,varijabla,NULL,REG_DWORD,(BYTE*)&flag, sizeof(BOOL) );
	if(res!=ERROR_SUCCESS){
			MessageBox(NULL,"Greska 2","",MB_OK);
			return FALSE;
	}
	 RegCloseKey(hKey);

 	return TRUE;
}

char* CitajRegistryString(char varijabla[])
{
	    HKEY hKey;
 	DWORD res;

	DWORD bufferSize=sizeof(buffer);
	 
	res=RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\andmatic",0,KEY_ALL_ACCESS,&hKey);
	if(res!=ERROR_SUCCESS){
	res=RegCreateKey(HKEY_CURRENT_USER,"Software\\andmatic", &hKey); 
	RegCloseKey(hKey);}
	res=RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\andmatic",0,KEY_ALL_ACCESS,&hKey);
	res=RegQueryValueEx(hKey,varijabla,NULL,NULL,(BYTE*)&buffer,&bufferSize);
   	RegCloseKey(hKey);
	
	return buffer;
}

BOOL ZapisiRegistryString(char vrijednost[],char varijabla[])
{
	HKEY hKey;
 	DWORD res;
  
	res=RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\andmatic",0,KEY_SET_VALUE,&hKey);
	if(res!=ERROR_SUCCESS){
		MessageBox(NULL,"Greska 1","",MB_OK);
		return FALSE;}
	res=RegSetValueEx(hKey,varijabla,NULL,REG_SZ,(BYTE*)vrijednost,(_tcslen(vrijednost)+1)*sizeof(char) );
	if(res!=ERROR_SUCCESS){
			MessageBox(NULL,"Greska 2","",MB_OK);
			return FALSE;
	}
	 RegCloseKey(hKey);

 	return TRUE;
}

// Èita iz buffera sat i minutu kada treba zvoniti,provjerava
// i pretvara ih u int
BOOL VrijednostSata()
{
	if(_tcslen(buffer)<4)
		return FALSE;
	char Sati[2],Minute[2];
	Sati[0]=buffer[0];
	Sati[1]=buffer[1];
	Sati[2]=NULL;
	Minute[0]=buffer[2];
	Minute[1]=buffer[3];
	Minute[2]=NULL;

	::Sati=atoi(Sati);
	::Minute=atoi(Minute);
 
	if (::Sati>23)
		return FALSE;
	if( ::Minute>59)
		return FALSE;
	return TRUE;
}
//Hook za openfile dialogbox izmjenjuje text na hrvatski jezik
UINT_PTR CALLBACK OFNHookProc(HWND hdlg,UINT uiMsg,WPARAM wParam,LPARAM lParam)
{
 
	HWND hWndParent;
	switch(uiMsg){
	case WM_INITDIALOG :
		hWndParent=GetParent(hdlg);
		SetDlgItemText(hWndParent,IDOK,"Odaberi");
		SetDlgItemText(hWndParent,IDCANCEL,"Odustani");
		SetDlgItemText(hWndParent,stc4,"Traži u:");
		SetDlgItemText(hWndParent,stc3,"Ime datoteke:");
		SetDlgItemText(hWndParent,stc2,"Vrsta datoteke:");
		return TRUE;
	}
return FALSE;}

/*-------Funkcija za selektiranje zvuènog faila za zvono---*/
static OPENFILENAME ofn ;

void PopFileInitialize ()
{
	ZeroMemory(&ofn, sizeof(OPENFILENAME));

     static TCHAR szFilter[] = TEXT ("Zvuk (*.Wav)\0*.wav\0");
                                
     ofn.lStructSize       = sizeof (OPENFILENAME) ;
     ofn.hwndOwner         = ::hWnd;
     ofn.hInstance         =(HINSTANCE)GetWindowLong(::hWnd,GWL_HINSTANCE);
     ofn.lpstrFilter       = szFilter ;
     ofn.lpstrCustomFilter = NULL ;
     ofn.nMaxCustFilter    = 0 ;
     ofn.nFilterIndex      = 0 ;
     ofn.lpstrFile         = imezvuka;           
     ofn.nMaxFile          = MAX_PATH ;
     ofn.lpstrFileTitle    = NULL;          
     ofn.nMaxFileTitle     = MAX_PATH ;
     ofn.lpstrInitialDir   = NULL ;
     ofn.lpstrTitle        = "Odabreite zvuk za zvono";
     ofn.Flags             =  OFN_ENABLEHOOK | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |  OFN_FILEMUSTEXIST; 
     ofn.nFileOffset       = 0 ;
     ofn.nFileExtension    = 0 ;
     ofn.lpstrDefExt       = TEXT ("wav") ;
     ofn.lCustData         = 0L ;
     ofn.lpfnHook          =OFNHookProc;
     ofn.lpTemplateName    =NULL;

}

/*--------------Funkcije za crtanje digitalnog sata------------------*/
void DisplayDigit (int iNumber) //Crtanje broja
{
     static BOOL  fSevenSegment [10][7] = {//Što æu crtat?
                         1, 1, 1, 0, 1, 1, 1,     // 0
                         0, 0, 1, 0, 0, 1, 0,     // 1
                         1, 0, 1, 1, 1, 0, 1,     // 2
                         1, 0, 1, 1, 0, 1, 1,     // 3
                         0, 1, 1, 1, 0, 1, 0,     // 4
                         1, 1, 0, 1, 0, 1, 1,     // 5
                         1, 1, 0, 1, 1, 1, 1,     // 6
                         1, 0, 1, 0, 0, 1, 0,     // 7
                         1, 1, 1, 1, 1, 1, 1,     // 8
                         1, 1, 1, 1, 0, 1, 1 } ;  // 9
     static POINT ptSegment [7][6] = {//Kako æu nacrtat?
                          7,  6,  11,  2,  31,  2,  35,  6,  31, 10,  11, 10,
                          6,  7,  10, 11,  10, 31,   6, 35,   2, 31,   2, 11,
                         36,  7,  40, 11,  40, 31,  36, 35,  32, 31,  32, 11,
                          7, 36,  11, 32,  31, 32,  35, 36,  31, 40,  11, 40,
                          6, 37,  10, 41,  10, 61,   6, 65,   2, 61,   2, 41,
                         36, 37,  40, 41,  40, 61,  36, 65,  32, 61,  32, 41,
                          7, 66,  11, 62,  31, 62,  35, 66,  31, 70,  11, 70 } ;
     int          iSeg ;

     for (iSeg = 0 ; iSeg < 7 ; iSeg++)
          if (fSevenSegment [iNumber][iSeg])
               Polygon (hDC, ptSegment [iSeg], 6) ;
}
//Kako ispiati dvije znamenke npr. 05 ili 5
void DisplayTwoDigits (int iNumber, BOOL fSuppress)
{
     if (!fSuppress || (iNumber / 10 != 0))
          DisplayDigit (iNumber / 10) ;
     OffsetWindowOrgEx (hDC,-42, 0, NULL) ;
     DisplayDigit (iNumber % 10) ;
     OffsetWindowOrgEx (hDC, -42, 0, NULL) ;
}
//Ispiši razmak izmeðu sat:min:sec
void DisplayColon ()
{
     POINT ptColon [2][4] = { 2,  21,  6,  17,  10, 21,  6, 25,
                              2,  51,  6,  47,  10, 51,  6, 55 } ;

     Polygon (hDC, ptColon [0], 4) ;
     Polygon (hDC, ptColon [1], 4) ;

     OffsetWindowOrgEx (hDC, -12, 0, NULL) ;
}

void DisplayTime ( BOOL f24Hour=TRUE, BOOL fSuppress=FALSE)
{
     SYSTEMTIME st ;

     GetLocalTime (&st) ;

     if (f24Hour)
          DisplayTwoDigits (st.wHour, fSuppress) ;
     else
          DisplayTwoDigits ((st.wHour %= 12) ? st.wHour : 12, fSuppress) ;

     DisplayColon () ;
     DisplayTwoDigits ( st.wMinute, FALSE) ;
     DisplayColon () ;
     DisplayTwoDigits (st.wSecond, FALSE) ;
}

//---------------------------------------------- 
////////////////////////
//Iniciranje SSavera
BOOL OnCreate()
{	
 
	fSat=CitajRegisteyBool("VrstaSSavera");
 	if(!(fZvono=CitajRegisteyBool("Zvono")))
	{
		//Citanje varijabli za zvono
		//Ime faila za zvuk zvona i vrijeme zvonjave
		//moraju iæi baš ovim redom jer koriste istu 
		//globalnu varijablu Buffer.
		CitajRegistryString("VrijemeZvonjave");
		VrijednostSata();
		CitajRegistryString("FailZaZvuk");
	}
	hDC = GetDC(hWnd);
	SetTimer(hWnd,TIMER_ID,1000,NULL);//Opali WM_TIME svake sekunde
	if(fSat)//Analogni else digitalni
	{
		hbmpDvanest=::LoadBitmap(hInstance,"dvanest");
		hbmpTri=::LoadBitmap(hInstance,"tri");
		hbmpSest=::LoadBitmap(hInstance,"sest");
		hbmpDevet=::LoadBitmap(hInstance,"devet");
		hbmpOstali=::LoadBitmap(hInstance,"ostali");
		hPenZuti=CreatePen(PS_SOLID,2,RGB(248,191,36));
		hPenCrni=CreatePen(PS_SOLID,12,RGB(0,0,0));
		hPenZutiDebel=CreatePen(PS_SOLID,12,RGB(248,191,36));
		SetViewportOrgEx(hDC,sirina,visina,NULL);
		SelectObject(hDC,hPenZuti);
		return TRUE;
	}
	else
	{
		hBrushRed = CreateSolidBrush (RGB (248,191,36)) ;
		SelectObject(hDC,hBrushRed);
	}

	return TRUE;
}
//---------------------------------------------------------------------------
void OnDestroy()
{
	PlaySound(NULL,NULL,NULL);//Ako svira zvuk uništi ga
	if(fSat)
	{
		DeleteObject(hbmpDvanest);
		DeleteObject(hbmpTri);
		DeleteObject(hbmpSest);
		DeleteObject(hbmpDevet);
		DeleteObject(hbmpOstali);
		DeleteObject(hPenZuti);
		DeleteObject(hPenCrni);
		DeleteObject(hPenZutiDebel);
	}
	else
		DeleteObject(hBrushRed);
	KillTimer(hWnd,TIMER_ID); 
	if(hDC)
	{
	ReleaseDC(hWnd,hDC);
	hDC = NULL;
	}
}

void OnPaint(HDC PaintDC){
	if(fSat)//Ananlogni else digitalni
	{
		hDCMem=CreateCompatibleDC(PaintDC);

		SetViewportOrgEx(PaintDC,sirina,visina,NULL);
		SelectObject(hDCMem,hbmpDvanest);
		BitBlt(PaintDC,0-50,-300,100,100,hDCMem,0,0,SRCCOPY);
		SelectObject(hDCMem,hbmpSest);
		BitBlt(PaintDC,0-50,200,100,100,hDCMem,0,0,SRCCOPY);
	 	SelectObject(hDCMem,hbmpTri);
		BitBlt(PaintDC,200,-50,100,100,hDCMem,0,0,SRCCOPY);
		SelectObject(hDCMem,hbmpDevet);
		BitBlt(PaintDC,-300,-50,100,100,hDCMem,0,0,SRCCOPY);
		SelectObject(hDCMem,hbmpOstali);
		for(int i=0;i<12;i++)
		{
			if(!(i%3))continue;
			int X=(cos(0.5235*i)*250)-50;
			int Y=(sin(0.5235*i)*250)-50;
			BitBlt(PaintDC,X,Y,100,100,hDCMem,0,0,SRCCOPY);
		}

		DeleteDC(hDCMem);
		return;
	}
	else
	{
		  SetMapMode (hDC, MM_ISOTROPIC) ;
          SetWindowExtEx (hDC, 276, 72, NULL) ;
          SetViewportExtEx (hDC, cxClient, cyClient, NULL) ;
          SetWindowOrgEx (hDC, 138, 36, NULL) ;
          SetViewportOrgEx (hDC, cxClient / 2, cyClient / 2, NULL) ;
          SelectObject (hDC, GetStockObject (NULL_PEN)) ;
          SelectObject (hDC, hBrushRed) ;
          DisplayTime () ;
		  return;
	}

         
}

//Pokretanje alarma u odreðeno vrijeme
void OdvaliZvono()
{
	static BOOL flag;
	if(!flag)
	{
		RECT rc;
		GetWindowRect(::hWnd,&rc);
		if(rc.top==0)
		{
			PlaySound(buffer,NULL,SND_FILENAME | SND_LOOP | SND_ASYNC);
		}
		flag=TRUE;
	}
}
//---------------------------------------------------------- 
///////////////////////
//Crtanje svake sekunde
void OnDraw()
{
		SYSTEMTIME time;
		GetLocalTime(&time);
		if(!fZvono)//Dali je zvono ukljuèeno 
		if((time.wHour==Sati) && (time.wMinute==Minute))//Oèu li zvoniti?
			OdvaliZvono();//Zvoni
	if(fSat)//Analogni else digitalni
	{	//Zapamti gdje su kazaljke pa da ih poslje izbrišem
		static float xSekunde,ySekunde;
		static float xMinute,yMinute;
		static float xSati,ySati;
		float temp;

		temp=PI-time.wSecond *6*RAD;
		SelectObject(hDC,hPenCrni);
		LineTo(hDC,xSekunde*200,ySekunde*200);
		SelectObject(hDC,hPenZuti);
		xSekunde=sin (temp) ;
		ySekunde=cos (temp);
 		MoveToEx(hDC,0,0,NULL);
		LineTo(hDC,xSekunde*200,ySekunde*200);
		MoveToEx(hDC,0,0,NULL);
		temp=PI-time.wMinute *6*RAD;
		SelectObject(hDC,hPenCrni);
		LineTo(hDC,xMinute*200,yMinute*200);
		SelectObject(hDC,hPenZutiDebel);
		xMinute=sin (temp) ;
		yMinute=cos (temp);
 		MoveToEx(hDC,0,0,NULL);
		LineTo(hDC,xMinute*200,yMinute*200);
		MoveToEx(hDC,0,0,NULL);
		temp=PI-time.wHour *30*RAD-time.wMinute *RAD/2;
		SelectObject(hDC,hPenCrni);
		LineTo(hDC,xSati*150,ySati*150);
		SelectObject(hDC,hPenZutiDebel);
		xSati=sin (temp) ;
		ySati=cos (temp);
 		MoveToEx(hDC,0,0,NULL);
		LineTo(hDC,xSati*150,ySati*150);
		Ellipse(hDC,-20,-20,20,20);
		MoveToEx(hDC,0,0,NULL);
		return;
	}
	else
 	InvalidateRect (hWnd,NULL,TRUE);//Digitalni se crta u OnPaint funkciji
	

}
//--------Funkcija koja se poziva kod podešavanja SSavera-----------------
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{ 
	DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_INFO),NULL,ScreenSaverConfigureDialog);
	return TRUE; 
}

//--------------Dijalog za konfiguraciju---------------------
BOOL WINAPI ScreenSaverConfigureDialog(HWND hWndDailog,UINT msg,WPARAM wParam,LPARAM lParam) 
{ 
	HWND hwndChild;
     switch (msg)
     {
		 ////////////////////
		 //Iniciranje dialoga
     case WM_INITDIALOG :
		  
		 ::hWnd=hWndDailog;
		PopFileInitialize();//Inicira strukturu za dialog		  
		 if(CitajRegisteyBool("VrstaSSavera"))
		 {
			 hwndChild = GetDlgItem (hWndDailog,ID_ANALOGNI) ;
			 SendMessage(hwndChild,BM_SETCHECK, 1, 0) ;
		 }
		 else
		 {
			 hwndChild = GetDlgItem (hWndDailog,ID_DIGITALNI) ;
			 SendMessage(hwndChild,BM_SETCHECK, 1, 0) ;
		 }
		 if(CitajRegisteyBool("Zvono"))
		 {
			 hwndChild = GetDlgItem (hWndDailog,IDC_FZVONO) ;
			 SendMessage(hwndChild,BM_SETCHECK, 1, 0);
			 hwndChild = GetDlgItem (hWndDailog,IDC_EDIT1) ;
			 EnableWindow (hwndChild, FALSE) ;
			 hwndChild = GetDlgItem (hWndDailog,IDC_EDIT2) ;
			 EnableWindow (hwndChild, FALSE) ;
			 hwndChild = GetDlgItem (hWndDailog,IDB_BROWSE) ;
			 EnableWindow (hwndChild, FALSE) ;
		 }
		
		  SetDlgItemText(hWndDailog,IDC_EDIT1,CitajRegistryString("FailZaZvuk"));
		  SetDlgItemText(hWndDailog,IDC_EDIT2,CitajRegistryString("VrijemeZvonjave"));
		  hwndChild=GetDlgItem(hWndDailog,IDC_EDIT2) ;
		  SendMessage(hwndChild,EM_SETLIMITTEXT ,(WPARAM)4,0);
	 
         return TRUE ;
          
     case WM_COMMAND :
          switch (LOWORD (wParam))
          {
		  case IDC_FZVONO:
			  if(HIWORD(wParam)==BN_CLICKED)
			  {
				  //MessageBox(NULL,"Netko me je kliknuo","",MB_OK);
				 if(SendMessage ((HWND) lParam, BM_GETCHECK, 0, 0))//~IF
				 {
					 			hwndChild = GetDlgItem (hWndDailog,IDC_EDIT1) ;
								EnableWindow (hwndChild, FALSE) ;
								hwndChild = GetDlgItem (hWndDailog,IDC_EDIT2) ;
								EnableWindow (hwndChild, FALSE) ;
							    hwndChild = GetDlgItem (hWndDailog,IDB_BROWSE) ;
								EnableWindow (hwndChild, FALSE) ;
								return TRUE;
				 }
				 else
				 {
					 			hwndChild = GetDlgItem (hWndDailog,IDC_EDIT1) ;
								EnableWindow (hwndChild, TRUE) ;
								hwndChild = GetDlgItem (hWndDailog,IDC_EDIT2) ;
								EnableWindow (hwndChild, TRUE);
						        hwndChild = GetDlgItem (hWndDailog,IDB_BROWSE) ;
			                    EnableWindow (hwndChild, TRUE) ;
							    return TRUE;
				 }
				 
			  }
			  return TRUE;
		  case IDB_BROWSE:
			  GetOpenFileName(&ofn);
			  SetDlgItemText(hWndDailog,IDC_EDIT1,ofn.lpstrFile);
			  return TRUE;
			 
          case IDOK ://Kraj dialoga i zapisivanje u registri
			 hwndChild = GetDlgItem (hWndDailog,ID_ANALOGNI) ;
			 if(SendMessage (hwndChild, BM_GETCHECK, 0, 0))
			      ZapisiRegistryBool(TRUE,"VrstaSSavera");
			 else
		          ZapisiRegistryBool(FALSE,"VrstaSSavera");

			 hwndChild = GetDlgItem (hWndDailog,IDC_FZVONO) ;
			 if(SendMessage (hwndChild, BM_GETCHECK, 0, 0))
				  ZapisiRegistryBool(TRUE,"Zvono");
			 else
			 {
				  ZapisiRegistryBool(FALSE,"Zvono");
				  GetDlgItemText(hWndDailog,IDC_EDIT1,buffer,sizeof(buffer));
				  if(_tcslen(buffer)<5){
					  MessageBox(hWndDailog,"Niste unjeli zvuènu datoteku","Sat",MB_OK | MB_ICONSTOP);
					  return TRUE; }
				  ZapisiRegistryString(buffer,"FailZaZvuk");
				  GetDlgItemText(hWndDailog,IDC_EDIT2,buffer,sizeof(buffer));
				  if(!VrijednostSata())//Dali je vrijeme zvonjave pravilno uneseno
				  {
					  MessageBox(NULL,"Vrijeme za zvonjavu je nepravilno uneseno.\nUnesite broj npr. 0244, \
gdje su prve dvije znamenke sat,\na zadnje dvije minute","Sat",MB_OK | MB_ICONSTOP);
					  return TRUE;
				  }
				  ZapisiRegistryString(buffer,"VrijemeZvonjave");
			 }

             EndDialog (hWndDailog, 0) ;
             return TRUE ;
          case IDCANCEL :
             EndDialog (hWndDailog, 0) ;
             return TRUE ;
 
 
     }
	}	
     return FALSE ;
} 

//----------------Funkcija za obradu dogaðaja----------------
LONG WINAPI ScreenSaverProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) 
{
	PAINTSTRUCT ps;

	switch(msg)
	{ 
		case WM_TIMER:
			OnDraw();
			return 0;
		case WM_CREATE:
			::hInstance = ((LPCREATESTRUCT) lParam)->hInstance ;
			::hWnd = hWnd;
			OnCreate();
			return 0;
		case WM_PAINT:
			BeginPaint(hWnd,&ps);
			OnPaint(ps.hdc);
			EndPaint(hWnd,&ps);
			return 0;
		case WM_SIZE:
			visina=HIWORD(lParam)/2;
			sirina=LOWORD(lParam)/2;
	        cxClient = LOWORD (lParam) ;
			cyClient = HIWORD (lParam) ;
			return 0;
 /* case WM_ERASEBKGND:
return 1;*/
		case WM_DESTROY:
			OnDestroy();
			return 0;
	}
	return DefScreenSaverProc(hWnd,msg,wParam,lParam); 
} 
//----------------Registriranj klase prozora SSavera------------- 
RegisterClass(HINSTANCE hInst)
{
 WNDCLASS wc; 
 wc.hCursor        = NULL; 
 wc.hIcon          = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON1));
 wc.lpszMenuName   = NULL; 
 wc.lpszClassName  = "WindowsScreenSaverClass"; 
 wc.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH); 
 wc.hInstance      = hInst; 
 wc.style          = CS_VREDRAW | CS_HREDRAW | CS_SAVEBITS | CS_DBLCLKS | CS_OWNDC;
 wc.lpfnWndProc    = (WNDPROC)ScreenSaverProc; 
 wc.cbWndExtra     = 0; 
 wc.cbClsExtra     = 0;  
 return TRUE;
}
 
