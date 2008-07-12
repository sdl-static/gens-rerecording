#include "gens.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include "G_main.h"
#include "G_ddraw.h"
#include "G_dsound.h"
#include "G_Input.h"
#include "Debug.h"
#include "rom.h"
#include "save.h"
#include "resource.h"
#include "misc.h"
#include "blit.h"
#include "ggenie.h"
#include "Cpu_68k.h"
#include "Star_68k.h"
#include "Cpu_SH2.h"
#include "Cpu_Z80.h"
#include "z80.h"
#include "mem_M68K.h"
#include "mem_S68K.h"
#include "mem_SH2.h"
#include "mem_Z80.h"
#include "io.h"
#include "psg.h"
#include "ym2612.h"
#include "pwm.h"
#include "scrshot.h"
#include "vdp_io.h"
#include "vdp_rend.h"
#include "vdp_32X.h"
#include "LC89510.h"
#include "gfx_cd.h"
#include "cd_aspi.h"
#include "net.h"
#include "pcm.h"
#include "htmlhelp.h"
#include "CCnet.h"
#include "wave.h"
#include "ram_search.h"
#include "movie.h"

extern "C" void Read_To_68K_Space(int adr);
#define MAPHACK
#define uint32 unsigned int

FILE *fp1;

#define STATES 3
unsigned int *rd_mode, *wr_mode, *ppu_mode, *pc_mode;
unsigned int *rd_low, *rd_high;
unsigned int *wr_low, *wr_high;
unsigned int *ppu_low, *ppu_high;
unsigned int *pc_low, *pc_high;
unsigned int *pc_start;

unsigned int *rd_mode_cd, *wr_mode_cd, *ppu_mode_cd, *pc_mode_cd;
unsigned int *rd_low_cd, *rd_high_cd;
unsigned int *wr_low_cd, *wr_high_cd;
unsigned int *ppu_low_cd, *ppu_high_cd;
unsigned int *pc_low_cd, *pc_high_cd;
unsigned int *pc_start_cd;

bool trace_map=0;
bool hook_trace=0;
bool AutoSearch=false;

FILE *fp_hook, *fp_hook_cd;
FILE *fp_trace, *fp_trace_cd;
char *mapped;
char *mapped_cd;

#define WM_KNUX WM_USER + 3
#define GENS_VERSION   2.10
#define GENS_VERSION_H 2 * 65536 + 10

#define MINIMIZE								\
if (Sound_Initialised) Clear_Sound_Buffer();	\
if (Full_Screen)								\
{												\
	Set_Render(hWnd, 0, -1, true);				\
	FS_Minimised = 1;							\
}

#define MENU_L(smenu, pos, flags, id, str, suffixe, def)										\
GetPrivateProfileString(language_name[Language], (str), (def), Str_Tmp, 1024, Language_Path);	\
strcat(Str_Tmp, (suffixe));																			\
InsertMenu((smenu), (pos), (flags), (id), Str_Tmp);

#define WORD_L(id, str, suffixe, def)															\
GetPrivateProfileString(language_name[Language], (str), (def), Str_Tmp, 1024, Language_Path);	\
strcat(Str_Tmp, (suffixe));																			\
SetDlgItemText(hDlg, id, Str_Tmp);

#define MESSAGE_L(str, def, time)																	\
{																									\
	GetPrivateProfileString(language_name[Language], (str), (def), Str_Tmp, 1024, Language_Path);	\
	Put_Info(Str_Tmp, (time));																		\
}

#define MESSAGE_NUM_L(str, def, num, time)															\
{																									\
	char mes_tmp[1024];																				\
	GetPrivateProfileString(language_name[Language], (str), (def), Str_Tmp, 1024, Language_Path);	\
	sprintf(mes_tmp, Str_Tmp, (num));																\
	Put_Info(mes_tmp, (time));																		\
}

HINSTANCE ghInstance;
HACCEL hAccelTable;
WNDCLASS WndClass;
HWND HWnd;
HMENU Gens_Menu;
HWND RamSearchHWnd = NULL; // modeless dialog
HWND RamWatchHWnd = NULL; // modeless dialog
HWND RamCheatHWnd = NULL; // modeless dialog
HWND VolControlHWnd = NULL;

char Str_Tmp[1024];
char Comment[256];
char Gens_Path[1024];
char Language_Path[1024];
char CGOffline_Path[1024];
char Manual_Path[1024];
//FILE *dbgfile=fopen("c:\\gensdbg.txt","r+b");
char **language_name = NULL;
struct Rom *Game = NULL;
int Active = 0;
int Paused = 0;
int Net_Play = 0;
int Full_Screen = -1;
int Resolution = 1;
int Fast_Blur = 0;
int Render_W = 0;
int Setting_Render = 0;
int Render_FS = 0;
int Show_FPS = 0;
int Show_Message = 0;
int Show_LED = 0;
int FS_Minimised = 0;
int Auto_Pause = 0;
int Auto_Fix_CS = 0;
int Language = 0;
int Country = -1;
int Country_Order[3];
int Kaillera_Client_Running = 0;
int Intro_Style = 0;
int SegaCD_Accurate = 1;
int Gens_Running = 0;
int WinNT_Flag = 0;
int Gens_Priority;
int SS_Actived;
int DialogsOpen = 0; //Modif
int SlowDownMode=0; //Modif
unsigned int QuickSaveKey=0;	//Modif
unsigned int QuickLoadKey=0;	//Modif
unsigned int AutoFireKey=0;	//Modif N.
unsigned int AutoHoldKey=0;	//Modif N.
unsigned int AutoClearKey=0;	//Modif N.
int QuickSaveKeyIsPressed=0;	//Modif
int QuickLoadKeyIsPressed=0;	//Modif
unsigned int QuickPauseKey=0;	//Modif
int QuickPauseKeyIsPressed=0;	//Modif
int SlowDownSpeed=1;	//Modif
unsigned int SlowDownKey=0;	//Modif
int SlowDownKeyIsPressed=1;	//Modif
int RecordMovieCanceled=1;//Modif
int PlayMovieCanceled=1; //Modif
int Disable_Blue_Screen=0; //Modif
int Never_Skip_Frame=0; //Modif
unsigned int SkipKey=0; //Modif
int SkipKeyIsPressed=0; //Modif
int FrameCounterEnabled=0; //Modif
int FrameCounterFrames=0; //Modif N.
int LagCounterEnabled=0; //Modif
int LagCounterFrames=0; //Modif N.
int ShowInputEnabled=0; //Modif
int AutoBackupEnabled=0; //Modif
int LeftRightEnabled=0; //Modif
int NumLoadEnabled=0; //Modif N.
int FrameCounterPosition=16*336+32; //Modif
int MustUpdateMenu=0; // Modif
bool RamSearchClosed = false;
bool RamWatchClosed = false;
unsigned char StateSelectCfg = 0;
char rs_c='s';
char rs_o='=';
char rs_t='s';
int rs_param=0, rs_val=0;
bool PaintsEnabled = true;

typeMovie SubMovie;
unsigned long SpliceFrame = 0;
unsigned long SeekFrame = 0;
char *TempName;
char SpliceMovie[1024];
long x = 0, y = 0, xg = 0, yg = 0;

POINT Window_Pos;

LRESULT WINAPI WinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GGenieProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ColorProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DirectoriesProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FilesProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ControllerProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK OptionProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AboutProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PlayMovieProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RecordMovieProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RamSearchProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RamWatchProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RamCheatProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK VolumeProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PromptSpliceFrameProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PromptSeekFrameProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PromptWatchNameProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditWatchProc(HWND, UINT, WPARAM, LPARAM);
void DoMovieSplice();

int Set_Render(HWND hWnd, int Full, int Num, int Force);
HMENU Build_Main_Menu(void);
bool Save_Watches()
{
	strncpy(Str_Tmp,Rom_Name,512);
	strcat(Str_Tmp,".wch");
	if(Change_File_S(Str_Tmp, Gens_Path, "Save Watches", "GENs Watchlist\0*.wch\0All Files\0*.*\0\0", "wch"))
	{
		FILE *WatchFile = fopen(Str_Tmp,"r+b");
		if (!WatchFile) WatchFile = fopen(Str_Tmp,"w+b");
		fputc(SegaCD_Started?'1':(_32X_Started?'2':'0'),WatchFile);
		fputc('\n',WatchFile);
		sprintf(Str_Tmp,"%d\n",WatchCount);
		fputs(Str_Tmp,WatchFile);
		const char DELIM = '\t';
		for (int i = 0; i < WatchCount; i++)
		{
			sprintf(Str_Tmp,"%05X%c%08X%c%c%c%c%c%d%c%s\n",rswatches[i].Index,DELIM,rswatches[i].Address,DELIM,rswatches[i].Size,DELIM,rswatches[i].Type,DELIM,rswatches[i].WrongEndian,DELIM,rsaddrs[rswatches[i].Index].comment);
			fputs(Str_Tmp,WatchFile);
		}
		fclose(WatchFile);
		return true;
	}
	return false;
}
bool Load_Watches()
{
	strncpy(Str_Tmp,Rom_Name,512);
	strcat(Str_Tmp,".wch");
	const char DELIM = '\t';
	if(Change_File_L(Str_Tmp, Gens_Path, "Load Watches", "GENs Watchlist\0*.wch\0All Files\0*.*\0\0", "wch"))
	{
		FILE *WatchFile = fopen(Str_Tmp,"rb");
		if (!WatchFile)
		{
			MessageBox(NULL,"Error opening file.","ERROR",MB_OK);
			return false;
		}
		AddressWatcher Temp;
		char mode;
		fgets(Str_Tmp,1024,WatchFile);
		sscanf(Str_Tmp,"%c%*s",&mode);
		if ((mode == '1' && !(SegaCD_Started)) || (mode == '2' && !(_32X_Started)))
		{
			char Device[8];
			strcpy(Device,(mode > '1')?"32X":"SegaCD");
			sprintf(Str_Tmp,"Warning: %s not started. \nWatches for %s addresses will be ignored.",Device,Device);
			MessageBox(NULL,Str_Tmp,"Device Mismatch",MB_OK);
		}
		int WatchAdd;
		fgets(Str_Tmp,1024,WatchFile);
		sscanf(Str_Tmp,"%d%*s",&WatchAdd);
		WatchAdd+=WatchCount;
		for (int i = WatchCount; i < WatchAdd; i++)
		{
			while (i < 0)
				i++;
			do {
				fgets(Str_Tmp,1024,WatchFile);
			} while (Str_Tmp[0] == '\n');
			sscanf(Str_Tmp,"%05X%*c%08X%*c%c%*c%c%*c%d",&(Temp.Index),&(Temp.Address),&(Temp.Size),&(Temp.Type),&(Temp.WrongEndian));
			Temp.WrongEndian = 0;
			if (SegaCD_Started && (mode != '1'))
				Temp.Index += SEGACD_RAM_SIZE;
			if ((mode == '1') && !SegaCD_Started)
				Temp.Index -= SEGACD_RAM_SIZE;
			if ((unsigned int)Temp.Index > (unsigned int)CUR_RAM_MAX)
			{
				i--;
				WatchAdd--;
			}
			else
			{
				char *Comment = strrchr(Str_Tmp,DELIM) + 1;
				*strrchr(Comment,'\n') = '\0';
				InsertWatch(Temp,Comment);
			}
		}
		fclose(WatchFile);
		if (RamWatchHWnd)
			ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
		return true;
	}
	return false;
}
void ResetWatches()
{
	for (;WatchCount>=0;WatchCount--)
	{
		rsaddrs[rswatches[WatchCount].Index].flags &= ~RS_FLAG_WATCHED;
		free(rsaddrs[rswatches[WatchCount].Index].comment);
		rsaddrs[rswatches[WatchCount].Index].comment = NULL;
	}
	WatchCount++;
	if (RamWatchHWnd)
		ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
}
void ResetResults()
{
	reset_address_info();
	ResultCount = 0;
	if (RamSearchHWnd)
		ListView_SetItemCount(GetDlgItem(RamSearchHWnd,IDC_RAMLIST),ResultCount);
}
void CloseRamWindows() //Close the Ram Search & Watch windows when rom closes
{
	ResetWatches();
	ResetResults();
	if (RamSearchHWnd)
	{
		SendMessage(RamSearchHWnd,WM_CLOSE,NULL,NULL);
		RamSearchClosed = true;
	}
	if (RamWatchHWnd)
	{
		SendMessage(RamWatchHWnd,WM_CLOSE,NULL,NULL);
		RamWatchClosed = true;
	}
}
void ReopenRamWindows() //Reopen them when a new Rom is loaded
{
	if (RamSearchClosed)
	{
		RamSearchClosed = false;
		if(!RamSearchHWnd)
		{
			reset_address_info();
			RamSearchHWnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_RAMSEARCH), HWnd, (DLGPROC) RamSearchProc);
			DialogsOpen++;
		}
	}
	if (RamWatchClosed)
	{
		RamWatchClosed = false;
		if(!RamWatchHWnd)
		{
			RamWatchHWnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_RAMWATCH), HWnd, (DLGPROC) RamWatchProc);
			DialogsOpen++;
		}
	}
}
void RemoveWatch(int watchIndex)
{
	free(rsaddrs[rswatches[watchIndex].Index].comment);
	rsaddrs[rswatches[watchIndex].Index].comment = NULL;
	for (int i = watchIndex; i <= WatchCount; i++)
		rswatches[i] = rswatches[i+1];
	rsaddrs[rswatches[WatchCount].Index].flags &= ~RS_FLAG_WATCHED;
	WatchCount--;
}
int Change_VSync(HWND hWnd)
{
	int *p_vsync;
	
	if (Full_Screen)
	{
		End_DDraw();
		p_vsync = &FS_VSync;
	}
	else p_vsync = &W_VSync;
	
	*p_vsync = 1 - *p_vsync;
	
	if (*p_vsync) MESSAGE_L("Vertical Sync Enabled", "Vertical Sync Enabled", 1000)
	else MESSAGE_L("Vertical Sync Disabled", "Vertical Sync Disabled", 1000)

	Build_Main_Menu();
	if (Full_Screen) return Init_DDraw(HWnd);
	else return 1;
}


int Set_Frame_Skip(HWND hWnd, int Num)
{
	Frame_Skip = Num;

	if (Frame_Skip != -1)
		MESSAGE_NUM_L("Frame skip set to %d", "Frame skip set to %d", Frame_Skip, 1500)
	else
		MESSAGE_L("Frame skip set to Auto", "Frame skip set to Auto", 1500)
	if (SeekFrame)
	{
		MESSAGE_L("Seek Cancelled","Seek Cancelled",1500);
	}
	SeekFrame = 0;
	Build_Main_Menu();
	return(1);
}


int Set_Current_State(HWND hWnd, int Num)
{
	FILE *f;
	
	Current_State = Num;

	if (f = Get_State_File())
	{
		fclose(f);
		MESSAGE_NUM_L("SLOT %d [OCCUPIED]", "SLOT %d [OCCUPIED]", Current_State, 1500)
	}
	else
	{
		MESSAGE_NUM_L("SLOT %d [EMPTY]", "SLOT %d [EMPTY]", Current_State, 1500)
	}

	Build_Main_Menu();
	return 1;
}


int Change_Stretch(void)
{
	if (Full_Screen && !FS_No_Res_Change && (Render_FS > 1)) return(0);
	
	Flag_Clr_Scr = 1;

	if (Stretch = (1 - Stretch))
		MESSAGE_L("Stretched mode", "Stretched mode", 1000)
	else
		MESSAGE_L("Correct ratio mode", "Correct ratio mode", 1000)

	Build_Main_Menu();
	return(1);
}


int Change_Blit_Style(void)
{
	if ((!Full_Screen) || (Render_FS > 1)) return(0);

	Flag_Clr_Scr = 1;

	if (Blit_Soft = (1 - Blit_Soft))
		MESSAGE_L("Force software blit for Full-Screen", "Force software blit for Full-Screen", 1000)
	else
		MESSAGE_L("Enable hardware blit for Full-Screen", "Enable hardware blit for Full-Screen", 1000)

	return(1);
}


int Set_Sprite_Over(HWND hWnd, int Num)
{
	if (Sprite_Over = Num)
		MESSAGE_L("Sprite Limit Enabled", "Sprite Limit Enabled", 1000)
	else
		MESSAGE_L("Sprite Limit Disabled", "Sprite Limit Disabled", 1000)

	Build_Main_Menu();
	return(1);
}


int Change_Debug(HWND hWnd, int Debug_Mode)
{
	if (!Game) return 0;
		
	Flag_Clr_Scr = 1;
	Clear_Sound_Buffer();

	if (Debug_Mode == Debug) Debug = 0;
	else Debug = Debug_Mode;
	
	Build_Main_Menu();
	return 1;
}


int Change_Fast_Blur(HWND hWnd)
{
	Flag_Clr_Scr = 1;

	if (Fast_Blur = (1 - Fast_Blur))
		MESSAGE_L("Fast Blur Enabled", "Fast Blur Enabled", 1000)
	else
		MESSAGE_L("Fast Blur Disabled", "Fast Blur Disabled", 1000)

	Build_Main_Menu();
	return(1);
}

int Change_Layer(HWND hWnd, int Num) //Nitsuja added this to allow for layer enabling and disabling.
{
	char* layer;
	switch(Num)
	{
	case 0: layer = &VScrollAl; break;
	case 1: layer = &VScrollBl; break;
	case 2: layer = &VScrollAh; break;
	case 3: layer = &VScrollBh; break;
	case 4: layer = &VSpritel; break;
	case 5: layer = &VSpriteh; break;
	default: return 1;
	}

	*layer = !*layer;

	char message [256];
	sprintf(message, "Layer %d %sabled", Num+1, *layer?"en":"dis");
	MESSAGE_L(message, message, 1000)

	Build_Main_Menu();
	return(1);
}

int Change_SpriteTop (HWND hWnd)
{
	Sprite_Always_Top = !Sprite_Always_Top;

	char message [256];
	sprintf(message, "Always Show Sprites %sabled", Sprite_Always_Top?"en":"dis");
	MESSAGE_L(message, message, 1000)

	Build_Main_Menu();
	return (1);
}


int Change_LayerSwap (HWND hWnd, int num)
{
	char *Plane;
	switch (num)
	{
		case 0:
			Plane = &Swap_Scroll_PriorityA;
			break;
		case 1:
			Plane = &Swap_Scroll_PriorityB;
			break;
		case 2:
			Plane = &Swap_Sprite_Priority;
			break;
		default:
			return 1;
	}
	*Plane = !(*Plane);

	char message [256];
	sprintf(message, "Layer Swapping %sabled", *Plane?"en":"dis");
	MESSAGE_L(message, message, 1000)

	Build_Main_Menu();
	return (1);
}

int Change_Plane (HWND hWnd, int num)
{
	char *Plane;
	switch (num)
	{
		case 0:
			Plane = &ScrollAOn;
			break;
		case 1:
			Plane = &ScrollBOn;
			break;
		case 2:
			Plane = &SpriteOn;
			break;
		default:
			return 1;
	}
	*Plane = !(*Plane);

	char message [256];
	sprintf(message, "Plane %sabled", *Plane?"en":"dis");
	MESSAGE_L(message, message, 1000)

	Build_Main_Menu();
	return (1);
}

typedef void (*BlitFunc)(unsigned char*, int, int, int, int);

void Set_Rend_Int(int Num, int* Rend, BlitFunc* Blit)
{
	bool quiet = false;
	if(Num == -1)
	{
		Num = *Rend;
		quiet = true;
	}
	else
	{
		*Rend = Num;
	}

	switch(Num)
	{
		case 0:
			if (Have_MMX) *Blit = Blit_X1_MMX;
			else *Blit = Blit_X1;
			break;

		case 1:
			if (Have_MMX) *Blit = Blit_X2_MMX;
			else *Blit = Blit_X2;
			break;

		case 2:
			*Blit = CBlit_EPX;
			break;

		case 3:
			if (Bits32) *Blit = CBlit_X2_Int;
			else if (Have_MMX) *Blit = Blit_X2_Int_MMX;
			else *Blit = Blit_X2_Int;
			break;

		case 4:
			if (Bits32) *Blit = CBlit_Scanline;
			else if (Have_MMX) *Blit = Blit_Scanline_MMX;
			else *Blit = Blit_Scanline;
			break;

		case 5:
			if (!Bits32 && Have_MMX) *Blit = Blit_Scanline_50_MMX;
			else *Blit = CBlit_Scanline_50;
			break;

		case 6:
			if (!Bits32 && Have_MMX) *Blit = Blit_Scanline_25_MMX;
			else *Blit = CBlit_Scanline_25;
			break;

		case 7:
			if (Bits32) *Blit = CBlit_Scanline_Int;
			else if (Have_MMX) *Blit = Blit_Scanline_Int_MMX;
			else *Blit = Blit_Scanline_Int;
			break;

		case 8:
			if (!Bits32 && Have_MMX) *Blit = Blit_Scanline_50_Int_MMX;
			else *Blit = CBlit_Scanline_50_Int;
			break;

		case 9:
			if (!Bits32 && Have_MMX) *Blit = Blit_Scanline_25_Int_MMX;
			else *Blit = CBlit_Scanline_25_Int;
			break;

		case 10:
			if (Have_MMX) *Blit = Blit_2xSAI_MMX;
			else
			{
				*Rend = 7;
				*Blit = Blit_Scanline_Int;
			}
			break;

		default:
			*Rend = 1;
			if (Have_MMX) *Blit = Blit_X2_MMX;
			else *Blit = Blit_X2;
			break;
	}

	if(!quiet)
	{
		switch(*Rend)
		{
		case 0: MESSAGE_L("Render selected : NORMAL", "Render selected : NORMAL", 1500); break;
		case 1: MESSAGE_L("Render selected : DOUBLE", "Render selected : DOUBLE", 1500); break;
		case 2: MESSAGE_L("Render selected : EPX 2X SCALE", "Render selected : EPX 2X SCALE", 1500); break;
		case 3: MESSAGE_L("Render selected : INTERPOLATED", "Render selected : INTERPOLATED", 1500); break;
		case 4: MESSAGE_L("Render selected : FULL SCANLINE", "Render selected : FULL SCANLINE", 1500); break;
		case 5: MESSAGE_L("Render selected : 50% SCANLINE", "Render selected : 50% SCANLINE", 1500); break;
		case 6: MESSAGE_L("Render selected : 25% SCANLINE", "Render selected : 25% SCANLINE", 1500); break;
		case 7: MESSAGE_L("Render selected : INTERPOLATED SCANLINE", "Render selected : INTERPOLATED SCANLINE", 1500); break;
		case 8: MESSAGE_L("Render selected : INTERPOLATED 50% SCANLINE", "Render selected : INTERPOLATED 50% SCANLINE", 1500); break;
		case 9: MESSAGE_L("Render selected : INTERPOLATED 25% SCANLINE", "Render selected : INTERPOLATED 25% SCANLINE", 1500); break;
		case 10: MESSAGE_L("Render selected : 2XSAI KREED'S ENGINE", "Render selected : 2XSAI KREED'S ENGINE", 1500); break;
		default: MESSAGE_L("Render selected : ??????", "Render selected : ??????", 1500); break;
		}
	}
}

int Set_Render(HWND hWnd, int Full, int Num, int Force)
{
	Setting_Render = TRUE;

	int Old_Rend, *Rend;
	BlitFunc* Blit;
	
	if (Full)
	{
		Rend = &Render_FS; // Render_FS = ...
		Blit = &Blit_FS; // Blit_FS = ...
	}
	else
	{
		Rend = &Render_W; // Render_W = ...
		Blit = &Blit_W; // Blit_W = ...
	}

	Old_Rend = *Rend;
	Flag_Clr_Scr = 1;

	bool reinit = false;
	if(Full != Full_Screen || (Num != -1 && (Num<2 || Old_Rend<2)) || Force)
		reinit = true;
	else if(Bits32 && Num == 10) // note: this is in the else statement because Bits32 is only valid to check here if reinit is false
		Num = 2; // we don't support 2xSaI in 32-bit mode

	Set_Rend_Int(Num, Rend, Blit);

	if(reinit)
	{
		RECT r;

		if (Sound_Initialised) Clear_Sound_Buffer();

		End_DDraw();

		if (Full_Screen = Full)
		{
			while (ShowCursor(true) < 1);
			while (ShowCursor(false) >= 0);

			SetWindowPos(hWnd, NULL, 0, 0, 320 * ((*Rend == 0)?1:2), 240 * ((*Rend == 0)?1:2), SWP_NOZORDER | SWP_NOACTIVATE);
			SetWindowLong(hWnd, GWL_STYLE, NULL);
		}
		else
		{

//			memset(&dm, 0, sizeof(DEVMODE));
//			dm.dmSize = sizeof(DEVMODE);
//			dm.dmBitsPerPel = 16;
//			dm.dmFields = DM_BITSPERPEL;

//			ChangeDisplaySettings(&dm, 0);

			while (ShowCursor(false) >= 0);
			while (ShowCursor(true) < 1);

			// MoveWindow / ResizeWindow code
			SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | WS_OVERLAPPEDWINDOW);
			SetRect(&r, 0, 0, 320 * ((*Rend == 0)?1:2), 240 * ((*Rend == 0)?1:2));
			AdjustWindowRectEx(&r, GetWindowLong(hWnd, GWL_STYLE), 1, GetWindowLong(hWnd, GWL_EXSTYLE));
			SetWindowPos(hWnd, NULL, Window_Pos.x, Window_Pos.y, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		DEVMODE dm;
		EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&dm);
		Bits32 = ((dm.dmBitsPerPel > 16) ? 1: 0);
	
		Build_Main_Menu();
		const int retval = Init_DDraw(HWnd);
		Setting_Render = FALSE;
		return retval;
	}

	InvalidateRect(hWnd, NULL, FALSE);
	Build_Main_Menu();
	Setting_Render = FALSE;
	return 1;
}


int Change_SegaCD_Synchro(void)
{
	if (SegaCD_Accurate)
	{
		SegaCD_Accurate = 0;

		if (SegaCD_Started)
		{
			Update_Frame = Do_SegaCD_Frame;
			Update_Frame_Fast = Do_SegaCD_Frame_No_VDP;
		}

		MESSAGE_L("SegaCD normal mode", "SegaCD normal mode", 1500)
	}
	else
	{
		SegaCD_Accurate = 1;

		if (SegaCD_Started)
		{
			Update_Frame = Do_SegaCD_Frame_Cycle_Accurate;
			Update_Frame_Fast = Do_SegaCD_Frame_No_VDP_Cycle_Accurate;
		}

		MESSAGE_L("SegaCD perfect synchro mode (SLOW)", "SegaCD perfect synchro mode (slower)", 1500)
	}

	Build_Main_Menu();
	return 1;
}


int Change_SegaCD_SRAM_Size(int num)
{
	if (num == -1)
	{
		BRAM_Ex_State &= 1;
		MESSAGE_L("SegaCD SRAM cart removed", "SegaCD SRAM cart removed", 1500)
	}
	else
	{
		char bsize[256];
	
		BRAM_Ex_State |= 0x100;
		BRAM_Ex_Size = num;

		sprintf(bsize, "SegaCD SRAM cart plugged (%d Kb)", 8 << num);
		MESSAGE_L(bsize, bsize, 1500)
	}

	Build_Main_Menu();
	return 1;
}


int Change_Z80(HWND hWnd)
{
	if (Z80_State & 1)
	{
		Z80_State &= ~1;
		MESSAGE_L("Z80 Disabled", "Z80 Disabled", 1000)
	}
	else
	{
		Z80_State |= 1;
		MESSAGE_L("Z80 Enabled", "Z80 Enabled", 1000)
	}

	Build_Main_Menu();
	return(1);
}


int Change_DAC(HWND hWnd)
{
	if (DAC_Enable)
	{
		DAC_Enable = 0;
		MESSAGE_L("DAC Disabled", "DAC Disabled", 1000)
	}
	else
	{
		DAC_Enable = 1;
		MESSAGE_L("DAC Enabled", "DAC Enabled", 1000)
	}

	Build_Main_Menu();
	return(1);
}


int Change_DAC_Improv(HWND hWnd)
{
	if (DAC_Improv)
	{
		DAC_Improv = 0;
		MESSAGE_L("Normal DAC sound", "Normal DAC sound", 1000)
	}
	else
	{
		DAC_Improv = 1;
		MESSAGE_L("High Quality DAC sound", "High Quality DAC sound", 1000) //Nitsuja modified this
	}

	Build_Main_Menu(); //Nitsuja added this line
	return(1);
}


int Change_YM2612(HWND hWnd)
{
	if (YM2612_Enable)
	{
		YM2612_Enable = 0;
		MESSAGE_L("YM2612 Disabled", "YM2612 Disabled", 1000)
	}
	else
	{
		YM2612_Enable = 1;
		MESSAGE_L("YM2612 Enabled", "YM2612 Enabled", 1000)
	}

	Build_Main_Menu();
	return(1);
}


int Change_YM2612_Improv(HWND hWnd)
{
	unsigned char Reg_1[0x200];

	if (YM2612_Improv)
	{
		YM2612_Improv = 0;
		MESSAGE_L("Normal YM2612 emulation", "Normal YM2612 emulation", 1000)
	}
	else
	{
		YM2612_Improv = 1;
		MESSAGE_L("High Quality YM2612 emulation", "High Quality YM2612 emulation", 1000)
	}

	YM2612_Save(Reg_1);

	if (CPU_Mode)
	{
		YM2612_Init(CLOCK_PAL / 7, Sound_Rate, YM2612_Improv);
	}
	else
	{
		YM2612_Init(CLOCK_NTSC / 7, Sound_Rate, YM2612_Improv);
	}

	YM2612_Restore(Reg_1);

	Build_Main_Menu();
	return 1;
}


int Change_PSG(HWND hWnd)
{
	if (PSG_Enable)
	{
		PSG_Enable = 0;
		MESSAGE_L("PSG Disabled", "PSG Disabled", 1000)
	}
	else
	{
		PSG_Enable = 1;
		MESSAGE_L("PSG Enabled", "PSG Enabled", 1000)
	}

	Build_Main_Menu();
	return 1;
}


int Change_PSG_Improv(HWND hWnd)
{
	if (PSG_Improv)
	{
		PSG_Improv = 0;
		MESSAGE_L("Normal PSG sound", "Normal PSG sound", 1000)
	}
	else
	{
		PSG_Improv = 1;
		MESSAGE_L("High Quality PSG sound", "High Quality PSG sound", 1000) //Nitsuja modified this
	}

	Build_Main_Menu(); //Nitsuja added this line
	return 1;
}


int Change_PCM(HWND hWnd)
{
	if (PCM_Enable)
	{
		PCM_Enable = 0;
		MESSAGE_L("PCM Sound Disabled", "PCM Sound Disabled", 1000)
	}
	else
	{
		PCM_Enable = 1;
		MESSAGE_L("PCM Sound Enabled", "PCM Sound Enabled", 1000)
	}

	Build_Main_Menu();
	return 1;
}


int Change_PWM(HWND hWnd)
{
	if (PWM_Enable)
	{
		PWM_Enable = 0;
		MESSAGE_L("PWM Sound Disabled", "PWM Sound Disabled", 1000)
	}
	else
	{
		PWM_Enable = 1;
		MESSAGE_L("PWM Sound Enabled", "PWM Sound Enabled", 1000)
	}

	Build_Main_Menu();
	return 1;
}


int Change_CDDA(HWND hWnd)
{
	if (CDDA_Enable)
	{
		CDDA_Enable = 0;
		MESSAGE_L("CD Audio Sound Disabled", "CD Audio Sound Disabled", 1000)
	}
	else
	{
		CDDA_Enable = 1;
		MESSAGE_L("CD Audio Enabled", "CD Audio Enabled", 1000)
	}

	Build_Main_Menu();
	return(1);
}


int	Change_Sound(HWND hWnd)
{
	if (Sound_Enable)
	{
		End_Sound();

		Sound_Enable = 0;
		YM2612_Enable = 0;
		PSG_Enable = 0;
		DAC_Enable = 0;
		PCM_Enable = 0;
		PWM_Enable = 0;
		CDDA_Enable = 0;

		MESSAGE_L("Sound Disabled", "Sound Disabled", 1500)
	}
	else
	{
		if (!Init_Sound(hWnd))
		{
			Sound_Enable = 0;
			YM2612_Enable = 0;
			PSG_Enable = 0;
			DAC_Enable = 0;
			PCM_Enable = 0;
			PWM_Enable = 0;
			CDDA_Enable = 0;

			return 0;
		}

		Sound_Enable = 1;
		Play_Sound();

		if (!(Z80_State & 1)) Change_Z80(hWnd);

		YM2612_Enable = 1;
		PSG_Enable = 1;
		DAC_Enable = 1;
		PCM_Enable = 1;
		PWM_Enable = 1;
		CDDA_Enable = 1;

		MESSAGE_L("Sound Enabled", "Sound Enabled", 1500)
	}

	Build_Main_Menu();
	return 1;
}

int Change_Sample_Rate(HWND hWnd, int Rate)
{
	unsigned char Reg_1[0x200];

	switch (Rate)
	{
	case 0:
		Sound_Rate = 11025;
		MESSAGE_L("Sound rate set to 11025", "Sound rate set to 11025", 2500)
		break;

	case 1:
		Sound_Rate = 22050;
		MESSAGE_L("Sound rate set to 22050", "Sound rate set to 22050", 2500)
		break;

	case 2:
		Sound_Rate = 44100;
		MESSAGE_L("Sound rate set to 44100", "Sound rate set to 44100", 2500)
		break;
	}

	if (Sound_Enable)
	{
		PSG_Save_State();
		YM2612_Save(Reg_1);

		End_Sound();
		Sound_Enable = 0;

		if (CPU_Mode)
		{
			YM2612_Init(CLOCK_PAL / 7, Sound_Rate, YM2612_Improv);
			PSG_Init(CLOCK_PAL / 15, Sound_Rate);
		}
		else
		{
			YM2612_Init(CLOCK_NTSC / 7, Sound_Rate, YM2612_Improv);
			PSG_Init(CLOCK_NTSC / 15, Sound_Rate);
		}

		if (SegaCD_Started) Set_Rate_PCM(Sound_Rate);
		YM2612_Restore(Reg_1);
		PSG_Restore_State();
		
		if(!Init_Sound(hWnd)) return(0);

		Sound_Enable = 1;
		Play_Sound();
	}
	
	Build_Main_Menu();
	return(1);
}


int Change_Sound_Stereo(HWND hWnd)
{
	unsigned char Reg_1[0x200];

	if (Sound_Stereo)
	{
		Sound_Stereo = 0;
		MESSAGE_L("Mono sound", "Mono sound", 1000)
	}
	else
	{
		Sound_Stereo = 1;
		MESSAGE_L("Stereo sound", "Stereo sound", 1000)
	}

	if (Sound_Enable)
	{
		PSG_Save_State();
		YM2612_Save(Reg_1);

		End_Sound();
		Sound_Enable = 0;

		if (CPU_Mode)
		{
			YM2612_Init(CLOCK_PAL / 7, Sound_Rate, YM2612_Improv);
			PSG_Init(CLOCK_PAL / 15, Sound_Rate);
		}
		else
		{
			YM2612_Init(CLOCK_NTSC / 7, Sound_Rate, YM2612_Improv);
			PSG_Init(CLOCK_NTSC / 15, Sound_Rate);
		}

		if (SegaCD_Started) Set_Rate_PCM(Sound_Rate);
		YM2612_Restore(Reg_1);
		PSG_Restore_State();
		
		if(!Init_Sound(hWnd)) return(0);

		Sound_Enable = 1;
		Play_Sound();
	}

	Build_Main_Menu();
	return(1);
}

// Modif N.
int Change_Sound_Soften(HWND hWnd)
{
	if (Sound_Soften)
	{
		Sound_Soften = 0;
		MESSAGE_L("Low pass filter off", "Low pass filter off", 1000)
	}
	else
	{
		Sound_Soften = 1;

		if (Sound_Rate == 44100)
		{
			MESSAGE_L("Low pass filter on", "Low pass filter on", 1000)
		}
		else
		{
			Change_Sample_Rate(hWnd, 2);
			MESSAGE_L("Low pass filter on and rate changed to 44100", "Low pass filter on and rate changed to 44100", 1000)
		}
	}

	Build_Main_Menu();
	return(1);
}

// Modif N.
int Change_Sound_Hog(HWND hWnd)
{
	if (Sleep_Time)
	{
		Sleep_Time = 0;
		MESSAGE_L("Maximum CPU usage", "Maximum CPU usage", 1000)
	}
	else
	{
		Sleep_Time = 5;
		MESSAGE_L("Balanced CPU usage", "Balanced CPU usage", 1000)
	}

	Build_Main_Menu();
	return(1);
}

int Change_Country(HWND hWnd, int Num)
{
	unsigned char Reg_1[0x200];

	Flag_Clr_Scr = 1;

	switch(Country = Num)
	{
		default:
		case -1:
			if (Genesis_Started || _32X_Started) Detect_Country_Genesis();
			else if (SegaCD_Started) Detect_Country_SegaCD();
			break;

		case 0:
			Game_Mode = 0;
			CPU_Mode = 0;
			break;

		case 1:
			Game_Mode = 1;
			CPU_Mode = 0;
			break;

		case 2:
			Game_Mode = 1;
			CPU_Mode = 1;
			break;

		case 3:
			Game_Mode = 0;
			CPU_Mode = 1;
			break;
	}

	if (CPU_Mode)
	{
		CPL_Z80 = Round_Double((((double) CLOCK_PAL / 15.0) / 50.0) / 312.0);
		CPL_M68K = Round_Double((((double) CLOCK_PAL / 7.0) / 50.0) / 312.0);
		CPL_MSH2 = Round_Double(((((((double) CLOCK_PAL / 7.0) * 3.0) / 50.0) / 312.0) * (double) MSH2_Speed) / 100.0);
		CPL_SSH2 = Round_Double(((((((double) CLOCK_PAL / 7.0) * 3.0) / 50.0) / 312.0) * (double) SSH2_Speed) / 100.0);

		VDP_Num_Lines = 312;
		VDP_Status |= 0x0001;
		_32X_VDP.Mode &= ~0x8000;

		CD_Access_Timer = 2080;
		Timer_Step = 136752;
	}
	else
	{
		CPL_Z80 = Round_Double((((double) CLOCK_NTSC / 15.0) / 60.0) / 262.0);
		CPL_M68K = Round_Double((((double) CLOCK_NTSC / 7.0) / 60.0) / 262.0);
		CPL_MSH2 = Round_Double(((((((double) CLOCK_NTSC / 7.0) * 3.0) / 60.0) / 262.0) * (double) MSH2_Speed) / 100.0);
		CPL_SSH2 = Round_Double(((((((double) CLOCK_NTSC / 7.0) * 3.0) / 60.0) / 262.0) * (double) SSH2_Speed) / 100.0);

		VDP_Num_Lines = 262;
		VDP_Status &= 0xFFFE;
		_32X_VDP.Mode |= 0x8000;

		CD_Access_Timer = 2096;
		Timer_Step = 135708;
	}

	if (Sound_Enable)
	{
		PSG_Save_State();
		YM2612_Save(Reg_1);

		End_Sound();
		Sound_Enable = 0;

		if (CPU_Mode)
		{
			YM2612_Init(CLOCK_PAL / 7, Sound_Rate, YM2612_Improv);
			PSG_Init(CLOCK_PAL / 15, Sound_Rate);
		}
		else
		{
			YM2612_Init(CLOCK_NTSC / 7, Sound_Rate, YM2612_Improv);
			PSG_Init(CLOCK_NTSC / 15, Sound_Rate);
		}

		if (SegaCD_Started) Set_Rate_PCM(Sound_Rate);
		YM2612_Restore(Reg_1);
		PSG_Restore_State();
		
		if(!Init_Sound(hWnd)) return(0);

		Sound_Enable = 1;
		Play_Sound();
	}

	if (Game_Mode)
	{
		if (CPU_Mode) MESSAGE_L("Europe system (50 FPS)", "Europe system (50 FPS)", 1500)
		else MESSAGE_L("USA system (60 FPS)", "USA system (60 FPS)", 1500)
	}
	else
	{
		if (CPU_Mode) MESSAGE_L("Japan system (50 FPS)", "Japan system (50 FPS)", 1500)
		else MESSAGE_L("Japan system (60 FPS)", "Japan system (60 FPS)", 1500)
	}

	if (Genesis_Started)
	{
		if ((CPU_Mode == 1) || (Game_Mode == 0))
			sprintf(Str_Tmp, "Gens - Megadrive : %s", Game->Rom_Name_W);
		else
			sprintf(Str_Tmp, "Gens - Genesis : %s", Game->Rom_Name_W);
	}
	else if (_32X_Started)
	{
		if (CPU_Mode == 1)
			sprintf(Str_Tmp, "Gens - 32X (PAL) : %s", Game->Rom_Name_W);
		else
			sprintf(Str_Tmp, "Gens - 32X (NTSC) : %s", Game->Rom_Name_W);
	}
	else if (SegaCD_Started)
	{
		if ((CPU_Mode == 1) || (Game_Mode == 0))
			sprintf(Str_Tmp, "Gens - MegaCD : %s", Rom_Name);
		else
			sprintf(Str_Tmp, "Gens - SegaCD : %s", Rom_Name);
	}

	if(Genesis_Started || _32X_Started || SegaCD_Started)
	{
		// Modif N. - remove double-spaces from title bar
		for(int i = 0 ; i < (int)strlen(Str_Tmp)-1 ; i++)
			if(Str_Tmp[i] == Str_Tmp[i+1] && Str_Tmp[i] == ' ')
				strcpy(Str_Tmp+i, Str_Tmp+i+1), i--;

		SetWindowText(HWnd, Str_Tmp);
	}

	Build_Main_Menu();
	return 1;
}


int Change_Country_Order(int Num)
{
	char c_str[4][4] = {"USA", "JAP", "EUR"};
	char str_w[128];
	int sav = Country_Order[Num];
		
	if (Num == 1) Country_Order[1] = Country_Order[0];
	else if (Num == 2)
	{
		Country_Order[2] = Country_Order[1];
		Country_Order[1] = Country_Order[0];
	}
	Country_Order[0] = sav;

	if (Country == -1) Change_Country(HWnd, -1);		// Update Country

	wsprintf(str_w, "Country detec.order : %s %s %s", c_str[Country_Order[0]], c_str[Country_Order[1]], c_str[Country_Order[2]]);
	MESSAGE_L(str_w, str_w, 1500)

	Build_Main_Menu();
	return(1);
}


int Check_If_Kaillera_Running(void)
{
	if (Kaillera_Client_Running)
	{
		if (Sound_Initialised) Clear_Sound_Buffer();
		MessageBox(HWnd, "You can't do it during netplay, you have to close rom and kaillera client before", "info", MB_OK);
		return 1;
	}

	return 0;
}


int WINAPI Play_Net_Game(char *game, int player, int maxplayers)
{
	MSG msg;
	char name[2048];
	HANDLE f;
	WIN32_FIND_DATA fd;

	SetCurrentDirectory(Rom_Dir);

	sprintf(name, "%s.*", game);
	memset(&fd, 0, sizeof(fd));
	fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	f = FindFirstFile(name, &fd);

	if (f == INVALID_HANDLE_VALUE) return 1;

	if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		sprintf(name, "%s%s", Rom_Dir, fd.cFileName);
		Pre_Load_Rom(HWnd, name);
	}

	if ((!Genesis_Started) && (!_32X_Started)) return 1;
	
	Net_Play = 1;
	SetFocus(HWnd);

	if (maxplayers > 4) maxplayers = 4;
	if (player > 4) player = 0;

	Controller_1_Type &= 0xF;
	Controller_2_Type &= 0xF;
	if (maxplayers > 2) Controller_1_Type |= 0x10;
	Make_IO_Table();

	Kaillera_Keys[0] = Kaillera_Keys[1] = Kaillera_Keys[2] = Kaillera_Keys[3] = 0xFF;
	Kaillera_Keys[4] = Kaillera_Keys[5] = Kaillera_Keys[6] = Kaillera_Keys[7] = 0xFF;
	Kaillera_Keys[8] = Kaillera_Keys[9] = 0xFF;
	Kaillera_Error = 0;

	while (Net_Play)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0)) return msg.wParam;
			if (!RamSearchHWnd || !IsDialogMessage(RamSearchHWnd, &msg)) 
			if (!TranslateAccelerator (HWnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg);
			}
		}
		else if ((Active) && (!Paused))
		{
			Update_Emulation_Netplay(HWnd, player, maxplayers);
		}
		else
		{
			Flip(HWnd);
			Sleep(100);
		}
	}

	Kaillera_End_Game();

	return 0;
}


int Start_Netplay(void)
{
	kailleraInfos K_Infos;
	char name[2048];
	char *Liste_Games = NULL, *LG = NULL, *Min_Game, *Cur_Game;
	int cursize = 8192, num = 0, dep = 0, len;
	int backup_infos[32];
	HANDLE f;
	WIN32_FIND_DATA fd;

	if (Kaillera_Initialised == 0)
	{
		MessageBox(HWnd, "You need the KAILLERACLIENT.DLL file to enable this feature", "Info", MB_OK);
		return 0;
	}
		
	if (Kaillera_Client_Running) return 0;

	SetCurrentDirectory(Rom_Dir);

	Liste_Games = (char *) malloc(cursize);
	Liste_Games[0] = 0;
	Liste_Games[1] = 0;

	memset(&fd, 0, sizeof(fd));
	fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	f = FindFirstFile("*.bin", &fd);
	if (f != INVALID_HANDLE_VALUE)
	{
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			len = strlen(fd.cFileName) - 4;
			fd.cFileName[len++] = 0;
			if ((dep + len) > cursize)
			{
				cursize += 8192;
				Liste_Games = (char*) realloc(Liste_Games, cursize);
			}
			strcpy(Liste_Games + dep, fd.cFileName);
			dep += len;
			num++;
		}

		while (FindNextFile(f, &fd))
		{
			if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			{
				len = strlen(fd.cFileName) - 4;
				fd.cFileName[len++] = 0;
				if ((dep + len) > cursize)
				{
					cursize += 8192;
					Liste_Games = (char*) realloc(Liste_Games, cursize);
				}
				strcpy(Liste_Games + dep, fd.cFileName);
				dep += len;
				num++;
			}
		}
	}

	memset(&fd, 0, sizeof(fd));
	fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	f = FindFirstFile("*.smd", &fd);
	if (f != INVALID_HANDLE_VALUE)
	{
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			len = strlen(fd.cFileName) - 4;
			fd.cFileName[len++] = 0;
			if ((dep + len) > cursize)
			{
				cursize += 8192;
				Liste_Games = (char*) realloc(Liste_Games, cursize);
			}
			strcpy(Liste_Games + dep, fd.cFileName);
			dep += len;
			num++;
		}

		while (FindNextFile(f, &fd))
		{
			if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			{
				len = strlen(fd.cFileName) - 4;
				fd.cFileName[len++] = 0;
				if ((dep + len) > cursize)
				{
					cursize += 8192;
					Liste_Games = (char*) realloc(Liste_Games, cursize);
				}
				strcpy(Liste_Games + dep, fd.cFileName);
				dep += len;
				num++;
			}
		}
	}

	memset(&fd, 0, sizeof(fd));
	fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	f = FindFirstFile("*.32X", &fd);
	if (f != INVALID_HANDLE_VALUE)
	{
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			len = strlen(fd.cFileName) - 4;
			fd.cFileName[len++] = 0;
			if ((dep + len) > cursize)
			{
				cursize += 8192;
				Liste_Games = (char*) realloc(Liste_Games, cursize);
			}
			strcpy(Liste_Games + dep, fd.cFileName);
			dep += len;
			num++;
		}

		while (FindNextFile(f, &fd))
		{
			if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			{
				len = strlen(fd.cFileName) - 4;
				fd.cFileName[len++] = 0;
				if ((dep + len) > cursize)
				{
					cursize += 8192;
					Liste_Games = (char*) realloc(Liste_Games, cursize);
				}
				strcpy(Liste_Games + dep, fd.cFileName);
				dep += len;
				num++;
			}
		}
	}

	memset(&fd, 0, sizeof(fd));
	fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	f = FindFirstFile("*.zip", &fd);
	if (f != INVALID_HANDLE_VALUE)
	{
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			len = strlen(fd.cFileName) - 4;
			fd.cFileName[len++] = 0;
			if ((dep + len) > cursize)
			{
				cursize += 8192;
				Liste_Games = (char*) realloc(Liste_Games, cursize);
			}
			strcpy(Liste_Games + dep, fd.cFileName);
			dep += len;
			num++;
		}

		while (FindNextFile(f, &fd))
		{
			if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			{
				len = strlen(fd.cFileName) - 4;
				fd.cFileName[len++] = 0;
				if ((dep + len) > cursize)
				{
					cursize += 8192;
					Liste_Games = (char*) realloc(Liste_Games, cursize);
				}
				strcpy(Liste_Games + dep, fd.cFileName);
				dep += len;
				num++;
			}
		}
	}

	Liste_Games[dep] = 0;
	LG = (char*) malloc(dep);
	dep = 0;
	
	for(; num > 0; num--)
	{
		Min_Game = Cur_Game = Liste_Games;

		while(*Cur_Game)
		{
			if (stricmp(Cur_Game, Min_Game) < 0) Min_Game = Cur_Game; 
			Cur_Game += strlen(Cur_Game) + 1;
		}

		strlwr(Min_Game);
		strcpy(LG + dep, Min_Game);
		dep += strlen(Min_Game) + 1;
		Min_Game[0] = -1;
	}

	GetWindowText(HWnd, name, 2046);
	backup_infos[0] = Controller_1_Type;
	backup_infos[1] = Controller_2_Type;

	memset(&K_Infos, 0, sizeof(K_Infos));
	
	K_Infos.appName = "Gens 2.10";
	K_Infos.gameList = LG;
	K_Infos.gameCallback = Play_Net_Game;

//	K_Infos.chatReceivedCallback = NULL;
//	K_Infos.clientDroppedCallback = NULL;
//	K_Infos.moreInfosCallback = NULL;

	Kaillera_Set_Infos(&K_Infos);

	Kaillera_Client_Running = 1;
	Kaillera_Select_Server_Dialog(NULL);
	Kaillera_Client_Running = 0;

	Controller_1_Type = backup_infos[0];
	Controller_2_Type = backup_infos[1];
	Make_IO_Table();
	SetWindowText(HWnd, name);

	free(Liste_Games);
	free(LG);

	return 1;
}

 
#ifdef CC_SUPPORT
void CC_End_Callback(char mess[256])
{
	MessageBox(HWnd, mess, "Console Classix", MB_OK);

	if (Sound_Initialised) Clear_Sound_Buffer();
	Debug = 0;
	Free_Rom(Game);
	Build_Main_Menu();
}
#endif


BOOL Init(HINSTANCE hInst, int nCmdShow)
{
	int i;

	timeBeginPeriod(1);

	Net_Play = 0;
	Full_Screen = -1;
	VDP_Num_Vis_Lines = 224;
	Resolution = 1;
	W_VSync = 0;
	FS_VSync = 0;
	Stretch = 0;
	Sprite_Over = 1;
	VScrollAl = 1; // Modif N.
	VScrollBl = 1; // Modif N.
	VScrollAh = 1; // Modif N.
	VScrollBh = 1; // Modif N.
	VSpritel = 1; // Modif U.
	VSpriteh = 1; // Modif U.
	ScrollAOn = 1;
	ScrollBOn = 1;
	SpriteOn = 1;
	Sprite_Always_Top = 0;
	Render_W = 0;
	Render_FS = 0;
	Show_Message = 1;

	Sound_Enable = 0;
	Sound_Segs = 8;
	Sound_Stereo = 1;
	Sound_Initialised = 0;
	Sound_Is_Playing = 0;
	WAV_Dumping = 0;
	GYM_Dumping = 0;

	FS_Minimised = 0;
	Game = NULL;
	Genesis_Started = 0;
	SegaCD_Started = 0;
	_32X_Started = 0;
	Debug = 0;
	CPU_Mode = 0;
	Window_Pos.x = 0;
	Window_Pos.y = 0;

	WndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = WinProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInst;
	WndClass.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SONIC));
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = NULL;
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = "Gens";

	FrameCount=0;
	LagCount = 0;

	RegisterClass(&WndClass);

	ghInstance = hInst;

	HWnd = CreateWindowEx(
		NULL,
		"Gens",
		"Gens - Idle",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		320 * 2,
		240 * 2,
		NULL,
		NULL,
		hInst,
		NULL);

	if (!HWnd) return FALSE;

	hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(RAC));
  
	Identify_CPU();

	i = GetVersion();
 
	// Get major and minor version numbers of Windows

	if (((i & 0xFF) > 4) || (i & 0x80000000)) WinNT_Flag = 0;
	else WinNT_Flag = 1;

	GetCurrentDirectory(1024, Gens_Path);
	GetCurrentDirectory(1024, Language_Path);
	GetCurrentDirectory(1024, Str_Tmp);
	strcpy(Manual_Path, "");
	strcpy(CGOffline_Path, "");

	strcat(Gens_Path, "\\");
	strcat(Language_Path, "\\language.dat");
	strcat(Str_Tmp, "\\gens.cfg");

	MSH2_Init();
	SSH2_Init();
	M68K_Init();
	S68K_Init();
	Z80_Init();

	YM2612_Init(CLOCK_NTSC / 7, Sound_Rate, YM2612_Improv);
	PSG_Init(CLOCK_NTSC / 15, Sound_Rate);
	PWM_Init();

	Load_Config(Str_Tmp, NULL);
	ShowWindow(HWnd, nCmdShow);

	if (!Init_Input(hInst, HWnd))
	{
		End_Sound();
		End_DDraw();
		return FALSE;
	}

	Init_CD_Driver();
	Init_Network();
	Init_Tab();
	Build_Main_Menu();

	SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &SS_Actived, 0);
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);

	switch(Gens_Priority)
	{
		case 0:
			SetThreadPriority(hInst, THREAD_PRIORITY_BELOW_NORMAL);
			break;

		case 2:
			SetThreadPriority(hInst, THREAD_PRIORITY_ABOVE_NORMAL);
			break;

		case 3:
			SetThreadPriority(hInst, THREAD_PRIORITY_HIGHEST);
			break;

		case 5:
			SetThreadPriority(hInst, THREAD_PRIORITY_TIME_CRITICAL);
			break;
	}

	Put_Info("   Gens Initialized", 1); // Modif N. -- added mainly to clear out some message gunk

	Gens_Running = 1;

	return TRUE;
}


void End_All(void)
{
	Free_Rom(Game);
	End_DDraw();
	End_Input();
	YM2612_End();
	End_Sound();
	End_CD_Driver();
	End_Network();

	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, SS_Actived, NULL, 0);
	if(MainMovie.File!=NULL)
		CloseMovieFile(&MainMovie);
	Close_AVI();

	timeEndPeriod(1);
}


int PASCAL WinMain(HINSTANCE hInst,	HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
///////////////////////////////////////////////////

	fp1 = fopen( "hook_log.txt", "r" );
	if( fp1 )
	{
		rd_mode = new unsigned int[ STATES ];
		wr_mode = new unsigned int[ STATES ];
		pc_mode = new unsigned int[ STATES ];
		ppu_mode = new unsigned int[ STATES ];

		rd_low = new unsigned int[ STATES ];
		wr_low = new unsigned int[ STATES ];
		pc_low = new unsigned int[ STATES ];
		ppu_low = new unsigned int[ STATES ];
		
		rd_high = new unsigned int[ STATES ];
		wr_high = new unsigned int[ STATES ];
		pc_high = new unsigned int[ STATES ];
		ppu_high = new unsigned int[ STATES ];

		pc_start = new unsigned int[ STATES ];

		fscanf(fp1,"hook_pc1 %x %x %x\n",&pc_mode[0],&pc_low[0],&pc_high[0]);
		fscanf(fp1,"hook_pc2 %x %x %x\n",&pc_mode[1],&pc_low[1],&pc_high[1]);
		fscanf(fp1,"hook_pc3 %x %x %x\n",&pc_mode[2],&pc_low[2],&pc_high[2]);

		fscanf(fp1,"hook_rd1 %x %x %x\n",&rd_mode[0],&rd_low[0],&rd_high[0]);
		fscanf(fp1,"hook_rd2 %x %x %x\n",&rd_mode[1],&rd_low[1],&rd_high[1]);
		fscanf(fp1,"hook_rd3 %x %x %x\n",&rd_mode[2],&rd_low[2],&rd_high[2]);

		fscanf(fp1,"hook_wr1 %x %x %x\n",&wr_mode[0],&wr_low[0],&wr_high[0]);
		fscanf(fp1,"hook_wr2 %x %x %x\n",&wr_mode[1],&wr_low[1],&wr_high[1]);
		fscanf(fp1,"hook_wr3 %x %x %x\n",&wr_mode[2],&wr_low[2],&wr_high[2]);

		fscanf(fp1,"hook_ppu1 %x %x %x\n",&ppu_mode[0],&ppu_low[0],&ppu_high[0]);
		fscanf(fp1,"hook_ppu2 %x %x %x\n",&ppu_mode[1],&ppu_low[1],&ppu_high[1]);
		fscanf(fp1,"hook_ppu3 %x %x %x\n",&ppu_mode[2],&ppu_low[2],&ppu_high[2]);

		pc_start[0] = 0;
		pc_start[1] = 0;
		pc_start[2] = 0;

		fclose( fp1 );
	}

	fp1 = fopen( "hook_log_cd.txt", "r" );
	if( fp1 )
	{
		rd_mode_cd = new unsigned int[ STATES ];
		wr_mode_cd = new unsigned int[ STATES ];
		pc_mode_cd = new unsigned int[ STATES ];
		ppu_mode_cd = new unsigned int[ STATES ];

		rd_low_cd = new unsigned int[ STATES ];
		wr_low_cd = new unsigned int[ STATES ];
		pc_low_cd = new unsigned int[ STATES ];
		ppu_low_cd = new unsigned int[ STATES ];
		
		rd_high_cd = new unsigned int[ STATES ];
		wr_high_cd = new unsigned int[ STATES ];
		pc_high_cd = new unsigned int[ STATES ];
		ppu_high_cd = new unsigned int[ STATES ];

		pc_start_cd = new unsigned int[ STATES ];

		fscanf(fp1,"hook_pc1 %x %x %x\n",&pc_mode_cd[0],&pc_low_cd[0],&pc_high_cd[0]);
		fscanf(fp1,"hook_pc2 %x %x %x\n",&pc_mode_cd[1],&pc_low_cd[1],&pc_high_cd[1]);
		fscanf(fp1,"hook_pc3 %x %x %x\n",&pc_mode_cd[2],&pc_low_cd[2],&pc_high_cd[2]);

		fscanf(fp1,"hook_rd1 %x %x %x\n",&rd_mode_cd[0],&rd_low_cd[0],&rd_high_cd[0]);
		fscanf(fp1,"hook_rd2 %x %x %x\n",&rd_mode_cd[1],&rd_low_cd[1],&rd_high_cd[1]);
		fscanf(fp1,"hook_rd3 %x %x %x\n",&rd_mode_cd[2],&rd_low_cd[2],&rd_high_cd[2]);

		fscanf(fp1,"hook_wr1 %x %x %x\n",&wr_mode_cd[0],&wr_low_cd[0],&wr_high_cd[0]);
		fscanf(fp1,"hook_wr2 %x %x %x\n",&wr_mode_cd[1],&wr_low_cd[1],&wr_high_cd[1]);
		fscanf(fp1,"hook_wr3 %x %x %x\n",&wr_mode_cd[2],&wr_low_cd[2],&wr_high_cd[2]);

		fscanf(fp1,"hook_ppu1 %x %x %x\n",&ppu_mode_cd[0],&ppu_low_cd[0],&ppu_high_cd[0]);
		fscanf(fp1,"hook_ppu2 %x %x %x\n",&ppu_mode_cd[1],&ppu_low_cd[1],&ppu_high_cd[1]);
		fscanf(fp1,"hook_ppu3 %x %x %x\n",&ppu_mode_cd[2],&ppu_low_cd[2],&ppu_high_cd[2]);

		pc_start_cd[0] = 0;
		pc_start_cd[1] = 0;
		pc_start_cd[2] = 0;

		fclose( fp1 );
	}

///////////////////////////////////////////////////

	MSG msg;
	long int OldFrame=-1;//Modif

	InitMovie(&MainMovie);

	Init(hInst, nCmdShow);

	// Have to do it *before* load by command line
	Init_Genesis_Bios();

	if (lpCmdLine[0])
	{
		int src;

#ifdef CC_SUPPORT
//		src = CC_Connect("CCGEN://Stef:gens@emu.consoleclassix.com/sonicthehedgehog2.gen", (char *) Rom_Data, CC_End_Callback);
		src = CC_Connect(lpCmdLine, (char *) Rom_Data, CC_End_Callback);

		if (src == 0)
		{
			Load_Rom_CC(CCRom.RName, CCRom.RSize);
			Build_Main_Menu();
		}
		else if (src == 1)
		{
			MessageBox(NULL, "Error during connection", NULL, MB_OK);
		}
		else if (src == 2)
		{
#endif
		src = 0;
		
		if (lpCmdLine[src] == '"')
		{
			src++;
			
			while ((lpCmdLine[src] != '"') && (lpCmdLine[src] != 0))
			{
				Str_Tmp[src - 1] = lpCmdLine[src];
				src++;
			}

			Str_Tmp[src - 1] = 0;
		}
		else
		{
			while (lpCmdLine[src] != 0)
			{
				Str_Tmp[src] = lpCmdLine[src];
				src++;
			}

			Str_Tmp[src] = 0;
		}

		Pre_Load_Rom(HWnd, Str_Tmp);

#ifdef CC_SUPPORT
		}
#endif
	}

	for (char r = 0; r <= 0x1F; r++)
	{
		for (char g = 0; g <= 0x3F; g++)
		{
			for (char b = 0; b <= 0x1F; b++)
			{
				Pal32_XRAY[(r << 11) | (g << 5) | b] = (r << 19) | (g << 10) | (b << 3);
			}
		}
	}
	while (Gens_Running)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0)) Gens_Running = 0;
			{
				if (!RamSearchHWnd || !IsDialogMessage(RamSearchHWnd, &msg))
				{
					if (!RamWatchHWnd || !IsDialogMessage(RamWatchHWnd, &msg))
					{
						if (!VolControlHWnd || !IsDialogMessage(VolControlHWnd, &msg))
						{
							if (!TranslateAccelerator (HWnd, hAccelTable, &msg))
							{
								TranslateMessage(&msg); 
								DispatchMessage(&msg);
							}
						}
					}
				}
			}
		}

#ifdef GENS_DEBUG
		if (Debug)						// DEBUG
		{
			Update_Debug_Screen();
			Flip(HWnd);
		}
		else
#endif
		if (Genesis_Started || _32X_Started || SegaCD_Started)
		{
			static DWORD tgtime = timeGetTime(); //Modif N - give frame advance sound:
			static bool soundCleared = false;

			if((Active) && QuickPauseKey!=0)
			{
				if(Check_Pause_Key())
				{
					if (Debug)
					{
						Change_Debug(HWnd, 0);
						Paused = 0;
						Build_Main_Menu();
					}
					else if (Paused)
					{
						Paused = 0;
					}
					else
					{
						Paused = 1;
						Pause_Screen();
						Clear_Sound_Buffer();
						Flip(HWnd);
					}
				}
			}
			Check_Misc_Key();
			if((Active) && !Paused && SkipKey!=0) // so that the frame advance key can pause even if pause on a gamekey isn't set
			{
				if (Check_Skip_Key() && !Paused)
				{
					Paused = 1;
					Clear_Sound_Buffer();
					Update_Emulation_One(HWnd);
					soundCleared = false;
					tgtime = timeGetTime();
				}
			}
			if (MustUpdateMenu)
			{
				Build_Main_Menu();
				MustUpdateMenu=0;
			}
			if ((Active) && (!Paused))	// EMULATION
			{
				Update_Emulation(HWnd);

				//Modif N - don't hog 100% of CPU power:
				static int count = 0;
				count++;
				unsigned short FFState = GetAsyncKeyState(VK_TAB);
				if(!((FFState & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000) && GetActiveWindow()==HWnd)) //Modif N - part of a quick hack to make Tab the fast-forward key
					if(Frame_Skip == -1 || ((count % (Frame_Skip+2)) == 0))
						Sleep(1); //Modif N
			}
			else		// EMULATION PAUSED
			{
				if(SkipKey)
				{
					if(Check_Skip_Key() != 0)
					{
						Update_Emulation_One(HWnd);
						soundCleared = false;
						tgtime = timeGetTime();
					}
					else
					{
						if(!soundCleared && timeGetTime() - tgtime >= 125) //eliminate stutter
						{
							Clear_Sound_Buffer();
							soundCleared = true;
						}
					}
				}
				Sleep(1);
			}
		}
		else if (GYM_Playing)			// PLAY GYM
		{
			Play_GYM();
			Update_Gens_Logo(HWnd);
		}
		else if (Intro_Style == 1)		// GENS LOGO EFFECT
		{
			Update_Gens_Logo(HWnd);
			Sleep(20);
		}
		else if (Intro_Style == 2)		// STRANGE EFFECT
		{
			Update_Crazy_Effect(HWnd);
			Sleep(20);
		}
		else if (Intro_Style == 3)		// GENESIS BIOS
		{
			Do_Genesis_Frame();
			Flip(HWnd);
			Sleep(20);
		}
		else							// BLANK SCREEN (MAX IDLE)
		{
			Clear_Back_Screen(HWnd);
			Flip(HWnd);
			Sleep(200);
		}
	}

	End_Sound(); //Modif N - making sure sound doesn't stutter upon exit

	strcpy(Str_Tmp, Gens_Path);
	strcat(Str_Tmp, "Gens.cfg");
	Save_Config(Str_Tmp);

	End_All(); //Modif N

	ChangeDisplaySettings(NULL, 0);

	DestroyWindow(HWnd);

	while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, 0, 0)) return msg.wParam;
	}

	TerminateProcess(GetCurrentProcess, 0); //Modif N

	return 0;
}


long PASCAL WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	int t;

	switch(message)
	{
		case WM_ACTIVATE:
			if (Gens_Running == 0) break;

			if (LOWORD(wParam) != WA_INACTIVE)
			{
				Active = 1;

				if (FS_Minimised && !(DialogsOpen))
				{
					FS_Minimised = 0;
					Set_Render(hWnd, 1, -1, true);
				}
			}
			else
			{
				if ((Full_Screen) && ((BOOL) HIWORD(wParam)) && (Active))
				{
					Set_Render(hWnd, 0, -1, false);
					FS_Minimised = 1;
				}

				if (Auto_Pause && Active)
				{
					Active = 0;

					if (!Paused) Pause_Screen();
					Clear_Sound_Buffer();
				}
			}
			break;

		case WM_MENUSELECT:
 		case WM_ENTERSIZEMOVE:
			Clear_Sound_Buffer();
			break;

 		case WM_EXITSIZEMOVE:
			if (!Full_Screen)
			{
				GetWindowRect(HWnd, &r);
				Window_Pos.x = r.left;
				Window_Pos.y = r.top;
			}
			break;

		case WM_CLOSE:
			if (Sound_Initialised) Clear_Sound_Buffer(); //Modif N - making sure sound doesn't stutter on exit
			if(MainMovie.File!=NULL)
				CloseMovieFile(&MainMovie);
			if ((Check_If_Kaillera_Running())) return 0;
			Gens_Running = 0;
			return 0;
		
		case WM_RBUTTONDOWN:
			if (Full_Screen)
			{
				Clear_Sound_Buffer();
				//SetCursorPos(40, 30);
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
				POINT point;
				GetCursorPos(&point);
				SendMessage(hWnd, WM_PAINT, 0,0);
				Restore_Primary();
				PaintsEnabled = false;
				TrackPopupMenu(Gens_Menu, TPM_LEFTALIGN | TPM_TOPALIGN, point.x, point.y, NULL, hWnd, NULL);
				PaintsEnabled = true;
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			break;

		case WM_CREATE:
			Active = 1;
			break;
		
		case WM_PAINT:
			{
				HDC         hDC;
				PAINTSTRUCT ps;

				hDC = BeginPaint(hWnd, &ps);

				if(PaintsEnabled)
				{
					Clear_Primary_Screen(HWnd);
					Flip(hWnd);
				}

				EndPaint(hWnd, &ps);
				break;
			}
			break;
		
		case WM_COMMAND:
			if ((LOWORD(wParam) >= ID_HELP_LANG) && (LOWORD(wParam) < ID_HELP_LANG + 50))
			{
				Language = LOWORD(wParam) - ID_HELP_LANG;
				Build_Main_Menu();
				return 0;
			}
			else switch(LOWORD(wParam))
			{
				case ID_GRAPHICS_NEVER_SKIP_FRAME:
					Never_Skip_Frame = !Never_Skip_Frame;
					Build_Main_Menu();
					return 0;
				case ID_GRAPHICS_AVI_SOUND:
					if(AVISound)
					{
						AVISound = 0;
					}
					else
					{
						AVISound = 1;
					}
					Build_Main_Menu();
					return 0;
				case ID_GRAPHICS_SYNC_AVI_MOVIE:
					if(AVIWaitMovie)
					{
						AVIWaitMovie = 0;
					}
					else
					{
						AVIWaitMovie = 1;
					}
					Build_Main_Menu();
					return 0;
				case ID_GRAPHICS_AVI:
					if(AVIRecording)
					{
						AVIRecording=0;
						Close_AVI();
					}
					else
					{
						if(InitAVI())
							AVIRecording=1;
						else
							AVIRecording=0;
					}
					Build_Main_Menu();
					return 0;
				case ID_SLOW_SPEED_1:
					SlowDownSpeed=1;
					Build_Main_Menu();
					return 0;
				case ID_SLOW_SPEED_2:
					SlowDownSpeed=2;
					Build_Main_Menu();
					return 0;
				case ID_SLOW_SPEED_3:
					SlowDownSpeed=3;
					Build_Main_Menu();
					return 0;
				case ID_SLOW_SPEED_4:
					SlowDownSpeed=4;
					Build_Main_Menu();
					return 0;
				case ID_SLOW_SPEED_5:
					SlowDownSpeed=5;
					Build_Main_Menu();
					return 0;
				case ID_SLOW_SPEED_9:
					SlowDownSpeed=9;
					Build_Main_Menu();
					return 0;
				case ID_SLOW_SPEED_15:
					SlowDownSpeed=15;
					Build_Main_Menu();
					return 0;
				case ID_SLOW_SPEED_31:
					SlowDownSpeed=31;
					Build_Main_Menu();
					return 0;
				case ID_SLOW_MODE:
					if(SlowDownMode==1)
						SlowDownMode=0;
					else
						SlowDownMode=1;
					Build_Main_Menu();
					return 0;
				case ID_SLOW_SPEED_PLUS: //Modif N - for new "speed up" key:
					if(SlowDownSpeed==1 || SlowDownMode==0)
						SlowDownMode=0;
					else
					{
						SlowDownMode=1;
						if(SlowDownSpeed<=5)
							SlowDownSpeed--;
						else if(SlowDownSpeed==9)
							SlowDownSpeed=5;
						else if(SlowDownSpeed==15)
							SlowDownSpeed=9;
						else if(SlowDownSpeed==31)
							SlowDownSpeed=15;
					}
					Str_Tmp[0] = '\0';
					if(SlowDownMode==0)
						strcpy(Str_Tmp, "100%");
					else switch(SlowDownSpeed)
					{
						case 1: strcpy(Str_Tmp, "50%"); break;
						case 2: strcpy(Str_Tmp, "25%"); break;
						case 3: strcpy(Str_Tmp, "20%"); break;
						case 4: strcpy(Str_Tmp, "16%"); break;
						case 5: strcpy(Str_Tmp, "10%"); break;
						case 9: strcpy(Str_Tmp,  "6%"); break;
						case 15: strcpy(Str_Tmp, "3%"); break;
					}
					if(Str_Tmp[0])
						Put_Info(Str_Tmp, 1000);

					Build_Main_Menu();
					return 0;
				case ID_SLOW_SPEED_MINUS: //Modif N - for new "speed down" key:
					if(SlowDownMode!=0)
					{
						if(SlowDownSpeed<5)
							SlowDownSpeed++;
						else if(SlowDownSpeed==5)
							SlowDownSpeed=9;
						else if(SlowDownSpeed==9)
							SlowDownSpeed=15;
						else if(SlowDownSpeed==15)
							SlowDownSpeed=31;
					}
					SlowDownMode=1;

					Str_Tmp[0] = '\0';
					switch(SlowDownSpeed)
					{
						case 1: strcpy(Str_Tmp, "50%"); break;
						case 2: strcpy(Str_Tmp, "25%"); break;
						case 3: strcpy(Str_Tmp, "20%"); break;
						case 4: strcpy(Str_Tmp, "16%"); break;
						case 5: strcpy(Str_Tmp, "10%"); break;
						case 9: strcpy(Str_Tmp,  "6%"); break;
						case 15: strcpy(Str_Tmp, "3%"); break;
					}
					if(Str_Tmp[0])
						Put_Info(Str_Tmp, 1000);

					Build_Main_Menu();
					return 0;
				case ID_TOGGLE_MOVIE_READONLY: //Modif N - for new toggle readonly key:
					MainMovie.ReadOnly = !MainMovie.ReadOnly;
					if(MainMovie.File!=NULL)
					{
						if(MainMovie.ReadOnly)
							Put_Info("Movie is now read-only.", 1000);
						else
							Put_Info("Movie is now editable.", 1000);
					}
					else
					{
						Put_Info("Can't toggle read-only; no movie is active.", 1000);
					}
					break;
				case ID_RAM_SEARCH:
					if(!RamSearchHWnd)
					{
						RamSearchHWnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_RAMSEARCH), hWnd, (DLGPROC) RamSearchProc);
						DialogsOpen++;
					}
					else
						SetForegroundWindow(RamSearchHWnd);
					break;
				case ID_RAM_WATCH:
					if(!RamWatchHWnd)
					{
						RamWatchHWnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_RAMWATCH), hWnd, (DLGPROC) RamWatchProc);
						DialogsOpen++;
					}
					else
						SetForegroundWindow(RamWatchHWnd);
					break;
				case ID_VOLUME_CONTROL:
					if(!VolControlHWnd)
					{
						VolControlHWnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_VOLUME), hWnd, (DLGPROC) VolumeProc);
						DialogsOpen++;
					}
					else
						SetForegroundWindow(VolControlHWnd);
					break;
				case ID_RESUME_RECORD:
					if(!(Game))
						return 0;
					if(MainMovie.Status!=MOVIE_PLAYING)
					{
						MESSAGE_L("Error: no movie is playing", "Error: no movie is playing", 1500);
						return 0;
					}
					if(MainMovie.ReadOnly)
					{
						MESSAGE_L("Error: movie is read only", "Error: movie is read only", 1500);
						return 0;
					}
					strncpy(Str_Tmp,MainMovie.FileName,512);
					if(AutoBackupEnabled)
					{
						strcat(MainMovie.FileName,".gmv");
						MainMovie.FileName[strlen(MainMovie.FileName)-7]='b';
						MainMovie.FileName[strlen(MainMovie.FileName)-6]='a';
						MainMovie.FileName[strlen(MainMovie.FileName)-5]='k';
						BackupMovieFile(&MainMovie);
						strncpy(MainMovie.FileName,Str_Tmp,512);
					}
					MainMovie.Status=MOVIE_RECORDING;
					MainMovie.NbRerecords++;
					if (MainMovie.TriplePlayerHack)
						MainMovie.LastFrame=max(max(max(Track1_FrameCount,Track2_FrameCount),Track3_FrameCount),FrameCount);
					else
						MainMovie.LastFrame=max(max(Track1_FrameCount,Track2_FrameCount),FrameCount);
					MESSAGE_L("Recording from current frame", "Recording from current frame", 1500);
					Build_Main_Menu();
					return 0;
				case ID_STOP_MOVIE:
					if(!(Game))
						return 0;
					if(MainMovie.Status == MOVIE_RECORDING)
						MovieRecordingStuff();
					if(MainMovie.File!=NULL)
						CloseMovieFile(&MainMovie);
					MainMovie.Status=0;
					MESSAGE_L("Recording/Playing stop", "Recording/Playing stop", 1500);
					Build_Main_Menu();
					return 0;
				case ID_RECORD_MOVIE:	//Modif
					if(!(Game))
						if(SendMessage(hWnd, WM_COMMAND, ID_FILES_OPENROM, 0) <= 0) // Modif N. -- prompt once to load ROM if it's not already loaded
							return 0;
//					if(MainMovie.File!=NULL || MainMovie.Status==MOVIE_FINISHED) //Modif N - disabled; if the user chose record, they meant it!
//						return 0;
					MINIMIZE
dialogAgain: //Nitsuja added this
					DialogsOpen++;
					DialogBox(ghInstance, MAKEINTRESOURCE(IDD_RECORD_A_MOVIE), hWnd, (DLGPROC) RecordMovieProc);
					if(RecordMovieCanceled)
						return 0;
					if(MainMovie.StateRequired)
					{
						FrameCount=0;
						LagCount = 0;
						strncpy(Str_Tmp,Rom_Name,507);
						strcat(Str_Tmp,"[GMV]");
						strcat(Str_Tmp,".gst");
						Change_File_S(Str_Tmp, Movie_Dir, "Save state", "State Files\0*.gs?\0All Files\0*.*\0\0", "");
						if(Save_State(Str_Tmp)==0)
							return 0;
					}
					MainMovie.File=fopen(MainMovie.FileName,"wb");

					if(!MainMovie.File)
					{
						char pathError [256];
						char* slash = strrchr(MainMovie.FileName, '\\');
						char* slash2 = strrchr(MainMovie.FileName, '/');
						if(slash > slash2) *(slash+1) = 0;
						if(slash2 > slash) *(slash2+1) = 0;
						sprintf(pathError, "Invalid path: %s", MainMovie.FileName);
						MessageBox(hWnd, pathError, "Error", MB_OK | MB_ICONERROR);
						MainMovie.Status=0;
						goto dialogAgain;
						//char* div = strrchr(MainMovie.FileName, '\\');
						//if(!div) div = strrchr(MainMovie.FileName, '/');
						//if(div)
						//{
						//	memmove(MainMovie.FileName, div+1, strlen(div));
						//	MainMovie.File=fopen(MainMovie.FileName,"wb");
						//}
					}

					if(!MainMovie.File)
					{
						MessageBox(hWnd, MainMovie.FileName, "Error", MB_OK);

						MainMovie.Status=0;
						MESSAGE_L("File error", "File error", 1500)
						return 0;
					}
					MainMovie.Ok=1;
					fseek(MainMovie.File,0,SEEK_SET);
					fwrite(MainMovie.Header,16,1,MainMovie.File);
					fseek(MainMovie.File,24,SEEK_SET);
					fwrite(MainMovie.Note,40,1,MainMovie.File);
					fclose(MainMovie.File);
					MainMovie.File=NULL;
					if(OpenMovieFile(&MainMovie)==0)
					{
						MainMovie.Status=0;
						MESSAGE_L("File error", "File error", 1500)
						return 0;
					}
					FrameCount=0;
					LagCount = 0;
					MainMovie.NbRerecords=0;
					MainMovie.LastFrame=0;
					if(MainMovie.StateRequired)
					{
						MESSAGE_L("Recording from now", "Recording from now", 1500)
					}
					else
					{
						if (Genesis_Started)
						{
							Pre_Load_Rom(HWnd,Recent_Rom[0]);
							MESSAGE_L("Genesis reseted", "Genesis reset", 1500)
			                memset(SRAM, 0, sizeof(SRAM));
						}
						else if (_32X_Started)
						{
							Pre_Load_Rom(HWnd, Recent_Rom[0]);
							MESSAGE_L("32X reseted", "32X reset", 1500)
			                memset(SRAM, 0, sizeof(SRAM));
						}
						else if (SegaCD_Started)
						{
							if(CD_Load_System == CDROM_)
							{
								MessageBox(GetActiveWindow(), "Warning: You are running from a mounted CD. To prevent desyncs, it is recommended you run the game from a CUE or ISO file instead.", "Recording Warning", MB_OK | MB_ICONWARNING);
								Reset_SegaCD();
							}
							else
								Pre_Load_Rom(HWnd, Recent_Rom[0]);
							MESSAGE_L("SegaCD reseted", "SegaCD reset", 1500)
			                memset(SRAM, 0, sizeof(SRAM));
							Format_Backup_Ram();
						}
						MESSAGE_L("Recording from start", "Recording from start", 1500)
					}
					Build_Main_Menu();
					return 0;
				case ID_PLAY_MOVIE:
//					if(MainMovie.Status==MOVIE_RECORDING) //Modif N - disabled; if the user chose playback, they meant it!
//						return 0;
//					if(MainMovie.Status==MOVIE_PLAYING || MainMovie.Status==MOVIE_FINISHED)
//						CloseMovieFile(&MainMovie);
					if(!(Game))
 						if(SendMessage(hWnd, WM_COMMAND, ID_FILES_OPENROM, 0) <= 0) // Modif N. -- prompt once to load ROM if it's not already loaded
							return 0;

					// Modif N. -- added so that a movie that's currently being recorded doesn't show up with bogus info in the movie play dialog
					if(!MainMovie.ReadOnly && MainMovie.File)
						WriteMovieHeader(&MainMovie);

					MINIMIZE
					DialogsOpen++;
					DialogBox(ghInstance, MAKEINTRESOURCE(IDD_PLAY_MOVIE), hWnd, (DLGPROC) PlayMovieProc);
					if (PlayMovieCanceled) return 0;
					if(OpenMovieFile(&MainMovie)==0)
					{
						MESSAGE_L("Error opening file", "Error opening file", 1500)
						MainMovie.Status=0;
						return 0;
					}
					FrameCount=0;
					LagCount = 0;
					if(MainMovie.UseState)
					{
						if(MainMovie.Status==MOVIE_PLAYING)
						{
							t=MainMovie.ReadOnly;
							MainMovie.ReadOnly = 1;
							Load_State(MainMovie.StateName);
							MainMovie.ReadOnly = t;
						}
						else
						{
							Load_State(MainMovie.StateName);
						}
						if(MainMovie.Status==MOVIE_RECORDING)
						{
							Str_Tmp[0] = 0;
							Get_State_File_Name(Str_Tmp);
							Save_State(Str_Tmp);
						}
					}
					else
					{
						// Modif N. reload currently-loaded ROM (more thorough than a Reset, apparently necessary) and clear out SRAM and BRAM
						int wasPaused = Paused;
						if (Genesis_Started)
						{
							Pre_Load_Rom(HWnd,Recent_Rom[0]);
							MESSAGE_L("Genesis reseted", "Genesis reset", 1500)
			                memset(SRAM, 0, sizeof(SRAM));
						}
						else if (_32X_Started)
						{
							Pre_Load_Rom(HWnd, Recent_Rom[0]);
							MESSAGE_L("32X reseted", "32X reset", 1500)
			                memset(SRAM, 0, sizeof(SRAM));
						}
						else if (SegaCD_Started)
						{
							if(CD_Load_System == CDROM_)
								Reset_SegaCD();
							else
								Pre_Load_Rom(HWnd, Recent_Rom[0]);
							MESSAGE_L("SegaCD reseted", "SegaCD reset", 1500)
			                memset(SRAM, 0, sizeof(SRAM));
							Format_Backup_Ram();
						}
						Paused = wasPaused; 
					}
					if(MainMovie.Status==MOVIE_PLAYING && MainMovie.UseState!=0)
					{
						if(CPU_Mode)
							sprintf(Str_Tmp, "Playing from savestate. %d frames. %d rerecords. %d min %2.2f s",MainMovie.LastFrame,MainMovie.NbRerecords,MainMovie.LastFrame/50/60,(MainMovie.LastFrame%3000)/60.0);
						else
							sprintf(Str_Tmp, "Playing from savestate. %d frames. %d rerecords. %d min %2.2f s",MainMovie.LastFrame,MainMovie.NbRerecords,MainMovie.LastFrame/60/60,(MainMovie.LastFrame%3600)/60.0);
					}
					else if(MainMovie.Status==MOVIE_PLAYING && MainMovie.UseState==0)
					{
						if(CPU_Mode)
							sprintf(Str_Tmp, "Playing from reset. %d frames. %d rerecords. %d min %2.2f s",MainMovie.LastFrame,MainMovie.NbRerecords,MainMovie.LastFrame/50/60,(MainMovie.LastFrame%3000)/60.0);
						else
							sprintf(Str_Tmp, "Playing from reset. %d frames. %d rerecords. %d min %2.2f s",MainMovie.LastFrame,MainMovie.NbRerecords,MainMovie.LastFrame/60/60,(MainMovie.LastFrame%3600)/60.0);
					}
					else
						wsprintf(Str_Tmp, "Resuming recording from savestate. Frame %d.",FrameCount);
					Put_Info(Str_Tmp, 4500);
					Build_Main_Menu();
					return 0;
				
				case ID_SPLICE:
					if (SpliceFrame)
					{
						DoMovieSplice();
						return 0;
					}
					else if (MainMovie.File != NULL)
					{
						DialogsOpen++;
						DialogBox(ghInstance, MAKEINTRESOURCE(IDD_PROMPT), hWnd, (DLGPROC) PromptSpliceFrameProc);
						return 0;
					}
					else
						return 1;

				case IDC_SEEK_FRAME:
					if (SeekFrame)
					{
						SeekFrame = 0;
						MustUpdateMenu = 1;
						MESSAGE_L("Seek Cancelled","Seek Cancelled",1500);
						return 0;
					}
					else
					{
						DialogsOpen++;
						DialogBox(ghInstance, MAKEINTRESOURCE(IDD_PROMPT), hWnd, (DLGPROC) PromptSeekFrameProc);
						return 0;
					}

				case ID_FILES_QUIT:
					PostMessage(hWnd, WM_CLOSE, 0, 0);
					return 0;

				case ID_FILES_OPENROM:
				{
					if(MainMovie.File!=NULL)
						CloseMovieFile(&MainMovie);
					if ((Check_If_Kaillera_Running())) return 0;
					MINIMIZE
					if (GYM_Playing) Stop_Play_GYM();
					FrameCount=0;
					LagCount = 0;
					int retval = Get_Rom(hWnd);
					ReopenRamWindows();
					return retval;
				}
				case ID_FILES_OPENRECENTROM0:
				case ID_FILES_OPENRECENTROM1:
				case ID_FILES_OPENRECENTROM2:
				case ID_FILES_OPENRECENTROM3:
				case ID_FILES_OPENRECENTROM4:
				case ID_FILES_OPENRECENTROM5:
				case ID_FILES_OPENRECENTROM6:
				case ID_FILES_OPENRECENTROM7:
				case ID_FILES_OPENRECENTROM8:
				case ID_FILES_OPENRECENTROM9:
				case ID_FILES_OPENRECENTROM10:
				case ID_FILES_OPENRECENTROM11:
				case ID_FILES_OPENRECENTROM12:
				case ID_FILES_OPENRECENTROM13:
				case ID_FILES_OPENRECENTROM14:
				{
					if(MainMovie.File!=NULL)

						CloseMovieFile(&MainMovie);
					if ((Check_If_Kaillera_Running())) return 0;
					if (GYM_Playing) Stop_Play_GYM();
					FrameCount=0;
					LagCount = 0;
					int retval = Pre_Load_Rom(HWnd, Recent_Rom[LOWORD(wParam) - ID_FILES_OPENRECENTROM0]);
					ReopenRamWindows();
					return retval;
				}

				case ID_FILES_BOOTCD:
					if(MainMovie.File!=NULL)
						CloseMovieFile(&MainMovie);
					if (Num_CD_Drive == 0) return 1;
					if (Check_If_Kaillera_Running()) return 0;
					if (GYM_Playing) Stop_Play_GYM();
					Free_Rom(Game);			// Don't forget it !
					SegaCD_Started = Init_SegaCD(NULL);
					Build_Main_Menu();
					FrameCount=0;
					LagCount = 0;
					ReopenRamWindows();
					return SegaCD_Started;

				case ID_FILES_OPENCLOSECD:
					if(MainMovie.File!=NULL)
						CloseMovieFile(&MainMovie);
					if (SegaCD_Started) Change_CD();
					FrameCount=0;
					LagCount = 0;
					return 0;

				case ID_FILES_NETPLAY:
					MINIMIZE
					if (GYM_Playing) Stop_Play_GYM();
					Start_Netplay();
					return 0;

				case ID_FILES_CLOSEROM:
					if(MainMovie.File!=NULL)
						CloseMovieFile(&MainMovie);
					if (Sound_Initialised) Clear_Sound_Buffer();
					Debug = 0;
					if (Net_Play)
					{
						if (Full_Screen) Set_Render(hWnd, 0, -1, true);
					}
					Free_Rom(Game);
					Build_Main_Menu();
					FrameCount=0;
					LagCount = 0;
					return 0;
		
				case ID_FILES_GAMEGENIE:
					if (Check_If_Kaillera_Running()) return 0;
					MINIMIZE
					DialogsOpen++;
					DialogBox(ghInstance, MAKEINTRESOURCE(IDD_GAMEGENIE), hWnd, (DLGPROC) GGenieProc);
					Build_Main_Menu();
					return 0;

				case ID_FILES_LOADSTATE:
					if (Check_If_Kaillera_Running()) return 0;
					Str_Tmp[0] = 0;
					Get_State_File_Name(Str_Tmp);
					Load_State(Str_Tmp);
					return 0;

				case ID_FILES_LOADSTATEAS:
					if (Check_If_Kaillera_Running()) return 0;
					Str_Tmp[0] = 0;
					DialogsOpen++;
					Change_File_L(Str_Tmp, State_Dir, "Load state", "State Files\0*.gs?\0All Files\0*.*\0\0", "");
					DialogsOpen--;
					Load_State(Str_Tmp);
					return 0;

				case ID_FILES_SAVESTATE:
					if (Check_If_Kaillera_Running()) return 0;
					Str_Tmp[0] = 0;
					Get_State_File_Name(Str_Tmp);
					Save_State(Str_Tmp);
					return 0;

				case ID_FILES_SAVESTATEAS:
					if (Check_If_Kaillera_Running()) return 0;
					DialogsOpen++;
					Change_File_S(Str_Tmp, State_Dir, "Save state", "State Files\0*.gs?\0All Files\0*.*\0\0", "");
					DialogsOpen--;
					Save_State(Str_Tmp);
					return 0;

				case ID_FILES_PREVIOUSSTATE:
					Set_Current_State(hWnd, (Current_State + 9) % 10);
					return 0;

				case ID_FILES_NEXTSTATE:
					Set_Current_State(hWnd, (Current_State + 1) % 10);
					return 0;

				case ID_GRAPHICS_VSYNC:
					Change_VSync(hWnd);
					return 0;

				case ID_GRAPHICS_SWITCH_MODE:
					if (Full_Screen) Set_Render(hWnd, 0, -1, true);
					else Set_Render(hWnd, 1, Render_FS, true);
					return 0;

				case ID_GRAPHICS_FS_SAME_RES: //Upth-Add - toggle the same-res fullscreen flag
					FS_No_Res_Change = !(FS_No_Res_Change);
					Build_Main_Menu();
					if (Full_Screen) Set_Render(hWnd, 1, Render_FS, true); // Modif N. -- if already in fullscreen, take effect immediately
					return 0;

				case ID_GRAPHICS_COLOR_ADJUST:
					if (Check_If_Kaillera_Running()) return 0;
					MINIMIZE
					DialogsOpen++;
					DialogBox(ghInstance, MAKEINTRESOURCE(IDD_COLOR), hWnd, (DLGPROC) ColorProc);
					return 0;

				case ID_GRAPHICS_RENDER_NORMAL:
					Set_Render(hWnd, Full_Screen, 0, false);
					return 0;

				case ID_GRAPHICS_RENDER_DOUBLE:
					Set_Render(hWnd, Full_Screen, 1, false);
					return 0;

				case ID_GRAPHICS_RENDER_EPX:
					Set_Render(hWnd, Full_Screen, 2, false);
					return 0;

				case ID_GRAPHICS_RENDER_DOUBLE_INT:
					Set_Render(hWnd, Full_Screen, 3, false);
					return 0;

				case ID_GRAPHICS_RENDER_FULLSCANLINE:
					Set_Render(hWnd, Full_Screen, 4, false);
					return 0;

				case ID_GRAPHICS_LAYER0: //Nitsuja added these
				case ID_GRAPHICS_LAYER1:
				case ID_GRAPHICS_LAYER2:
				case ID_GRAPHICS_LAYER3:
				case ID_GRAPHICS_LAYERSPRITE:
				case ID_GRAPHICS_LAYERSPRITEHIGH:
					Change_Layer(hWnd, LOWORD(wParam) - ID_GRAPHICS_LAYER0);
					return 0;

				case ID_GRAPHICS_SPRITEALWAYS:
					Change_SpriteTop(hWnd);
					return 0;

				case ID_GRAPHICS_LAYERSWAPA:
				case ID_GRAPHICS_LAYERSWAPB:
				case ID_GRAPHICS_LAYERSWAPS:
					Change_LayerSwap(hWnd, LOWORD(wParam) - ID_GRAPHICS_LAYERSWAPA);
					return 0;

				case ID_GRAPHICS_TOGGLEA:
				case ID_GRAPHICS_TOGGLEB:
				case ID_GRAPHICS_TOGGLES:
					Change_Plane(hWnd, LOWORD(wParam) - ID_GRAPHICS_TOGGLEA);
					return 0;

				case ID_GRAPHICS_RENDER_50SCANLINE:
					Set_Render(hWnd, Full_Screen, 5, false);
					return 0;

				case ID_GRAPHICS_RENDER_25SCANLINE:
					Set_Render(hWnd, Full_Screen, 6, false);
					return 0;

				case ID_GRAPHICS_RENDER_INTESCANLINE:
					Set_Render(hWnd, Full_Screen, 7, false);
					return 0;

				case ID_GRAPHICS_RENDER_INT50SCANLIN:
					Set_Render(hWnd, Full_Screen, 8, false);
					return 0;

				case ID_GRAPHICS_RENDER_INT25SCANLIN:
					Set_Render(hWnd, Full_Screen, 9, false);
					return 0;

				case ID_GRAPHICS_RENDER_2XSAI:
					Set_Render(hWnd, Full_Screen, 10, false);
					return 0;

				case ID_GRAPHICS_PREVIOUS_RENDER:
					if ((Full_Screen) && (Render_FS > 0)) Set_Render(hWnd, 1, Render_FS - 1, false);
					else if ((!Full_Screen) && (Render_W > 0)) Set_Render(hWnd, 0, Render_W - 1, false);
					return 0;

				case ID_GRAPHICS_NEXT_RENDER:
				{
					int Rend = (Full_Screen?Render_FS:Render_W);
					if ((Full_Screen) && (Render_FS < (Bits32?2:10))) Set_Render(hWnd, 1, Render_FS + 1, false);
					else if ((!Full_Screen) && (Render_W < (Bits32?2:10))) Set_Render(hWnd, 0, Render_W + 1, false);
					return 0;
				}

				case ID_GRAPHICS_STRETCH:
					Change_Stretch();
					return 0;
	
				case ID_GRAPHICS_FORCESOFT:
					Change_Blit_Style();
					return 0;
				
				case ID_GRAPHICS_FRAMESKIP_AUTO:
					Set_Frame_Skip(hWnd, -1);
					return 0;

				case ID_GRAPHICS_FRAMESKIP_0:
				case ID_GRAPHICS_FRAMESKIP_1:
				case ID_GRAPHICS_FRAMESKIP_2:
				case ID_GRAPHICS_FRAMESKIP_3:
				case ID_GRAPHICS_FRAMESKIP_4:
				case ID_GRAPHICS_FRAMESKIP_5:
				case ID_GRAPHICS_FRAMESKIP_6:
				case ID_GRAPHICS_FRAMESKIP_7:
				case ID_GRAPHICS_FRAMESKIP_8:
					Set_Frame_Skip(hWnd, LOWORD(wParam) - ID_GRAPHICS_FRAMESKIP_0);
					return 0;

				case ID_GRAPHICS_FRAMESKIP_DECREASE:
					if (Frame_Skip == -1)
					{
						Set_Frame_Skip(hWnd, 0);
					}
					else
					{
						if (Frame_Skip > 0) Set_Frame_Skip(hWnd, Frame_Skip - 1);
					}
					return 0;

				case ID_GRAPHICS_FRAMESKIP_INCREASE:
					if (Frame_Skip == -1)
					{
						Set_Frame_Skip(hWnd, 1);
					}
					else
					{
						if (Frame_Skip < 8) Set_Frame_Skip(hWnd, Frame_Skip + 1);
					}
					return 0;

				case ID_GRAPHICS_SPRITEOVER:
					Set_Sprite_Over(hWnd, Sprite_Over ^ 1);
					return 0;

				case ID_GRAPHICS_SHOT:
					Clear_Sound_Buffer();
					Take_Shot();
					Build_Main_Menu();
					return 0;

				case ID_FILES_STATE_0:
				case ID_FILES_STATE_1:
				case ID_FILES_STATE_2:
				case ID_FILES_STATE_3:
				case ID_FILES_STATE_4:
				case ID_FILES_STATE_5:
				case ID_FILES_STATE_6:
				case ID_FILES_STATE_7:
				case ID_FILES_STATE_8:
				case ID_FILES_STATE_9:
					switch (StateSelectCfg)
					{
						case 1:
						case 3:
							Set_Current_State(hWnd, LOWORD(wParam) - ID_FILES_STATE_0);
							if (!NumLoadEnabled) return 0; //Nitsuja added this
							if (Check_If_Kaillera_Running()) return 0;
							Str_Tmp[0] = 0;
							Get_State_File_Name(Str_Tmp);
							Save_State(Str_Tmp);
							return 0;
						case 4:
						case 5:
							Set_Current_State(hWnd, LOWORD(wParam) - ID_FILES_STATE_0);
							if (!NumLoadEnabled) return 0; //Nitsuja added this
							if (Check_If_Kaillera_Running()) return 0;
							Str_Tmp[0] = 0;
							Get_State_File_Name(Str_Tmp);
							Load_State(Str_Tmp);
							return 0;
						default:	//cases 0, 2 or undefined
							Set_Current_State(hWnd, LOWORD(wParam) - ID_FILES_STATE_0);
							return 0;
					}


				case ID_FILES_CTRLSTATE_0: //Modif N - for new loadstate# keys:
				case ID_FILES_CTRLSTATE_1:
				case ID_FILES_CTRLSTATE_2:
				case ID_FILES_CTRLSTATE_3:
				case ID_FILES_CTRLSTATE_4:
				case ID_FILES_CTRLSTATE_5:
				case ID_FILES_CTRLSTATE_6:
				case ID_FILES_CTRLSTATE_7:
				case ID_FILES_CTRLSTATE_8:
				case ID_FILES_CTRLSTATE_9:
					switch (StateSelectCfg)
					{
						case 3:
						case 5:
							Set_Current_State(hWnd, LOWORD(wParam) - ID_FILES_CTRLSTATE_0);
							return 0;
						case 2:
						case 4:
							Set_Current_State(hWnd, LOWORD(wParam) - ID_FILES_CTRLSTATE_0);
							if (!NumLoadEnabled) return 0; //Nitsuja added this
							if (Check_If_Kaillera_Running()) return 0;
							Str_Tmp[0] = 0;
							Get_State_File_Name(Str_Tmp);
							Save_State(Str_Tmp);
							return 0;
						default:	//cases 0, 1, or undefined
							Set_Current_State(hWnd, LOWORD(wParam) - ID_FILES_CTRLSTATE_0);
							if (!NumLoadEnabled) return 0; //Nitsuja added this
							if (Check_If_Kaillera_Running()) return 0;
							Str_Tmp[0] = 0;
							Get_State_File_Name(Str_Tmp);
							Load_State(Str_Tmp);
							return 0;
					}

				case ID_FILES_SFTSTATE_0: //Modif N - for new savestate# keys:
				case ID_FILES_SFTSTATE_1:
				case ID_FILES_SFTSTATE_2:
				case ID_FILES_SFTSTATE_3:
				case ID_FILES_SFTSTATE_4:
				case ID_FILES_SFTSTATE_5:
				case ID_FILES_SFTSTATE_6:
				case ID_FILES_SFTSTATE_7:
				case ID_FILES_SFTSTATE_8:
				case ID_FILES_SFTSTATE_9:
					switch (StateSelectCfg)
					{
						case 1:
						case 4:
							Set_Current_State(hWnd, LOWORD(wParam) - ID_FILES_STATE_0);
							return 0;
						case 2:
						case 3:
							Set_Current_State(hWnd, LOWORD(wParam) - ID_FILES_SFTSTATE_0);
							if (!NumLoadEnabled) return 0; //Nitsuja added this
							if (Check_If_Kaillera_Running()) return 0;
							Str_Tmp[0] = 0;
							Get_State_File_Name(Str_Tmp);
							Load_State(Str_Tmp);
							return 0;
						default:	//cases 0, 5 or undefined
							Set_Current_State(hWnd, LOWORD(wParam) - ID_FILES_SFTSTATE_0);
							if (!NumLoadEnabled) return 0; //Nitsuja added this
							if (Check_If_Kaillera_Running()) return 0;
							Str_Tmp[0] = 0;
							Get_State_File_Name(Str_Tmp);
							Save_State(Str_Tmp);
							return 0;
					}
				case ID_MOVIE_CHANGETRACK_ALL:
					track = 1 | 2 | 4;
					Put_Info("Recording all tracks",1000);
					if (!MainMovie.TriplePlayerHack) track &= 3;
					return 0;
				case ID_MOVIE_CHANGETRACK_1:
				case ID_MOVIE_CHANGETRACK_2:
				case ID_MOVIE_CHANGETRACK_3:
				{
					int chgtrack = LOWORD(wParam) - ID_MOVIE_CHANGETRACK_ALL;
					track ^= chgtrack;
					sprintf(Str_Tmp,"Recording player %d %sed",min(chgtrack,3),(track & chgtrack)?"start":"end");
					Put_Info(Str_Tmp,1000);
					if (!MainMovie.TriplePlayerHack) track &= 3;
					return 0;
				}
				case ID_PREV_TRACK:
				{
					int maxtrack = TRACK1 | TRACK2;
					if (MainMovie.TriplePlayerHack) maxtrack |= TRACK3;
					track &= maxtrack;
					if (track == maxtrack)
						track = 2;
					else 
					{
						track >>=1;
						if (!track) track = maxtrack;
					}
					if (track == maxtrack) sprintf(Str_Tmp,"Recording all players.");
					else sprintf(Str_Tmp,"Recording player %d.",min(track,3));
					Put_Info(Str_Tmp,1000);
				}
				break;
				case ID_NEXT_TRACK:
				{
					int maxtrack = TRACK1 | TRACK2;
					if (MainMovie.TriplePlayerHack) maxtrack |= TRACK3;
					track &= maxtrack;
					if (track == maxtrack)
						track = 1;
					else 
					{
						track <<=1;
						track &= maxtrack;
						if (!track) track = maxtrack;
					}
					if (track == maxtrack) sprintf(Str_Tmp,"Recording all players.");
					else sprintf(Str_Tmp,"Recording player %d.",min(track,3));
					Put_Info(Str_Tmp,1000);
				}
				break;
#ifdef GENS_DEBUG
				case ID_CPU_DEBUG_GENESIS_68000:
					Change_Debug(hWnd, 1);
					return 0;

				case ID_CPU_DEBUG_GENESIS_Z80:
					Change_Debug(hWnd, 2);
					return 0;

				case ID_CPU_DEBUG_GENESIS_VDP:
					Change_Debug(hWnd, 3);
					return 0;

				case ID_CPU_DEBUG_SEGACD_68000:
					Change_Debug(hWnd, 4);
					return 0;

				case ID_CPU_DEBUG_SEGACD_CDC:
					Change_Debug(hWnd, 5);
					return 0;

				case ID_CPU_DEBUG_SEGACD_GFX:
					Change_Debug(hWnd, 6);
					return 0;

				case ID_CPU_DEBUG_32X_MAINSH2:
					Change_Debug(hWnd, 7);
					return 0;

				case ID_CPU_DEBUG_32X_SUBSH2:
					Change_Debug(hWnd, 8);
					return 0;

				case ID_CPU_DEBUG_32X_VDP:
					Change_Debug(hWnd, 9);
					return 0;
#endif
				case ID_LAG_RESET:
					LagCount = 0;
					return 0;

				case ID_CPU_RESET:
					if(!(Game))
						return 0;
					if((MainMovie.File!=NULL) && (AutoCloseMovie)) //Upth-Modif - So movie close on reset is optional
					{
						CloseMovieFile(&MainMovie);
						MainMovie.Status=0;
					}
					if((MainMovie.Status == MOVIE_RECORDING) && (!(AutoCloseMovie)) && (MainMovie.ReadOnly)) //Upth-Add - on reset, switch movie from recording
						MainMovie.Status = MOVIE_PLAYING; //Upth-Add - To playing, if read only has been toggled on

					if (Check_If_Kaillera_Running()) return 0;

					if (Genesis_Started)
						Reset_Genesis();
					else if (_32X_Started)
						Reset_32X();
					else if (SegaCD_Started)
						Reset_SegaCD();

					FrameCount=0;
					LagCount = 0;

					if (Genesis_Started)
						MESSAGE_L("Genesis reseted", "Genesis reset", 1500)
					else if (_32X_Started)
						MESSAGE_L("32X reseted", "32X reset", 1500)
					else if (SegaCD_Started)
						MESSAGE_L("SegaCD reseted", "SegaCD reset", 1500)

					return 0;

				case ID_CPU_RESET68K:
					if(!(Game))
						return 0;
					if(MainMovie.File!=NULL)
						CloseMovieFile(&MainMovie);
					MainMovie.Status=0;

					if (Check_If_Kaillera_Running()) return 0;
					if (Game)
					{
						Paused = 0;
						main68k_reset();
						if (Genesis_Started) MESSAGE_L("68000 CPU reseted", "68000 CPU reseted", 1000)
						else if (SegaCD_Started) MESSAGE_L("Main 68000 CPU reseted", "Main 68000 CPU reseted", 1000)
					}
					FrameCount=0;
					LagCount = 0;
					return 0;

				case ID_CPU_RESET_MSH2:
					if(!(Game))
						return 0;
					if(MainMovie.File!=NULL)
						CloseMovieFile(&MainMovie);
					MainMovie.Status=0;

					if (Check_If_Kaillera_Running()) return 0;
					if ((Game) && (_32X_Started))
					{
						Paused = 0;
						SH2_Reset(&M_SH2, 1);
						MESSAGE_L("Master SH2 reseted", "Master SH2 reseted", 1000)
					}
					FrameCount=0;
					LagCount = 0;
					return 0;

				case ID_CPU_RESET_SSH2:
					if(!(Game))
						return 0;
					if(MainMovie.File!=NULL)
						CloseMovieFile(&MainMovie);
					MainMovie.Status=0;

					if (Check_If_Kaillera_Running()) return 0;
					if ((Game) && (_32X_Started))
					{
						Paused = 0;
						SH2_Reset(&S_SH2, 1);
						MESSAGE_L("Slave SH2 reseted", "Slave SH2 reseted", 1000)
					}
					FrameCount=0;
					LagCount = 0;
					return 0;

				case ID_CPU_RESET_SUB68K:
					if(!(Game))
						return 0;
					if(MainMovie.File!=NULL)
						CloseMovieFile(&MainMovie);
					MainMovie.Status=0;

					if (Check_If_Kaillera_Running()) return 0;
					if ((Game) && (SegaCD_Started))
					{
						Paused = 0;
						sub68k_reset();
						MESSAGE_L("Sub 68000 CPU reseted", "Sub 68000 CPU reseted", 1000)
					}
					FrameCount=0;
					LagCount = 0;
					return 0;

				case ID_CPU_RESETZ80:
					if(!(Game))
						return 0;
					if(MainMovie.File!=NULL)
						CloseMovieFile(&MainMovie);
					MainMovie.Status=0;

					if (Check_If_Kaillera_Running()) return 0;
					if (Game)
					{
						z80_Reset(&M_Z80);
						MESSAGE_L("CPU Z80 reseted", "CPU Z80 reseted", 1000)
					}
					FrameCount=0;
					LagCount = 0;
					return 0;

				case ID_CPU_ACCURATE_SYNCHRO:
					Change_SegaCD_Synchro();
					return 0;

				case ID_CPU_COUNTRY_AUTO:
					Change_Country(hWnd, -1);
					return 0;

				case ID_CPU_COUNTRY_JAPAN:
					Change_Country(hWnd, 0);
					return 0;

				case ID_CPU_COUNTRY_USA:
					Change_Country(hWnd, 1);
					return 0;

				case ID_CPU_COUNTRY_EUROPE:
					Change_Country(hWnd, 2);
					return 0;

				case ID_CPU_COUNTRY_MISC:
					Change_Country(hWnd, 3);
					return 0;

				case ID_CPU_COUNTRY_ORDER + 0:
				case ID_CPU_COUNTRY_ORDER + 1:
				case ID_CPU_COUNTRY_ORDER + 2:
					Change_Country_Order(LOWORD(wParam) - ID_CPU_COUNTRY_ORDER);
					return 0;

				case ID_SOUND_Z80ENABLE:
					Change_Z80(hWnd);
					return 0;

				case ID_SOUND_YM2612ENABLE:
					Change_YM2612(hWnd);
					return 0;

				case ID_SOUND_PSGENABLE:
					Change_PSG(hWnd);
					return 0;

				case ID_SOUND_DACENABLE:
					Change_DAC(hWnd);
					return 0;

				case ID_SOUND_PCMENABLE:
					Change_PCM(hWnd);
					return 0;

				case ID_SOUND_PWMENABLE:
					Change_PWM(hWnd);
					return 0;

				case ID_SOUND_CDDAENABLE:
					Change_CDDA(hWnd);
					return 0;

				case ID_SOUND_DACIMPROV:
					Change_DAC_Improv(hWnd);
					return 0;

				case ID_SOUND_PSGIMPROV:
					Change_PSG_Improv(hWnd);
					return 0;

				case ID_SOUND_YMIMPROV:
					Change_YM2612_Improv(hWnd);
					return 0;

				case ID_SOUND_ENABLE:
					Change_Sound(hWnd);
					return 0;

				case ID_SOUND_RATE_11000:
					Change_Sample_Rate(hWnd, 0);
					return 0;

				case ID_SOUND_RATE_22000:
					Change_Sample_Rate(hWnd, 1);
					return 0;

				case ID_SOUND_RATE_44000:
					Change_Sample_Rate(hWnd, 2);
					return 0;

				case ID_SOUND_STEREO:
					Change_Sound_Stereo(hWnd);
					return 0;

				case ID_SOUND_SOFTEN: //Nitsuja added this
					Change_Sound_Soften(hWnd);
					return 0;

				case ID_SOUND_HOG: //Nitsuja added this
					Change_Sound_Hog(hWnd);
					return 0;

				case ID_SOUND_STARTWAVDUMP:
					if (WAV_Dumping) Stop_WAV_Dump();
					else Start_WAV_Dump();
					Build_Main_Menu();
					return 0;

				case ID_SOUND_STARTGYMDUMP:
					if (GYM_Dumping) Stop_GYM_Dump();
					else Start_GYM_Dump();
					Build_Main_Menu();
					return 0;

				case ID_SOUND_PLAYGYM:
					MINIMIZE
					if (!Genesis_Started && !SegaCD_Started && !_32X_Started)
					{
						if (GYM_Playing) Stop_Play_GYM();
						else Start_Play_GYM();
					}
					Build_Main_Menu();
					return 0;

				case ID_OPTIONS_FASTBLUR:
					Change_Fast_Blur(hWnd);
					return 0;

				case ID_OPTIONS_SHOWFPS:
					if (Show_FPS) Show_FPS = 0;
					else Show_FPS = 1;
					return 0;

				case ID_OPTIONS_GENERAL:
					if (Check_If_Kaillera_Running()) return 0;
					MINIMIZE
					DialogsOpen++;
					DialogBox(ghInstance, MAKEINTRESOURCE(IDD_OPTION), hWnd, (DLGPROC) OptionProc);
					Build_Main_Menu();
					return 0;

				case ID_OPTIONS_JOYPADSETTING:
					if (Check_If_Kaillera_Running()) return 0;
					MINIMIZE
					End_Input();
					DialogsOpen++;
					DialogBox(ghInstance, MAKEINTRESOURCE(IDD_CONTROLLER), hWnd, (DLGPROC) ControllerProc);
					if (!Init_Input(ghInstance, HWnd)) return false;
					Build_Main_Menu();
					return 0;

				case ID_OPTIONS_CHANGEDIR:
					if (Check_If_Kaillera_Running()) return 0;
					MINIMIZE
					DialogsOpen++;
					DialogBox(ghInstance, MAKEINTRESOURCE(IDD_DIRECTORIES), hWnd, (DLGPROC) DirectoriesProc);
					Build_Main_Menu();
					return 0;

				case ID_OPTIONS_CHANGEFILES:
					if (Check_If_Kaillera_Running()) return 0;
					MINIMIZE
					DialogsOpen++;
					DialogBox(ghInstance, MAKEINTRESOURCE(IDD_FILES), hWnd, (DLGPROC) FilesProc);
					Build_Main_Menu();
					return 0;

				case ID_OPTION_CDDRIVE_0:
				case ID_OPTION_CDDRIVE_1:
				case ID_OPTION_CDDRIVE_2:
				case ID_OPTION_CDDRIVE_3:
				case ID_OPTION_CDDRIVE_4:
				case ID_OPTION_CDDRIVE_5:
				case ID_OPTION_CDDRIVE_6:
				case ID_OPTION_CDDRIVE_7:
					if (Num_CD_Drive > (LOWORD(wParam) - ID_OPTION_CDDRIVE_0))
					{
						CUR_DEV = LOWORD(wParam) - ID_OPTION_CDDRIVE_0;
					}
					Build_Main_Menu();
					return 0;

				case ID_OPTION_SRAMSIZE_0:
					Change_SegaCD_SRAM_Size(-1);
					return 0;

				case ID_OPTION_SRAMSIZE_8:
					Change_SegaCD_SRAM_Size(0);
					return 0;

				case ID_OPTION_SRAMSIZE_16:
					Change_SegaCD_SRAM_Size(1);
					return 0;

				case ID_OPTION_SRAMSIZE_32:
					Change_SegaCD_SRAM_Size(2);
					return 0;

				case ID_OPTION_SRAMSIZE_64:
					Change_SegaCD_SRAM_Size(3);
					return 0;

				case ID_OPTIONS_SAVECONFIG:
					strcpy(Str_Tmp, Gens_Path);
					strcat(Str_Tmp, "Gens.cfg");
					Save_Config(Str_Tmp);
					return 0;

				case ID_OPTIONS_LOADCONFIG:
					if (Check_If_Kaillera_Running()) return 0;
					MINIMIZE
					DialogsOpen++;
					Load_As_Config(hWnd, Game);
					DialogsOpen--;
					return 0;

				case ID_OPTIONS_SAVEASCONFIG:
					MINIMIZE
					DialogsOpen++;
					Save_As_Config(hWnd);
					DialogsOpen--;
					return 0;

				case ID_HELP_ABOUT:
					Clear_Sound_Buffer();
					DialogsOpen++;
					DialogBox(ghInstance, MAKEINTRESOURCE(ABOUTDIAL), hWnd, (DLGPROC) AboutProc);
					return 0;

				case ID_HELP_HELP:
					if (Game)
					{
						if (Genesis_Started)
						{
							HtmlHelp(GetDesktopWindow(), CGOffline_Path, HH_HELP_CONTEXT, Calculate_CRC32());
						}
					}
					else
					{
						if (Detect_Format(Manual_Path) != -1)		// Can be used to test if file exist
						{
							strcpy(Str_Tmp, Manual_Path);
							strcat(Str_Tmp, " index.html");
							system(Str_Tmp);
						}
					}

					/* Gens manual file :
					**
					**	File menu						manual.exe helpfilemenu.html
					**	Graphics menu					manual.exe helpgraphicsmenu.html
					**	CPU menu						manual.exe helpcpumenu.html
					**	Sound menu						manual.exe helpsoundmenu.html
					**	Options menu					manual.exe helpoptionmenu.html
					**	Netplay							manual.exe helpnetplay.html
					**
					**	Game Genie						manual.exe helpgamegenie.html
					**	Misc/General options			manual.exe helpmisc.html
					**	Joypad/Controllers settings		manual.exe helpjoypads.html
					**	Directories/file configuration	manual.exe helpdir.html
					**
					**	Help menu						manual.exe helphelpmenu.html
					**	Mega-CD                         manual.exe helpmegacd.html
					**	FAQ                             manual.exe helpfaq.html
					**	Default Keys/keyboard shortcuts manual.exe helpkeys.html
					**	Multitap                        manual.exe helpmultitap.html
					*/
					return 0;

				case ID_HELP_MENU_FILE:
					if (Detect_Format(Manual_Path) != -1)		// Can be used to test if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpfilemenu.html");
						system(Str_Tmp);
					}
					return 0;

				case ID_HELP_MENU_GRAPHICS:
					if (Detect_Format(Manual_Path) != -1)		// Can be used to test if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpgraphicsmenu.html");
						system(Str_Tmp);
					}
					return 0;

				case ID_HELP_MENU_CPU:
					if (Detect_Format(Manual_Path) != -1)		// Can be used to test if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpcpumenu.html");
						system(Str_Tmp);
					}
					return 0;

				case ID_HELP_MENU_SOUND:
					if (Detect_Format(Manual_Path) != -1)		// Can be used to test if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpsoundmenu.html");
						system(Str_Tmp);
					}
					return 0;

				case ID_HELP_MENU_OPTIONS:
					if (Detect_Format(Manual_Path) != -1)		// Can be used to test if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpoptionmenu.html");
						system(Str_Tmp);
					}
					return 0;

				case ID_HELP_NETPLAY:
					if (Detect_Format(Manual_Path) != -1)		// Can be used to test if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpnetplay.html");
						system(Str_Tmp);
					}
					return 0;

				case ID_HELP_MEGACD:
					if (Detect_Format(Manual_Path) != -1)		// Can be used to test if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpmegacd.html");
						system(Str_Tmp);
					}
					return 0;

				case ID_HELP_FAQ:
					if (Detect_Format(Manual_Path) != -1)		// Can be used to test if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpfaq.html");
						system(Str_Tmp);
					}
					return 0;

				case ID_HELP_KEYS:
					if (Detect_Format(Manual_Path) != -1)		// Can be used to test if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpkeys.html");
						system(Str_Tmp);
					}
					return 0;
				case ID_CHANGE_CLEANAVI:
					CleanAvi = !CleanAvi;
					Build_Main_Menu();
					return 0;
				case ID_CHANGE_PALLOCK:
				{
					PalLock = !PalLock;
					Build_Main_Menu();
					char message [256];
					sprintf(message, "Palette %sed", PalLock?"lock":"unlock");
					MESSAGE_L(message, message, 1000)

					return 0;
				}

				case ID_CHANGE_TRACE:
				{
					trace_map = !trace_map;
					Build_Main_Menu();
					if (trace_map)
					{
						if( !fp_trace )
						{
							fp_trace = fopen( "trace.log", "a" );
							mapped = new char[ 0x100*0x10000 ];
							memset( mapped,0,0x100*0x10000 );
//							fseek(fp_trace,0,SEEK_END);
							fprintf(fp_trace,"TRACE STARTED\n\n");
						}
						if (SegaCD_Started && !fp_trace_cd)
						{
							fp_trace_cd = fopen( "trace_cd.log", "a" );
							mapped_cd = new char[ 0x100*0x10000 ];
							memset( mapped_cd,0,0x100*0x10000 );
//							fseek(fp_trace_cd,0,SEEK_END);
							fprintf(fp_trace_cd,"TRACE STARTED\n\n");
						}
					}
					else
					{
						if( fp_trace )
						{
							fprintf(fp_trace,"\nTRACE STOPPED\n\n");
							fclose(fp_trace);
							delete [] (mapped);
							fp_trace = NULL;
						}
						if ( fp_trace_cd )
						{
							fprintf(fp_trace_cd,"\nTRACE STOPPED\n\n");
							fclose(fp_trace_cd);
							delete [] (mapped_cd);
							fp_trace_cd = NULL;
						}
					}


					char message [256];
					sprintf(message, "Instruction logging %sed", trace_map?"start":"end");
					MESSAGE_L(message, message, 1000)

					return 0;
				}

				case ID_CHANGE_HOOK:
				{
					hook_trace = !hook_trace;
					Build_Main_Menu();
					if (hook_trace)
					{
						if( !fp_hook )
						{
							fp_hook = fopen( "hook.txt", "a" );
							fseek(fp_hook,0,SEEK_END);
						}
						fprintf(fp_hook,"MEMORY ACCESS LOGGING STARTED\n\n");
						if (SegaCD_Started) 
						{
							if (!fp_hook_cd) 
							{
								fp_hook_cd = fopen( "hook_cd.txt", "a" );
								fseek(fp_hook_cd,0,SEEK_END);
							}
							fprintf(fp_hook_cd,"MEMORY ACCESS LOGGING STARTED\n\n");
						}
					}
					else
					{
						if(fp_hook && (fp_hook != fp_trace))
						{
							fprintf(fp_hook,"\nMEMORY ACCESS LOGGING STOPPED\n\n");
							fclose(fp_hook);
							fp_hook = NULL;
						}
						if (fp_hook_cd && (fp_hook_cd != fp_trace_cd))
						{
							fprintf(fp_hook_cd,"\nMEMORY ACCESS LOGGING STOPPED\n\n");
							fclose(fp_hook_cd);
							fp_hook_cd = NULL;
						}
					}
					
					char message [256];
					sprintf(message, "RAM logging %sed", hook_trace?"start":"end");
					MESSAGE_L(message, message, 1000)

					return 0;
				}

				case ID_EMULATION_PAUSED:
					if (Debug)
					{
						Change_Debug(HWnd, 0);
						Paused = 0;
						Build_Main_Menu();
					}
					else if (Paused)
					{
						Paused = 0;
					}
					else
					{
						Paused = 1;
						Pause_Screen();
						Clear_Sound_Buffer();
						Flip(HWnd);
					}
					return 0;
			}
			break;

/*
			// A device has be modified (new CD inserted for instance)
			case WM_DEVICECHANGE:
				ASPI_Mechanism_State(0, NULL);
				break;
*/

#ifdef GENS_DEBUG
		case WM_KEYDOWN:
			if (Debug) Debug_Event((lParam >> 16) & 0x7F);
			break;
#endif

		case WM_KNUX:
			MESSAGE_L("Communicating", "Communicating ...", 1000)

			switch(wParam)
			{
				case 0:
					switch(lParam)
					{
						case 0:
							return 4;

						case 1:
							GetWindowText(HWnd, Str_Tmp, 1024);
							return (long) (char *) Str_Tmp;

						case 2:
							return 5;

						case 3:
							return GENS_VERSION_H;

						default:
							return -1;
					}

				case 1:
					switch(lParam)
					{
						case 0:
							return((long) (unsigned short *)&Ram_68k[0]);
						case 1:
							return(64 * 1024);
						case 2:
							return(1);
						default:
							return(-1);
					}

				case 2:
					switch(lParam)
					{
						case 0:
							return((long) (unsigned char *)&Ram_Z80[0]);
						case 1:
							return(8 * 1024);
						case 2:
							return(0);
						default:
							return(-1);
					}

				case 3:
					switch(lParam)
					{
						case 0:
							return((long) (char *)&Rom_Data[0]);
						case 1:
							return(0);
						case 2:
							return(Rom_Size);
						default:
							return(-1);
					}

				case 4:
					switch(lParam)
					{
						case 0:
							return(0);
						case 1:
							return((Game != NULL)?1:0);
						case 2:
							return(0);
						default:
							return(-1);
					}

				default:
					return(-1);
			}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}


int Build_Language_String(void)
{
	unsigned long nb_lue = 1;
	int allocated_sections = 1, poscar = 0;
	enum etat_sec {DEB_LIGNE, SECTION, NORMAL} state = DEB_LIGNE;
	HANDLE LFile;
	char c;

	if (language_name)
	{
		int i = 0;
		while(language_name[i])
			free(language_name[i++]);
		free(language_name);
		language_name = NULL;
	}

	language_name = (char**)malloc(allocated_sections * sizeof(char*));
	language_name[0] = NULL;

	LFile = CreateFile(Language_Path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
 	
	while(nb_lue)
	{
		ReadFile(LFile, &c, 1, &nb_lue, NULL);
		
		switch(state)
		{
			case DEB_LIGNE:
				switch(c)
				{
					case '[':
						state = SECTION;
						allocated_sections++;
						language_name = (char**)realloc(language_name, allocated_sections * sizeof(char*));
						language_name[allocated_sections - 2] = (char*)malloc(32 * sizeof(char));
						language_name[allocated_sections - 1] = NULL;
						poscar = 0;
						break;

					case '\n':
						break;

					default: state = NORMAL;
						break;
				}
				break;

			case NORMAL:
				switch(c)
				{
					case '\n':
						state = DEB_LIGNE;
						break;

					default:
						break;
				}
				break;

			case SECTION:
				switch(c)
				{
					case ']':
						language_name[allocated_sections - 2][poscar] = 0;
						state = DEB_LIGNE;
						break;

					default:
						if(poscar < 32)
							language_name[allocated_sections - 2][poscar++] = c;
						break;
				}
				break;
		}
	}

	CloseHandle(LFile);

	if (allocated_sections == 1)
	{
		language_name = (char**)realloc(language_name, 2 * sizeof(char*));
		language_name[0] = (char*)malloc(32 * sizeof(char));
		strcpy(language_name[0], "English");
		language_name[1] = NULL;
		WritePrivateProfileString("English", "Menu Language", "&English menu", Language_Path);
	}

	return(0);	
}


HMENU Build_Main_Menu(void)
{
	unsigned int Flags;
	int i, Rend;

	HMENU MainMenu;
	HMENU Files;
	HMENU Graphics;
	HMENU CPU;
	HMENU Sound;
	HMENU TAS_Tools; //Upth-Add - For the new menu which contains all the TAS stuff
	HMENU Options;
	HMENU Help;

	HMENU FilesChangeState;
	HMENU FilesHistory;
	HMENU GraphicsRender;
	HMENU GraphicsLayers; //Nitsuja added this
	HMENU GraphicsLayersA;
	HMENU GraphicsLayersB;
	HMENU GraphicsLayersS;
	HMENU GraphicsFrameSkip;
#ifdef GENS_DEBUG
	HMENU CPUDebug;
#endif
	HMENU CPUCountry;
	HMENU CPUCountryOrder;
	HMENU CPUSlowDownSpeed;
	HMENU SoundRate;
	HMENU OptionsCDDrive;
	HMENU OptionsSRAMSize;
	HMENU Tools_Movies; //Upth-Add - Submenu of TAS_Tools
	HMENU Movies_Tracks; //Upth-Add - submenu of Tas_Tools -> Tools_Movies
	HMENU Tools_AVI;    //Upth-Add - Submenu of TAS_Tools
	HMENU Tools_Trace;    //Upth-Add - Submenu of TAS_Tools

	DestroyMenu(Gens_Menu);

	Build_Language_String();

	if (Full_Screen)
	{
		MainMenu = CreatePopupMenu();
		Rend = Render_FS;
	}
	else
	{
		MainMenu = CreateMenu();
		Rend = Render_W;
	}

	Files = CreatePopupMenu();
	Graphics = CreatePopupMenu();
	CPU = CreatePopupMenu();
	Sound = CreatePopupMenu();
	Options = CreatePopupMenu();
	TAS_Tools = CreatePopupMenu(); //Upth-Add - Initialize my new menus
	Help = CreatePopupMenu();
	FilesChangeState = CreatePopupMenu();
	FilesHistory = CreatePopupMenu();
	GraphicsRender = CreatePopupMenu();
	GraphicsLayers = CreatePopupMenu(); //Nitsuja added this
	GraphicsLayersA = CreatePopupMenu();
	GraphicsLayersB = CreatePopupMenu();
	GraphicsLayersS = CreatePopupMenu();
	GraphicsFrameSkip = CreatePopupMenu();
#ifdef GENS_DEBUG
	CPUDebug = CreatePopupMenu();
#endif
	CPUCountry = CreatePopupMenu();
	CPUCountryOrder = CreatePopupMenu();
	CPUSlowDownSpeed = CreatePopupMenu();
	SoundRate = CreatePopupMenu();
	OptionsCDDrive = CreatePopupMenu();
	OptionsSRAMSize = CreatePopupMenu();
	Tools_Movies = CreatePopupMenu(); //Upth-Add - Initialize my new menus
	Movies_Tracks = CreatePopupMenu(); //Upth-Add - Initialize new menu
	Tools_AVI = CreatePopupMenu(); //Upth-Add - Initialize my new menus
	Tools_Trace = CreatePopupMenu(); //Upth-Add - Initialize my new menus

	// Cr�ation des sous-menu pricipaux

	Flags = MF_BYPOSITION | MF_POPUP | MF_STRING;

	MENU_L(MainMenu, 0, Flags, (UINT)Files, "File", "", "&File");
	MENU_L(MainMenu, 1, Flags, (UINT)Graphics, "Graphic", "", "&Graphic");
	MENU_L(MainMenu, 2, Flags, (UINT)CPU, "CPU", "", "&CPU");
	MENU_L(MainMenu, 3, Flags, (UINT)Sound, "Sound", "", "&Sound");
	MENU_L(MainMenu, 4, Flags, (UINT)TAS_Tools, "Tools", "", "&Tools"); //Upth-Add - Put the new menu in between sound and options // Nitsuja: changed TAS Tools to Tools to prevent extra-wide menu in normal render mode, and because spaces in menu titles can be confusing
	MENU_L(MainMenu, 5, Flags, (UINT)Options, "Option", "", "&Option"); //Upth-Modif - this now goes in one later
	MENU_L(MainMenu, 6, Flags, (UINT)Help, "Help", "", "&Help"); //Upth-Modif - this now goes in one later

	// Menu Files 
	
	Flags = MF_BYPOSITION | MF_STRING;
	
	MENU_L(Files, 0, Flags, ID_FILES_OPENROM, "Open Rom", "\tCtrl+O", "&Open ROM");
	MENU_L(Files, 1, Flags, ID_FILES_CLOSEROM, "Free Rom", "\tCtrl+C", "&Close ROM");

	i = 2;

	MENU_L(Files, i++, Flags, ID_FILES_BOOTCD, "Boot CD", "\tCtrl+B", "&Boot CD");

	if (Kaillera_Initialised)
	{
		MENU_L(Files, i++, Flags, ID_FILES_NETPLAY, "Netplay", "", "&Netplay");
	}
	
	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);

	MENU_L(Files, i++, Flags, ID_FILES_GAMEGENIE, "Game Genie", "", "&Game Genie");
	
	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	
	MENU_L(Files, i++, Flags, ID_FILES_LOADSTATEAS, "Load State as", "\tShift+F8", "&Load State ...");
	MENU_L(Files, i++, Flags, ID_FILES_SAVESTATEAS, "Save State as", "\tShift+F5", "&Save State as...");
	MENU_L(Files, i++, Flags, ID_FILES_LOADSTATE, "Load State", "\tF8", "Quick &Load");
	MENU_L(Files, i++, Flags, ID_FILES_SAVESTATE, "Save State", "\tF5", "Quick &Save");
	MENU_L(Files, i++, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)FilesChangeState, "Change State", "\tF6-F7", "C&hange State");

	InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);

	if (strcmp(Recent_Rom[0], ""))
	{
		MENU_L(Files, i++, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)FilesHistory, "Rom History", "", "&ROM History");
		InsertMenu(Files, i++, MF_SEPARATOR, NULL, NULL);
	}

	MENU_L(Files, i++, Flags, ID_FILES_QUIT, "Quit", "", "&Quit");



	// Menu FilesChangeState
	
	for(i = 0; i < 10; i++)
	{
		wsprintf(Str_Tmp ,"&%d", i);

		if(Current_State == i)
			InsertMenu(FilesChangeState, i, Flags | MF_CHECKED, ID_FILES_STATE_0 + i, Str_Tmp);
		else
			InsertMenu(FilesChangeState, i, Flags | MF_UNCHECKED, ID_FILES_STATE_0 + i, Str_Tmp);
	}


	// Menu FilesHistory
	
	for(i = 0; i < 15; i++)
	{
		if (strcmp(Recent_Rom[i], ""))
		{
			char tmp[1024];

			switch (Detect_Format(Recent_Rom[i]) >> 1)			// do not exist anymore
			{
				default:
					strcpy(tmp, "[---]\t- ");
					break;

				case 1:
					strcpy(tmp, "[MD]\t- ");
					break;

				case 2:
					strcpy(tmp, "[32X]\t- ");
					break;

				case 3:
					strcpy(tmp, "[SCD]\t- ");
					break;

				case 4:
					strcpy(tmp, "[SCDX]\t- ");
					break;
			}

			Get_Name_From_Path(Recent_Rom[i], Str_Tmp);
			strcat(tmp, Str_Tmp);
			InsertMenu(FilesHistory, i, Flags, ID_FILES_OPENRECENTROM0 + i, tmp);

		}
		else break;
	}

	
	// Menu Graphics

	Flags = MF_BYPOSITION | MF_STRING;
	
	i = 0; //In this next section Nitsuja and I simplified the menu generation code greatly through consistent use of "i" and the trinary operator.

	if (Full_Screen)
	{
		MENU_L(Graphics, i++, Flags, ID_GRAPHICS_SWITCH_MODE, "Windowed", "\tAlt+Enter", "&Windowed");
	}
	else
	{
		MENU_L(Graphics, i++, Flags, ID_GRAPHICS_SWITCH_MODE, "Full Screen", "\tAlt+Enter", "&Full Screen");
	}

	MENU_L(Graphics, i++, Flags | (((Full_Screen && FS_VSync) || (!Full_Screen && W_VSync)) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_VSYNC, "VSync", "\tShift+F3", "&VSync");

	if (Full_Screen && !FS_No_Res_Change && (Render_FS > 1))
	{
		MENU_L(Graphics, i++, Flags | MF_UNCHECKED | MF_GRAYED, ID_GRAPHICS_STRETCH, "Stretch", "\tShift+F2", "&Stretch");
	}
	else
	{
		MENU_L(Graphics, i++, Flags | (Stretch ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_STRETCH, "Stretch", "\tShift+F2", "&Stretch");
	}

	MENU_L(Graphics, i++, Flags |(FS_No_Res_Change ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_FS_SAME_RES, "FS_Windowed", "", "&Windowed Fullscreen"); // UpthAdd // Modif N: removed reference to VBA

	MENU_L(Graphics, i++, Flags, ID_GRAPHICS_COLOR_ADJUST, "Color", "", "&Color Adjust...");
	MENU_L(Graphics, i++, Flags | MF_POPUP, (UINT)GraphicsRender, "Render", "", "&Render");
	InsertMenu(Graphics, i++, MF_SEPARATOR, NULL, NULL);

	MENU_L(Graphics, i++, Flags | MF_POPUP, (UINT)GraphicsLayers, "Layers", "", "&Layers");
	MENU_L(Graphics, i++, Flags | (PalLock ? MF_CHECKED : MF_UNCHECKED), ID_CHANGE_PALLOCK, "Lock Palette", "", "Lock &Palette");
	MENU_L(Graphics, i++, Flags | (Sprite_Over ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_SPRITEOVER, "Sprite Limit", "", "&Sprite Limit");

	InsertMenu(Graphics, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(Graphics, i++, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)GraphicsFrameSkip, "Frame Skip", "", "&Frame Skip");
	MENU_L(Graphics, i++, Flags | (Never_Skip_Frame ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_NEVER_SKIP_FRAME, "Never skip frame with auto frameskip", "", "&Never skip frame with auto frameskip");
	
	InsertMenu(Graphics, i++, MF_MENUBARBREAK, NULL, NULL);
	MENU_L(Graphics, i++, Flags | MF_UNCHECKED, ID_GRAPHICS_SHOT, "Screen Shot", "\tShift+Backspc", "&Screen Shot");
	
	//InsertMenu(Graphics, 12, MF_SEPARATOR, NULL, NULL);

	// Menu GraphicsRender

	i = 0;
	MENU_L(GraphicsRender, i++, MF_BYPOSITION | MF_STRING | ((Rend == 0) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_RENDER_NORMAL, "Normal", "", "&Normal");
	MENU_L(GraphicsRender, i++, MF_BYPOSITION | MF_STRING | ((Rend == 1) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_RENDER_DOUBLE, "Double", "", "&Double");
	MENU_L(GraphicsRender, i++, MF_BYPOSITION | ((Rend == 2) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_RENDER_EPX, "EPX", "", "&EPX"); //Modif N.

	MENU_L(GraphicsRender, i++, MF_BYPOSITION | (/*Bits32 ? MF_DISABLED | MF_GRAYED | MF_UNCHECKED :*/ ((Rend == 3) ? MF_CHECKED : MF_UNCHECKED)), ID_GRAPHICS_RENDER_DOUBLE_INT, "Interpolated", "", "&Interpolated");
	MENU_L(GraphicsRender, i++, MF_BYPOSITION | (/*Bits32 ? MF_DISABLED | MF_GRAYED | MF_UNCHECKED :*/ ((Rend == 4) ? MF_CHECKED : MF_UNCHECKED)), ID_GRAPHICS_RENDER_FULLSCANLINE, "Scanline", "", "&Scanline");

	if (Have_MMX)
	{
		MENU_L(GraphicsRender, i++, MF_BYPOSITION | MF_STRING | (/*Bits32 ? MF_DISABLED | MF_GRAYED | MF_UNCHECKED :*/ ((Rend == 5) ? MF_CHECKED : MF_UNCHECKED)), ID_GRAPHICS_RENDER_50SCANLINE, "50% Scanline", "", "&50% Scanline");
		MENU_L(GraphicsRender, i++, MF_BYPOSITION | (/*Bits32 ? MF_DISABLED | MF_GRAYED | MF_UNCHECKED :*/ ((Rend == 6) ? MF_CHECKED : MF_UNCHECKED)), ID_GRAPHICS_RENDER_25SCANLINE, "25% Scanline", "", "&25% Scanline");
	}
	MENU_L(GraphicsRender, i++, MF_BYPOSITION | MF_STRING | (/*Bits32 ? MF_DISABLED | MF_GRAYED | MF_UNCHECKED :*/ ((Rend == 7) ? MF_CHECKED : MF_UNCHECKED)), ID_GRAPHICS_RENDER_INTESCANLINE, "Interpolated Scanline", "", "&Interpolated Scanline");
	if (Have_MMX)
	{
		MENU_L(GraphicsRender, i++, MF_BYPOSITION | MF_STRING | (/*Bits32 ? MF_DISABLED | MF_GRAYED | MF_UNCHECKED :*/ ((Rend == 8) ? MF_CHECKED : MF_UNCHECKED)), ID_GRAPHICS_RENDER_INT50SCANLIN, "Interpolated 50% Scanline", "", "Interpolated 50% Scanline");
		MENU_L(GraphicsRender, i++, MF_BYPOSITION | (/*Bits32 ? MF_DISABLED | MF_GRAYED | MF_UNCHECKED :*/ ((Rend == 9) ? MF_CHECKED : MF_UNCHECKED)), ID_GRAPHICS_RENDER_INT25SCANLIN, "Interpolated 25% Scanline", "", "Interpolated 25% Scanline");
		MENU_L(GraphicsRender, i++, MF_BYPOSITION | (Bits32 ? MF_DISABLED | MF_GRAYED | MF_UNCHECKED : ((Rend == 10) ? MF_CHECKED : MF_UNCHECKED)), ID_GRAPHICS_RENDER_2XSAI, "2xSAI (Kreed)", "", "2xSAI (&Kreed)");
	}

	// Menu GraphicsLayers
     // Nitsuja Added this
	i = 0;
	MENU_L(GraphicsLayers, i++, Flags | MF_POPUP, (UINT)GraphicsLayersA, "Scroll A", "", "Scroll &A");
	//MENU_L(GraphicsLayers, i++, MF_BYPOSITION | (VScrollAl ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYER0, "Layer 1", "", "Layer &1");
	MENU_L(GraphicsLayers, i++, Flags | MF_POPUP, (UINT)GraphicsLayersB, "Scroll B", "", "Scroll &B");
	//MENU_L(GraphicsLayers, i++, MF_BYPOSITION | (VScrollAl ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYER0, "Layer 1", "", "Layer &1");
	MENU_L(GraphicsLayers, i++, Flags | MF_POPUP, (UINT)GraphicsLayersS, "Sprites", "", "&Sprites");
	//MENU_L(GraphicsLayers, i++, MF_BYPOSITION | (VScrollAl ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYER0, "Layer 1", "", "Layer &1");

	//menu GraphicLayers Submenus
	i = 0;
	MENU_L(GraphicsLayersA, i, MF_BYPOSITION | (VScrollAl ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYER0, "Scroll A Low", "", "Scroll A &Low");
	MENU_L(GraphicsLayersB, i, MF_BYPOSITION | (VScrollBl ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYER1, "Scroll B Low", "", "Scroll B &Low");
	MENU_L(GraphicsLayersS, i++, MF_BYPOSITION | (VSpritel ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYERSPRITE, "Sprites Low", "", "Sprites &Low");
	MENU_L(GraphicsLayersA, i, MF_BYPOSITION | (VScrollAh ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYER2, "Scroll A High", "", "Scroll A &High");
	MENU_L(GraphicsLayersB, i, MF_BYPOSITION | (VScrollBh ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYER3, "Scroll B High", "", "Scroll B &High");
	MENU_L(GraphicsLayersS, i++, MF_BYPOSITION | (VSpriteh ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYERSPRITEHIGH, "Sprites High", "", "Sprites &High");
	MENU_L(GraphicsLayersA, i, MF_BYPOSITION | (Swap_Scroll_PriorityA ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYERSWAPA, "Swap Scroll Layers", "", "&Swap Scroll Layers");
	MENU_L(GraphicsLayersB, i, MF_BYPOSITION | (Swap_Scroll_PriorityB ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYERSWAPB, "Swap Scroll Layers", "", "&Swap Scroll Layers");
	MENU_L(GraphicsLayersS, i++, MF_BYPOSITION | (Swap_Sprite_Priority ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_LAYERSWAPS, "Swap Sprite Layers", "", "&Swap Sprite Layers");
	MENU_L(GraphicsLayersA, i, MF_BYPOSITION | (ScrollAOn ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_TOGGLEA, "Enable", "", "&Enable");
	MENU_L(GraphicsLayersB, i, MF_BYPOSITION | (ScrollBOn ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_TOGGLEB, "Enable", "", "&Enable");
	MENU_L(GraphicsLayersS, i++, MF_BYPOSITION | (SpriteOn ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_TOGGLES, "Enable", "", "&Enable");

	MENU_L(GraphicsLayersS, i++, MF_BYPOSITION | (Sprite_Always_Top ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_SPRITEALWAYS, "Sprites Always On Top", "", "Sprites Always On &Top");

	// Menu GraphicsFrameSkip

	Flags = MF_BYPOSITION | MF_STRING;

	MENU_L(GraphicsFrameSkip, 0, Flags | ((Frame_Skip == -1) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_FRAMESKIP_AUTO, "Auto", "", "&Auto");

	for(i = 0; i < 9; i++)
	{
		wsprintf(Str_Tmp ,"&%d", i);

		InsertMenu(GraphicsFrameSkip, i + 1, Flags | ((Frame_Skip == i) ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_FRAMESKIP_0 + i, Str_Tmp);
	}
	
	// Menu CPU

	i = 0;

#ifdef GENS_DEBUG
	MENU_L(CPU, i++, Flags | MF_POPUP, (UINT)CPUDebug, "Debug", "", "&Debug");
	InsertMenu(CPU, i++, MF_SEPARATOR, NULL, NULL);
#endif

	MENU_L(CPU, i++, Flags | MF_POPUP, (UINT)CPUCountry, "Country", "", "&Country");
	InsertMenu(CPU, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(CPU, i++, Flags, ID_CPU_RESET, "Hard Reset", "\tCtrl+Shift+R", "&Hard Reset");

	if (SegaCD_Started)
	{
		MENU_L(CPU, i++, Flags, ID_CPU_RESET68K, "Reset main 68000", "", "Reset &main 68000");
		MENU_L(CPU, i++, Flags, ID_CPU_RESET_SUB68K, "Reset sub 68000", "", "Reset &sub 68000");
	}
	else if (_32X_Started)
	{
		MENU_L(CPU, i++, Flags, ID_CPU_RESET68K, "Reset 68K", "", "Reset &68000");
		MENU_L(CPU, i++, Flags, ID_CPU_RESET_MSH2, "Reset master SH2", "", "Reset master SH2");
		MENU_L(CPU, i++, Flags, ID_CPU_RESET_SSH2, "Reset slave SH2", "", "Reset slave SH2");
	}
	else
	{
		MENU_L(CPU, i++, Flags, ID_CPU_RESET68K, "Reset 68K", "", "Reset &68000");
	}

	MENU_L(CPU, i++, Flags, ID_CPU_RESETZ80, "Reset Z80", "", "Reset &Z80");

	if (!Genesis_Started && !_32X_Started)
	{
		InsertMenu(CPU, i++, MF_SEPARATOR, NULL, NULL);

		MENU_L(CPU, i++, Flags | (SegaCD_Accurate ? MF_CHECKED : MF_UNCHECKED), ID_CPU_ACCURATE_SYNCHRO, "Perfect SegaCD Synchro", "", "&Perfect SegaCD Synchro");
	}
	//InsertMenu(CPU, i++, MF_SEPARATOR, NULL, NULL);

#ifdef GENS_DEBUG
	// Menu CPU Debug

	if (Debug == 1) Flags |= MF_CHECKED;
	else Flags &= ~MF_CHECKED;

	MENU_L(CPUDebug, 0, Flags, ID_CPU_DEBUG_GENESIS_68000, "Genesis - 68000", "", "&Genesis - 68000");

	if (Debug == 2) Flags |= MF_CHECKED;
	else Flags &= ~MF_CHECKED;

	MENU_L(CPUDebug, 1, Flags, ID_CPU_DEBUG_GENESIS_Z80, "Genesis - Z80", "", "Genesis - &Z80");

	if (Debug == 3) Flags |= MF_CHECKED;
	else Flags &= ~MF_CHECKED;

	MENU_L(CPUDebug, 2, Flags, ID_CPU_DEBUG_GENESIS_VDP, "Genesis - VDP", "", "Genesis - &VDP");

	i = 3;

	if (SegaCD_Started)
	{
		if (Debug == (i + 1)) Flags |= MF_CHECKED;
		else Flags &= ~MF_CHECKED;

		MENU_L(CPUDebug, i++, Flags, ID_CPU_DEBUG_SEGACD_68000, "SegaCD - 68000", "", "&SegaCD - 68000");

		if (Debug == (i + 1)) Flags |= MF_CHECKED;
		else Flags &= ~MF_CHECKED;

		MENU_L(CPUDebug, i++, Flags, ID_CPU_DEBUG_SEGACD_CDC, "SegaCD - CDC", "", "SegaCD - &CDC");

		if (Debug == (i + 1)) Flags |= MF_CHECKED;
		else Flags &= ~MF_CHECKED;

		MENU_L(CPUDebug, i++, Flags, ID_CPU_DEBUG_SEGACD_GFX, "SegaCD - GFX", "", "SegaCD - GF&X");
	}

	if (_32X_Started)
	{
		if (Debug == (i + 1)) Flags |= MF_CHECKED;
		else Flags &= ~MF_CHECKED;

		MENU_L(CPUDebug, i++, Flags, ID_CPU_DEBUG_32X_MAINSH2, "32X - main SH2", "", "32X - main SH2");

		if (Debug == (i + 1)) Flags |= MF_CHECKED;
		else Flags &= ~MF_CHECKED;

		MENU_L(CPUDebug, i++, Flags, ID_CPU_DEBUG_32X_SUBSH2, "32X - sub SH2", "", "32X - sub SH2");

		if (Debug == (i + 1)) Flags |= MF_CHECKED;
		else Flags &= ~MF_CHECKED;

		MENU_L(CPUDebug, i++, Flags, ID_CPU_DEBUG_32X_VDP, "32X - VDP", "", "32X - VDP");
	}
#endif


	// Menu CPU Country

	Flags = MF_BYPOSITION | MF_STRING;

	MENU_L(CPUCountry, 0, Flags | (Country == -1 ? MF_CHECKED : MF_UNCHECKED), ID_CPU_COUNTRY_AUTO, "Auto detect", "", "&Auto detect");
	MENU_L(CPUCountry, 1, Flags | (Country == 0 ? MF_CHECKED : MF_UNCHECKED), ID_CPU_COUNTRY_JAPAN, "Japan (NTSC)", "", "&Japan (NTSC)");
	MENU_L(CPUCountry, 2, Flags | (Country == 1 ? MF_CHECKED : MF_UNCHECKED), ID_CPU_COUNTRY_USA, "USA (NTSC)", "", "&USA (NTSC)");
	MENU_L(CPUCountry, 3, Flags | (Country == 2 ? MF_CHECKED : MF_UNCHECKED), ID_CPU_COUNTRY_EUROPE, "Europe (PAL)", "", "&Europe (PAL)");
	MENU_L(CPUCountry, 4, Flags | (Country == 3 ? MF_CHECKED : MF_UNCHECKED), ID_CPU_COUNTRY_MISC, "Japan (PAL)", "", "Japan (PAL)");
 
 	InsertMenu(CPUCountry, 5, MF_SEPARATOR, NULL, NULL);

	MENU_L(CPUCountry, 6, Flags | MF_POPUP, (UINT)CPUCountryOrder, "Auto detection order", "", "&Auto detection order");


	// Menu CPU Prefered Country 

	for(i = 0; i < 3; i++)
	{
		if (Country_Order[i] == 0)
		{
			MENU_L(CPUCountryOrder, i, Flags, ID_CPU_COUNTRY_ORDER + i, "USA (NTSC)", "", "&USA (NTSC)");
		}
		else if (Country_Order[i] == 1)
		{
			MENU_L(CPUCountryOrder, i, Flags, ID_CPU_COUNTRY_ORDER + i, "Japan (NTSC)", "", "&Japan (NTSC)");
		}
		else
		{
			MENU_L(CPUCountryOrder, i, Flags, ID_CPU_COUNTRY_ORDER + i, "Europe (PAL)", "", "&Europe (PAL)");
		}
	}


	// Menu Sound

	i = 0;

	MENU_L(Sound, i++, Flags | (Sound_Enable ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_ENABLE, "Enable", "", "&Enable");

	InsertMenu(Sound, i++, MF_SEPARATOR, NULL, NULL);

	MENU_L(Sound, i++, Flags | MF_POPUP, (UINT)SoundRate, "Rate", "", "&Rate");

	MENU_L(Sound, i++, Flags | (Sound_Stereo ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_STEREO, "Stereo", "", "&Stereo");

	MENU_L(Sound, i++, Flags | (Sound_Soften ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_SOFTEN, "Soften Filter", "", "Soften &Filter"); // Modif N.

	MENU_L(Sound, i++, Flags | (!Sleep_Time ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_HOG, "Hog CPU", "", "&Hog CPU"); // Modif N.

	InsertMenu(Sound, i++, MF_SEPARATOR, NULL, NULL);
	InsertMenu(Sound, i++, Flags, ID_VOLUME_CONTROL, "&Volume Control");
	InsertMenu(Sound, i++, MF_SEPARATOR, NULL, NULL);

	InsertMenu(Sound, i++, Flags | ((Z80_State & 1) ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_Z80ENABLE, "&Z80");
	InsertMenu(Sound, i++, Flags | (YM2612_Enable ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_YM2612ENABLE, "&YM2612");
	InsertMenu(Sound, i++, Flags | (PSG_Enable ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_PSGENABLE, "&PSG");

	InsertMenu(Sound, i++, Flags | (DAC_Enable ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_DACENABLE, "&DAC");

	if (!Genesis_Started && !_32X_Started)
	{
		InsertMenu(Sound, i++, Flags | (PCM_Enable ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_PCMENABLE, "P&CM");
	}

	if (!Genesis_Started && !_32X_Started)
	{
		InsertMenu(Sound, i++, Flags | (CDDA_Enable ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_CDDAENABLE, "CDD&A");
	}

	if (!Genesis_Started && !SegaCD_Started)
	{
		InsertMenu(Sound, i++, Flags | (PWM_Enable ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_PWMENABLE, "P&WM");
	}

	InsertMenu(Sound, i++, MF_SEPARATOR, NULL, NULL);

	MENU_L(Sound, i++, Flags | (YM2612_Improv ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_YMIMPROV, "YM2612 High Quality", "", "YM2612 High &Quality");
	MENU_L(Sound, i++, Flags | (PSG_Improv ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_PSGIMPROV, "PSG High Quality", "", "PSG High &Quality");
	MENU_L(Sound, i++, Flags | (DAC_Improv ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_DACIMPROV, "DAC High Quality", "", "DAC High &Quality");

	InsertMenu(Sound, i++, MF_SEPARATOR, NULL, NULL);

	MENU_L(Sound, i++, Flags, ID_SOUND_STARTWAVDUMP, WAV_Dumping?"Stop Dump":"Start Dump", "", WAV_Dumping?"Stop WAV Dump":"Start WAV Dump");

	MENU_L(Sound, i++, Flags, ID_SOUND_STARTGYMDUMP, GYM_Dumping?"Stop GYM Dump":"Start GYM Dump", "", GYM_Dumping?"Stop GYM Dump":"Start GYM Dump");

	// Sous-Menu SoundRate

	InsertMenu(SoundRate, 0, Flags | (Sound_Rate == 11025 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_11000, "&11025");
	InsertMenu(SoundRate, 1, Flags | (Sound_Rate == 22050 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_22000, "&22050");
	InsertMenu(SoundRate, 2, Flags | (Sound_Rate == 44100 ? MF_CHECKED : MF_UNCHECKED), ID_SOUND_RATE_44000, "&44100");

	//Upth-Add - Menu TAS_Tools 
	MENU_L(TAS_Tools,i++, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)Tools_Movies, "Movie", "", "&Movie");
	MENU_L(TAS_Tools,i++, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)Tools_AVI, "AVI Options", "", "&AVI Options");
	InsertMenu(TAS_Tools, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(TAS_Tools,i++, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)Tools_Trace, "Tracer Tools", "", "&Tracer tools");
	InsertMenu(TAS_Tools, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(TAS_Tools,i++, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)CPUSlowDownSpeed, "Slow Down Speed", "", "S&low Down Speed");
	MENU_L(TAS_Tools,i++,Flags | ((SlowDownMode==1) ? MF_CHECKED : MF_UNCHECKED),ID_SLOW_MODE,"Slow mode","","&Slow mode"); //Modif
	InsertMenu(TAS_Tools, i++, MF_SEPARATOR, NULL, NULL);
	MENU_L(TAS_Tools,i++,Flags,ID_RAM_SEARCH,"RAM Search","","&RAM Search"); //Modif N.
	MENU_L(TAS_Tools,i++,Flags,ID_RAM_WATCH,"RAM Watch","","RAM &Watch");   //Modif U.
	//Upth-Add - Menu Tools_Movies
	i = 0;
	MENU_L(Tools_Movies,i++,Flags | ((MainMovie.Status==MOVIE_PLAYING) ? MF_CHECKED : MF_UNCHECKED),ID_PLAY_MOVIE,"Play Movie or Resume record from savestate","","&Play Movie or Resume record from savestate"); //Modif
	MENU_L(Tools_Movies,i++,Flags | ((MainMovie.Status==MOVIE_RECORDING) ? MF_CHECKED : MF_UNCHECKED),ID_RECORD_MOVIE,"Record New Movie","","Record &New Movie"); //Modif
	MENU_L(Tools_Movies,i++,Flags ,ID_RESUME_RECORD,"Resume record from now","","&Resume record from now"); //Modif
	MENU_L(Tools_Movies,i++,Flags | ((SpliceFrame) ? MF_CHECKED : MF_UNCHECKED) | ((MainMovie.File) ? MF_ENABLED : MF_DISABLED | MF_GRAYED),ID_SPLICE,"Input Splice","\tShift-S","&Input Splice"); //Modif
	MENU_L(Tools_Movies,i++,Flags | ((SeekFrame) ? MF_CHECKED : MF_UNCHECKED) | ((MainMovie.File) ? MF_ENABLED : MF_DISABLED | MF_GRAYED),IDC_SEEK_FRAME,"Seek to Frame","","Seek to &Frame"); //Modif
	MENU_L(Tools_Movies,i++, MF_BYPOSITION | MF_POPUP | MF_STRING| (MainMovie.Status ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)), (UINT)Movies_Tracks, "Tracks", "", "&Tracks"); //Modif
	MENU_L(Tools_Movies,i++,((MainMovie.File != NULL) ? Flags : (Flags | MF_DISABLED | MF_GRAYED)),ID_STOP_MOVIE,"Stop Movie","","&Stop Movie"); 
 	//Upth-Add - Menu Movies_Tracks
	i = 0;
	MENU_L(Movies_Tracks,i++,Flags,ID_MOVIE_CHANGETRACK_ALL,"All Players","\tCtrl-Shift-0","&All Players"); //Modif
	MENU_L(Movies_Tracks,i++,Flags | ((track & TRACK1) ? MF_CHECKED : MF_UNCHECKED),ID_MOVIE_CHANGETRACK_1,"Player 1","\tCtrl-Shift-1","Players &1"); //Modif
	MENU_L(Movies_Tracks,i++,Flags | ((track & TRACK2) ? MF_CHECKED : MF_UNCHECKED),ID_MOVIE_CHANGETRACK_2,"Player 2","\tCtrl-Shift-2","Players &2"); //Modif
	MENU_L(Movies_Tracks,i++,Flags | (MainMovie.TriplePlayerHack ? MF_ENABLED : MF_DISABLED) | ((track & TRACK3) ? MF_CHECKED : MF_UNCHECKED),ID_MOVIE_CHANGETRACK_3,"Player 3","\tCtrl-Shift-3","Players &3"); //Modif
 	//Upth-Add - Menu Tools_AVI
	i = 0;
	MENU_L(Tools_AVI, i++, Flags | (AVIWaitMovie ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_SYNC_AVI_MOVIE, "Sync AVI with movie", "", "S&ync AVI with movie");
	MENU_L(Tools_AVI, i++, Flags | (AVISound ? MF_CHECKED : MF_UNCHECKED), ID_GRAPHICS_AVI_SOUND, "Add sound to AVI", "", "&Add sound to AVI");
	MENU_L(Tools_AVI, i++, Flags | (CleanAvi ? MF_CHECKED : MF_UNCHECKED), ID_CHANGE_CLEANAVI, "Clean AVI screen", "", "&Clean AVI screen");
	MENU_L(Tools_AVI, i++, Flags | MF_UNCHECKED, ID_GRAPHICS_AVI, AVIRecording?"Stop AVI Dump":"Start AVI Dump", "", AVIRecording?"&Stop AVI Dump":"&Start AVI Dump");
 	//Upth-Add - Menu Tools_Trace
	i = 0;
	MENU_L(Tools_Trace, i++, Flags | (trace_map ? MF_CHECKED : MF_UNCHECKED), ID_CHANGE_TRACE, "Log Instructions", "", "&Trace");
	MENU_L(Tools_Trace, i++, Flags | (hook_trace ? MF_CHECKED : MF_UNCHECKED), ID_CHANGE_HOOK, "Log RAM access", "", "&Hook RAM");

	//Upth-Modif - Slow Mode Selection -- now a submenu of TAS_Tools
	// Menu CPUSlowDownSpeed
	i = 0;
	MENU_L(CPUSlowDownSpeed, i++, Flags | ((SlowDownSpeed == 1) ? MF_CHECKED : MF_UNCHECKED), ID_SLOW_SPEED_1, "50%", "", "50%");
	MENU_L(CPUSlowDownSpeed, i++, Flags | ((SlowDownSpeed == 2) ? MF_CHECKED : MF_UNCHECKED), ID_SLOW_SPEED_2, "33%", "", "33%");
	MENU_L(CPUSlowDownSpeed, i++, Flags | ((SlowDownSpeed == 3) ? MF_CHECKED : MF_UNCHECKED), ID_SLOW_SPEED_3, "25%", "", "25%");
	MENU_L(CPUSlowDownSpeed, i++, Flags | ((SlowDownSpeed == 4) ? MF_CHECKED : MF_UNCHECKED), ID_SLOW_SPEED_4, "20%", "", "20%");
	MENU_L(CPUSlowDownSpeed, i++, Flags | ((SlowDownSpeed == 5) ? MF_CHECKED : MF_UNCHECKED), ID_SLOW_SPEED_5, "16%", "", "16%");
	MENU_L(CPUSlowDownSpeed, i++, Flags | ((SlowDownSpeed == 9) ? MF_CHECKED : MF_UNCHECKED), ID_SLOW_SPEED_9, "10%", "", "10%");
	MENU_L(CPUSlowDownSpeed, i++, Flags | ((SlowDownSpeed == 15)? MF_CHECKED : MF_UNCHECKED), ID_SLOW_SPEED_15, "6%", "", " 6%");
	MENU_L(CPUSlowDownSpeed, i  , Flags | ((SlowDownSpeed == 31)? MF_CHECKED : MF_UNCHECKED), ID_SLOW_SPEED_31, "3%", "", " 3%");
	
	// Menu Options

	MENU_L(Options, 0, MF_BYPOSITION | MF_STRING, ID_OPTIONS_GENERAL, "General ", "", "&General..."); // Modif N: changed Misc... to General...
	MENU_L(Options, 1, Flags, ID_OPTIONS_JOYPADSETTING, "Joypad", "", "&Joypads...");
	MENU_L(Options, 2, Flags, ID_OPTIONS_CHANGEDIR, "Directories", "", "&Directories...");
	MENU_L(Options, 3, Flags, ID_OPTIONS_CHANGEFILES, "Bios/Misc Files", "", "Bios/Misc &Files...");
	InsertMenu(Options, 4, MF_SEPARATOR, NULL, NULL);
	MENU_L(Options, 5, Flags | MF_POPUP, (UINT)OptionsCDDrive, "Current CD Drive", "", "Current CD Drive");
	MENU_L(Options, 6, Flags | MF_POPUP, (UINT)OptionsSRAMSize, "Sega CD SRAM Size", "", "Sega CD SRAM Size");
	InsertMenu(Options, 7, MF_SEPARATOR, NULL, NULL);
	MENU_L(Options, 8, Flags, ID_OPTIONS_LOADCONFIG, "Load config", "", "&Load config");
	MENU_L(Options, 9, Flags, ID_OPTIONS_SAVEASCONFIG, "Save config as", "", "&Save config as");


	// Sous-Menu CDDrive

	if (Num_CD_Drive)
	{
		char drive_name[100];

		for(i = 0; i < Num_CD_Drive; i++)
		{
			ASPI_Get_Drive_Info(i, (unsigned char *) drive_name);

			if (CUR_DEV == i)
			{
				InsertMenu(OptionsCDDrive, i, Flags | MF_CHECKED, ID_OPTION_CDDRIVE_0 + i, &drive_name[8]);
			}
			else
			{
				InsertMenu(OptionsCDDrive, i, Flags | MF_UNCHECKED, ID_OPTION_CDDRIVE_0 + i, &drive_name[8]);
			}
		}
	}
	else
	{
		MENU_L(OptionsCDDrive, 0, Flags | MF_GRAYED, NULL, "No drive detected", "", "No Drive Detected");
	}


	// Sous-Menu SRAMSize

	if (BRAM_Ex_State & 0x100)
	{
		MENU_L(OptionsSRAMSize, 0, Flags | MF_UNCHECKED, ID_OPTION_SRAMSIZE_0, "None", "", "&None");

		for (i = 0; i < 4; i++)
		{
			char bsize[16];

			sprintf(bsize, "&%d Kb", 8 << i);

			if (BRAM_Ex_Size == i)
			{
				InsertMenu(OptionsSRAMSize, i + 1, Flags | MF_CHECKED, ID_OPTION_SRAMSIZE_8 + i, bsize);
			}
			else
			{
				InsertMenu(OptionsSRAMSize, i + 1, Flags | MF_UNCHECKED, ID_OPTION_SRAMSIZE_8 + i, bsize);
			}
		}
	}
	else
	{
		MENU_L(OptionsSRAMSize, 0, Flags | MF_CHECKED, ID_OPTION_SRAMSIZE_0, "None", "", "&None");

		for (i = 0; i < 4; i++)
		{
			char bsize[16];

			sprintf(bsize, "&%d Kb", 8 << i);

			InsertMenu(OptionsSRAMSize, i + 1, Flags | MF_UNCHECKED, ID_OPTION_SRAMSIZE_8 + i, bsize);
		}
	}


	// Menu Help

	i = 0;
	
	while (language_name[i])
	{
		GetPrivateProfileString(language_name[i], "Menu Language", "Undefined language", Str_Tmp, 1024, Language_Path);
		if (Language == i)
			InsertMenu(Help, i, Flags | MF_CHECKED, ID_HELP_LANG + i, Str_Tmp);
		else 
			InsertMenu(Help, i, Flags | MF_UNCHECKED, ID_HELP_LANG + i, Str_Tmp);

		i++;
	}

	InsertMenu(Help, i++, MF_SEPARATOR, NULL, NULL);

	if (Detect_Format(Manual_Path) != -1)		// can be used to detect if file exist
	{
		MENU_L(Help, i++, Flags, ID_HELP_MENU_FILE, "File menu" ,"", "&File menu");
		MENU_L(Help, i++, Flags, ID_HELP_MENU_GRAPHICS, "Graphics menu" ,"", "&Graphics menu");
		MENU_L(Help, i++, Flags, ID_HELP_MENU_CPU, "CPU menu" ,"", "&CPU menu");
		MENU_L(Help, i++, Flags, ID_HELP_MENU_SOUND, "Sound menu" ,"", "&Sound menu");
		MENU_L(Help, i++, Flags, ID_HELP_MENU_OPTIONS, "Options menu" ,"", "&Options menu");

		InsertMenu(Help, i++, MF_SEPARATOR, NULL, NULL);

		MENU_L(Help, i++, Flags, ID_HELP_NETPLAY, "Netplay" ,"", "&Netplay");
		MENU_L(Help, i++, Flags, ID_HELP_MEGACD, "Mega/Sega CD" ,"", "&Mega/Sega CD");
		MENU_L(Help, i++, Flags, ID_HELP_FAQ, "FAQ" ,"", "&FAQ");
		MENU_L(Help, i++, Flags, ID_HELP_KEYS, "Shortcuts" ,"", "&Defaults keys && Shortcuts");

		InsertMenu(Help, i++, MF_SEPARATOR, NULL, NULL);
	}

	MENU_L(Help, i, Flags, ID_HELP_ABOUT, "About" ,"", "&About");

	Gens_Menu = MainMenu;

	if (Full_Screen) SetMenu(HWnd, NULL);
	else SetMenu(HWnd, Gens_Menu);

	return(Gens_Menu);
}


LRESULT CALLBACK GGenieProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2, i, value;
	char tmp[1024];

	switch(uMsg)
	{
		case WM_INITDIALOG:
			Build_Language_String();

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

			WORD_L(IDC_INFO_GG, "Informations GG", "", "Informations about GG/Patch codes");
			WORD_L(IDC_GGINFO1, "Game Genie info 1", "", "Both Game Genie code and Patch code are supported.");
			WORD_L(IDC_GGINFO2, "Game Genie info 2", "", "Highlight a code to activate it.");
			WORD_L(IDC_GGINFO3, "Game Genie info 3", "", "yntax for Game Genie code :  XXXX-XXXX");
			WORD_L(IDC_GGINFO4, "Game Genie info 4", "", "Syntax for Patch code :  XXXXXX:YYYY    (address:data)");

			WORD_L(ID_GGADD, "Add code", "", "Add &code");
			WORD_L(ID_GGREMOVE, "Remove selected codes", "", "&Remove selected codes");
			WORD_L(ID_GGDEACTIVATE, "Deactivate all codes", "", "&Deactivate all codes");
			WORD_L(ID_OK, "OK", "", "&OK");
			WORD_L(ID_CANCEL, "Cancel", "", "&Cancel");

			for(i = 0; i < 256; i++)
			{
				if (Liste_GG[i].code[0] != 0)
				{
					strcpy(Str_Tmp, Liste_GG[i].code);
					while (strlen(Str_Tmp) < 20) strcat(Str_Tmp, " ");
					strcat(Str_Tmp, Liste_GG[i].name);

					SendDlgItemMessage(hDlg, IDC_LIST1, LB_ADDSTRING, (WPARAM) 0, (LONG) (LPTSTR) Str_Tmp);

					if (Liste_GG[i].active)
						SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETSEL, (WPARAM) 1, (LONG) i);
					else
						SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETSEL, (WPARAM) 0, (LONG) i);

					if ((Liste_GG[i].restore != 0xFFFFFFFF) && (Liste_GG[i].addr < Rom_Size) && (Genesis_Started))
					{
						Rom_Data[Liste_GG[i].addr] = (unsigned char)(Liste_GG[i].restore & 0xFF);
						Rom_Data[Liste_GG[i].addr + 1] = (unsigned char)((Liste_GG[i].restore & 0xFF00) >> 8);
					}
				}
			}
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_GGADD:
					if (GetDlgItemText(hDlg, IDC_EDIT1, Str_Tmp, 14))
					{
						if ((strlen(Str_Tmp) == 9) || (strlen(Str_Tmp) == 11))
						{						
							strupr(Str_Tmp);
							while (strlen(Str_Tmp) < 20) strcat(Str_Tmp, " ");

							GetDlgItemText(hDlg, IDC_EDIT2, (char *) (Str_Tmp + strlen(Str_Tmp)), 240);

							SendDlgItemMessage(hDlg, IDC_LIST1, LB_ADDSTRING, (WPARAM) 0, (LONG) (LPTSTR) Str_Tmp);

							SetDlgItemText(hDlg, IDC_EDIT1, "");
							SetDlgItemText(hDlg, IDC_EDIT2, "");
						}
					}
					return true;
					break;

				case ID_GGREMOVE:
					value = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
					if (value == LB_ERR) value = 0;

					for(i = value - 1; i >= 0; i--)
					{
						if (SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETSEL, (WPARAM) i, NULL) > 0)
							SendDlgItemMessage(hDlg, IDC_LIST1, LB_DELETESTRING , (WPARAM) i, (LPARAM) 0);
					}
					return true;
					break;

				case ID_GGDEACTIVATE:
					value = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
					if (value == LB_ERR) value = 0;

					for(i = value - 1; i >= 0; i--)
					{
						SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETSEL , (WPARAM) 0, (LPARAM) i);
					}
					return true;
					break;

				case ID_OK:
					value = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
					if (value == LB_ERR) value = 0;

					for(i = 0; i < 256; i++)
					{
						Liste_GG[i].code[0] = 0;
						Liste_GG[i].name[0] = 0;
						Liste_GG[i].active = 0;
						Liste_GG[i].addr = 0xFFFFFFFF;
						Liste_GG[i].data = 0;
						Liste_GG[i].restore = 0xFFFFFFFF;
					}

					for(i = 0; i < value; i++)
					{
						if (SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXT, (WPARAM) i, (LONG) (LPTSTR) tmp) != LB_ERR)
						{
							dx1 = 0;

							while ((tmp[dx1] != ' ') && (tmp[dx1] != 0)) dx1++;

							memcpy(Liste_GG[i].code, tmp, dx1);
							Liste_GG[i].code[dx1] = 0;

							while ((tmp[dx1] == ' ') && (tmp[dx1] != 0)) dx1++;

							strcpy(Liste_GG[i].name, (char *) (tmp + dx1));

							if (SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETSEL, (WPARAM) i, NULL) > 0)
								Liste_GG[i].active = 1;
							else Liste_GG[i].active = 0;
						}
					}

					for(i = 0; i < value; i++)
					{
						if ((Liste_GG[i].code[0] != 0) && (Liste_GG[i].addr == 0xFFFFFFFF) && (Liste_GG[i].data == 0))
						{
							decode(Liste_GG[i].code, (patch *) (&(Liste_GG[i].addr)));

							if ((Liste_GG[i].restore == 0xFFFFFFFF) && (Liste_GG[i].addr < Rom_Size) && (Genesis_Started))
							{
								Liste_GG[i].restore = (unsigned int) (Rom_Data[Liste_GG[i].addr] & 0xFF);
								Liste_GG[i].restore += (unsigned int) ((Rom_Data[Liste_GG[i].addr + 1] & 0xFF) << 8);
							}
						}
					}

				case ID_CANCEL:
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;


				case ID_HELP_HELP:
					if (Detect_Format(Manual_Path) != -1)		// can be used to detect if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpgamegenie.html");
						system(Str_Tmp);
					}
					else
					{
						MessageBox(NULL, "You need to configure the Manual Path to have help", "info", MB_OK);
					}
					return true;
					break;
			}
			break;

		case WM_CLOSE:
			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}


LRESULT CALLBACK DirectoriesProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char Str_Tmp2[1024];
	
	switch(uMsg)
	{
		case WM_INITDIALOG:
			RECT r;
			RECT r2;
			int dx1, dy1, dx2, dy2;

			Build_Language_String();

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

			WORD_L(IDD_DIRECTORIES, "Directories configuration", "", "Directories configuration");
			WORD_L(IDC_DIRECTORIES, "Setting directories", "", "Configure directories");
						
			WORD_L(ID_CANCEL, "Cancel", "", "&Cancel");
			WORD_L(ID_OK, "OK", "", "&OK");

			WORD_L(ID_CHANGE_SAVE, "Change", "", "Change");
			WORD_L(ID_CHANGE_SRAM, "Change", "", "Change");
			WORD_L(ID_CHANGE_BRAM, "Change", "", "Change");
			WORD_L(ID_CHANGE_WAV, "Change", "", "Change");
			WORD_L(ID_CHANGE_GYM, "Change", "", "Change");
			WORD_L(ID_CHANGE_SHOT, "Change", "", "Change");
			WORD_L(ID_CHANGE_PATCH, "Change", "", "Change");
			WORD_L(ID_CHANGE_IPS, "Change", "", "Change");
			WORD_L(ID_CHANGE_MOVIE, "Change", "", "Change");

			WORD_L(IDC_STATIC_SAVE, "Save static", "", "SAVE STATE");
			WORD_L(IDC_STATIC_SRAM, "Sram static", "", "SRAM BACKUP");
			WORD_L(IDC_STATIC_BRAM, "Bram static", "", "BRAM BACKUP");
			WORD_L(IDC_STATIC_WAV, "Wav static", "", "WAV DUMP");
			WORD_L(IDC_STATIC_GYM, "Gym static", "", "GYM DUMP");
			WORD_L(IDC_STATIC_SHOT, "Shot static", "", "SCREEN SHOT");
			WORD_L(IDC_STATIC_PATCH, "Patch static", "", "PAT PATCH");
			WORD_L(IDC_STATIC_IPS, "IPS static", "", "IPS PATCH");

			SetDlgItemText(hDlg, IDC_EDIT_SAVE, State_Dir);
			SetDlgItemText(hDlg, IDC_EDIT_SRAM, SRAM_Dir);
			SetDlgItemText(hDlg, IDC_EDIT_BRAM, BRAM_Dir);
			SetDlgItemText(hDlg, IDC_EDIT_WAV, Dump_Dir);
			SetDlgItemText(hDlg, IDC_EDIT_GYM, Dump_GYM_Dir);
			SetDlgItemText(hDlg, IDC_EDIT_SHOT, ScrShot_Dir);
			SetDlgItemText(hDlg, IDC_EDIT_PATCH, Patch_Dir);
			SetDlgItemText(hDlg, IDC_EDIT_IPS, IPS_Dir);
			SetDlgItemText(hDlg, IDC_EDIT_MOVIE, Movie_Dir);

			return true;
			break;

		case WM_COMMAND:
			switch(wParam)
			{
				case ID_CHANGE_SAVE:
					GetDlgItemText(hDlg, IDC_EDIT_SAVE, Str_Tmp2, 1024);
					if (Change_Dir(Str_Tmp, Str_Tmp2, "Save state directory", "Save state files\0*.gs?\0\0", "gs0"))
						SetDlgItemText(hDlg, IDC_EDIT_SAVE, Str_Tmp);
					break;

				case ID_CHANGE_SRAM:
					GetDlgItemText(hDlg, IDC_EDIT_SRAM, Str_Tmp2, 1024);
					if (Change_Dir(Str_Tmp, Str_Tmp2, "SRAM backup directory", "SRAM backup files\0*.srm\0\0", "srm"))
						SetDlgItemText(hDlg, IDC_EDIT_SRAM, Str_Tmp);
					break;

				case ID_CHANGE_BRAM:
					GetDlgItemText(hDlg, IDC_EDIT_BRAM, Str_Tmp2, 1024);
					if (Change_Dir(Str_Tmp, Str_Tmp2, "BRAM backup directory", "BRAM backup files\0*.brm\0\0", "brm"))
						SetDlgItemText(hDlg, IDC_EDIT_BRAM, Str_Tmp);
					break;

				case ID_CHANGE_WAV:
					GetDlgItemText(hDlg, IDC_EDIT_WAV, Str_Tmp2, 1024);
					if (Change_Dir(Str_Tmp, Str_Tmp2, "Sound WAV dump directory", "Sound WAV dump files\0*.wav\0\0", "wav"))
						SetDlgItemText(hDlg, IDC_EDIT_WAV, Str_Tmp);
					break;

				case ID_CHANGE_GYM:
					GetDlgItemText(hDlg, IDC_EDIT_GYM, Str_Tmp2, 1024);
					if (Change_Dir(Str_Tmp, Str_Tmp2, "GYM dump directory", "GYM dump files\0*.gym\0\0", "gym"))
						SetDlgItemText(hDlg, IDC_EDIT_GYM, Str_Tmp);
					break;

				case ID_CHANGE_SHOT:
					GetDlgItemText(hDlg, IDC_EDIT_SHOT, Str_Tmp2, 1024);
					if (Change_Dir(Str_Tmp, Str_Tmp2, "Screen-shot directory", "Screen-shot files\0*.bmp\0\0", "bmp"))
						SetDlgItemText(hDlg, IDC_EDIT_SHOT, Str_Tmp);
					break;

				case ID_CHANGE_PATCH:
					GetDlgItemText(hDlg, IDC_EDIT_PATCH, Str_Tmp2, 1024);
					if (Change_Dir(Str_Tmp, Str_Tmp2, "PAT Patch directory", "PAT Patch files\0*.pat\0\0", "pat"))
						SetDlgItemText(hDlg, IDC_EDIT_PATCH, Str_Tmp);
					break;

				case ID_CHANGE_IPS:
					GetDlgItemText(hDlg, IDC_EDIT_IPS, Str_Tmp2, 1024);
					if (Change_Dir(Str_Tmp, Str_Tmp2, "IPS Patch directory", "IPS Patch files\0*.ips\0\0", "ips"))
						SetDlgItemText(hDlg, IDC_EDIT_IPS, Str_Tmp);
					break;
				case ID_CHANGE_MOVIE:
					GetDlgItemText(hDlg, IDC_EDIT_MOVIE, Str_Tmp2, 1024);
					if (Change_Dir(Str_Tmp, Str_Tmp2, "Movie Patch directory", "Gens Movie files\0*.gmv\0\0", "gmv"))
						SetDlgItemText(hDlg, IDC_EDIT_MOVIE, Str_Tmp);
					break;
				case ID_OK:
					GetDlgItemText(hDlg, IDC_EDIT_SAVE, State_Dir, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_SRAM, SRAM_Dir, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_BRAM, BRAM_Dir, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_WAV, Dump_Dir, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_GYM, Dump_GYM_Dir, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_SHOT, ScrShot_Dir, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_PATCH, Patch_Dir, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_IPS, IPS_Dir, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_MOVIE, Movie_Dir, 1024);
				case ID_CANCEL:
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;

				case ID_HELP_HELP:
					if (Detect_Format(Manual_Path) != -1)		// can be used to detect if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpdir.html");
						system(Str_Tmp);
					}
					else
					{
						MessageBox(NULL, "You need to configure the Manual Path to have help", "info", MB_OK);
					}
					return true;
					break;
			}
			break;

		case WM_CLOSE:
			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}


LRESULT CALLBACK FilesProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char Str_Tmp2[1024];
	switch(uMsg)
	{
		case WM_INITDIALOG:
			RECT r;
			RECT r2;
			int dx1, dy1, dx2, dy2;

			Build_Language_String();

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

			WORD_L(IDD_FILES, "Files configuration", "", "Files configuration");
			WORD_L(IDC_GENESISBIOS_FILE, "Setting Genesis bios file", "", "Configure Genesis bios file");
			WORD_L(IDC_32XBIOS_FILES, "Setting 32X bios files", "", "Configure 32X bios files");
			WORD_L(IDC_CDBIOS_FILES, "Setting SEGA CD bios files", "", "Configure SEGA CD bios files");
			WORD_L(IDC_MISC_FILES, "Setting misc files", "", "Configure misc file");
						
			WORD_L(ID_CANCEL, "Cancel", "", "&Cancel");
			WORD_L(ID_OK, "OK", "", "&OK");

			WORD_L(ID_CHANGE_GENESISBIOS, "Change", "", "Change");
			WORD_L(ID_CHANGE_32XGBIOS, "Change", "", "Change");
			WORD_L(ID_CHANGE_32XMBIOS, "Change", "", "Change");
			WORD_L(ID_CHANGE_32XSBIOS, "Change", "", "Change");
			WORD_L(ID_CHANGE_USBIOS, "Change", "", "Change");
			WORD_L(ID_CHANGE_EUBIOS, "Change", "", "Change");
			WORD_L(ID_CHANGE_JABIOS, "Change", "", "Change");
			WORD_L(ID_CHANGE_CGOFFLINE, "Change", "", "Change");
			WORD_L(ID_CHANGE_MANUAL, "Change", "", "Change");

			WORD_L(IDC_STATIC_GENESISBIOS, "Genesis bios static", "", "Genesis");
			WORD_L(IDC_STATIC_32XGBIOS, "M68000 bios static", "", "M68000");
			WORD_L(IDC_STATIC_32XMBIOS, "M SH2 bios static", "", "Master SH2");
			WORD_L(IDC_STATIC_32XSBIOS, "S SH2 bios static", "", "Slave SH2");
			WORD_L(IDC_STATIC_USBIOS, "US bios static", "", "USA");
			WORD_L(IDC_STATIC_EUBIOS, "EU bios static", "", "Europe");
			WORD_L(IDC_STATIC_JABIOS, "JA bios static", "", "Japan");
			WORD_L(IDC_STATIC_CGOFFLINE, "CGOffline static", "", "CGOffline");
			WORD_L(IDC_STATIC_MANUAL, "Manual static", "", "Manual");

			SetDlgItemText(hDlg, IDC_EDIT_GENESISBIOS, Genesis_Bios);
			SetDlgItemText(hDlg, IDC_EDIT_32XGBIOS, _32X_Genesis_Bios);
			SetDlgItemText(hDlg, IDC_EDIT_32XMBIOS, _32X_Master_Bios);
			SetDlgItemText(hDlg, IDC_EDIT_32XSBIOS, _32X_Slave_Bios);
			SetDlgItemText(hDlg, IDC_EDIT_USBIOS, US_CD_Bios);
			SetDlgItemText(hDlg, IDC_EDIT_EUBIOS, EU_CD_Bios);
			SetDlgItemText(hDlg, IDC_EDIT_JABIOS, JA_CD_Bios);
			SetDlgItemText(hDlg, IDC_EDIT_CGOFFLINE, CGOffline_Path);
			SetDlgItemText(hDlg, IDC_EDIT_MANUAL, Manual_Path);

			return true;
			break;

		case WM_COMMAND:
			switch(wParam)
			{
				case ID_CHANGE_GENESISBIOS:
					GetDlgItemText(hDlg, IDC_EDIT_GENESISBIOS, Str_Tmp2, 1024);
					strcpy(Str_Tmp, "genesis.bin"); 
					DialogsOpen++;
					if (Change_File_S(Str_Tmp, Str_Tmp2, "Genesis bios file", "bios files\0*.bin\0\0", "bin"))
						SetDlgItemText(hDlg, IDC_EDIT_GENESISBIOS, Str_Tmp);
					DialogsOpen--;
					break;

				case ID_CHANGE_32XGBIOS:
					GetDlgItemText(hDlg, IDC_EDIT_32XGBIOS, Str_Tmp2, 1024);
					strcpy(Str_Tmp, "32X_G_bios.bin"); 
					DialogsOpen++;
					if (Change_File_S(Str_Tmp, Str_Tmp2, "32X M68000 bios file", "bios files\0*.bin\0\0", "bin"))
						SetDlgItemText(hDlg, IDC_EDIT_32XGBIOS, Str_Tmp);
					DialogsOpen--;
					break;

				case ID_CHANGE_32XMBIOS:
					GetDlgItemText(hDlg, IDC_EDIT_32XMBIOS, Str_Tmp2, 1024);
					strcpy(Str_Tmp, "32X_M_bios.bin"); 
					DialogsOpen++;
					if (Change_File_S(Str_Tmp, Str_Tmp2, "32X Master SH2 bios file", "bios files\0*.bin\0\0", "bin"))
						SetDlgItemText(hDlg, IDC_EDIT_32XMBIOS, Str_Tmp);
					DialogsOpen--;
					break;

				case ID_CHANGE_32XSBIOS:
					GetDlgItemText(hDlg, IDC_EDIT_32XSBIOS, Str_Tmp2, 1024);
					strcpy(Str_Tmp, "32X_S_bios.bin"); 
					DialogsOpen++;
					if (Change_File_S(Str_Tmp, Str_Tmp2, "32X Slave SH2 bios file", "bios files\0*.bin\0\0", "bin"))
						SetDlgItemText(hDlg, IDC_EDIT_32XSBIOS, Str_Tmp);
					DialogsOpen--;
					break;

				case ID_CHANGE_USBIOS:
					GetDlgItemText(hDlg, IDC_EDIT_USBIOS, Str_Tmp2, 1024);
					strcpy(Str_Tmp, "us_scd1_9210.bin"); 
					DialogsOpen++;
					if (Change_File_S(Str_Tmp, Str_Tmp2, "USA CD bios file", "bios files\0*.bin\0\0", "bin"))
						SetDlgItemText(hDlg, IDC_EDIT_USBIOS, Str_Tmp);
					DialogsOpen--;
					break;

				case ID_CHANGE_EUBIOS:
					GetDlgItemText(hDlg, IDC_EDIT_EUBIOS, Str_Tmp2, 1024);
					strcpy(Str_Tmp, "eu_mcd1_9210.bin"); 
					DialogsOpen++;
					if (Change_File_S(Str_Tmp, Str_Tmp2, "EUROPEAN CD bios file", "bios files\0*.bin\0\0", "bin"))
						SetDlgItemText(hDlg, IDC_EDIT_EUBIOS, Str_Tmp);
					DialogsOpen--;
					break;

				case ID_CHANGE_JABIOS:
					GetDlgItemText(hDlg, IDC_EDIT_JABIOS, Str_Tmp2, 1024);
					strcpy(Str_Tmp, "jp_mcd1_9112.bin"); 
					DialogsOpen++;
					if (Change_File_S(Str_Tmp, Str_Tmp2, "JAPAN CD bios file", "bios files\0*.bin\0\0", "bin"))
						SetDlgItemText(hDlg, IDC_EDIT_JABIOS, Str_Tmp);
					DialogsOpen--;
					break;

				case ID_CHANGE_CGOFFLINE:
					GetDlgItemText(hDlg, IDC_EDIT_CGOFFLINE, Str_Tmp2, 1024);
					strcpy(Str_Tmp, "GCOffline.chm"); 
					if (Change_File_S(Str_Tmp, Str_Tmp2, "Genesis Collective - CGOffline file", "html help files\0*.chm\0\0", "chm"))
						SetDlgItemText(hDlg, IDC_EDIT_CGOFFLINE, Str_Tmp);
					DialogsOpen--;
					break;

				case ID_CHANGE_MANUAL:
					GetDlgItemText(hDlg, IDC_EDIT_MANUAL, Str_Tmp2, 1024);
					strcpy(Str_Tmp, "Manual.exe"); 
					if (Change_File_S(Str_Tmp, Str_Tmp2, "Gens Manual", "executable files\0*.exe\0\0", "exe"))
						SetDlgItemText(hDlg, IDC_EDIT_MANUAL, Str_Tmp);
					DialogsOpen--;
					break;

				case ID_OK:
					GetDlgItemText(hDlg, IDC_EDIT_GENESISBIOS, Genesis_Bios, 1024);

					GetDlgItemText(hDlg, IDC_EDIT_32XGBIOS, _32X_Genesis_Bios, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_32XMBIOS, _32X_Master_Bios, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_32XSBIOS, _32X_Slave_Bios, 1024);

					GetDlgItemText(hDlg, IDC_EDIT_USBIOS, US_CD_Bios, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_EUBIOS, EU_CD_Bios, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_JABIOS, JA_CD_Bios, 1024);

					GetDlgItemText(hDlg, IDC_EDIT_CGOFFLINE, CGOffline_Path, 1024);
					GetDlgItemText(hDlg, IDC_EDIT_MANUAL, Manual_Path, 1024);

				case ID_CANCEL:
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;

				case ID_HELP_HELP:
					if (Detect_Format(Manual_Path) != -1)		// can be used to detect if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpdir.html");
						system(Str_Tmp);
					}
					else
					{
						MessageBox(NULL, "You need to configure the Manual Path to have help", "info", MB_OK);
					}
					return true;
					break;
			}
			break;

		case WM_CLOSE:
			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}
int GetControlType(int i)
{
	switch(i)
	{
		case 0:
			return Controller_1_Type;
		case 1:
			return Controller_2_Type;
		case 2:
			return Controller_1B_Type;
		case 3:
			return Controller_1C_Type;
		case 4:
			return Controller_1D_Type;
		case 5:
			return Controller_2B_Type;
		case 6:
			return Controller_2C_Type;
		case 7:
			return Controller_2D_Type;
		default:
			return -1;
	}
}
LRESULT CALLBACK PlayMovieProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			//SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			
			SendDlgItemMessage(hDlg, IDC_RADIO_PLAY_START, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_RADIO_PLAY_STATE, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK_READ_ONLY, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	
			if((Controller_1_Type&1)==1)
				strcpy(Str_Tmp,"6 BUTTONS");
			else
				strcpy(Str_Tmp,"3 BUTTONS");
			SendDlgItemMessage(hDlg,IDC_STATIC_CON1_CURSET,WM_SETTEXT,0,(LPARAM)Str_Tmp);

			if((Controller_2_Type&1)==1)
				strcpy(Str_Tmp,"6 BUTTONS");
			else
				strcpy(Str_Tmp,"3 BUTTONS");
			SendDlgItemMessage(hDlg,IDC_STATIC_CON2_CURSET,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			strcpy(Str_Tmp,"");
			SendDlgItemMessage(hDlg,IDC_STATIC_SAVESTATEREQ,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			SendDlgItemMessage(hDlg,IDC_STATIC_3PLAYERS,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			InitMovie(&SubMovie);
			
			strncpy(Str_Tmp,Movie_Dir,512);
			strncat(Str_Tmp,Rom_Name,507);
			strcat(Str_Tmp,".gmv");
			SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_NAME,WM_SETTEXT,0,(LPARAM)Str_Tmp);

			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_EDIT_MOVIE_NAME:
					switch(HIWORD(wParam))
					{
					case EN_CHANGE:
						SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_NAME,WM_GETTEXT,(WPARAM)512,(LPARAM)Str_Tmp);
						GetMovieInfo(Str_Tmp,&SubMovie);
						if(SubMovie.Ok)
						{
							sprintf(Str_Tmp,"%d",SubMovie.LastFrame);
							SendDlgItemMessage(hDlg,IDC_STATIC_MOVIE_FRAME,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							sprintf(Str_Tmp,"%d",SubMovie.NbRerecords);
							SendDlgItemMessage(hDlg,IDC_STATIC_MOVIE_RERECORDS,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							if(SubMovie.Vfreq)
								sprintf(Str_Tmp,"%d",SubMovie.LastFrame/50/60);
							else
								sprintf(Str_Tmp,"%d",SubMovie.LastFrame/60/60);
							SendDlgItemMessage(hDlg,IDC_STATIC_MINUTES,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							
							if(SubMovie.Vfreq)
								sprintf(Str_Tmp,"%2.4f",(SubMovie.LastFrame%3000)/50.0);
							else
								sprintf(Str_Tmp,"%2.4f",(SubMovie.LastFrame%3600)/60.0);
							SendDlgItemMessage(hDlg,IDC_STATIC_SECONDS,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							SendDlgItemMessage(hDlg,IDC_STATIC_MOVIE_VERSION,WM_SETTEXT,0,(LPARAM)SubMovie.Header);
							SendDlgItemMessage(hDlg,IDC_STATIC_COMMENTS,WM_SETTEXT,0,(LPARAM)SubMovie.Note);
							switch(SubMovie.PlayerConfig[0])
							{
							case 3:
								strncpy(Str_Tmp,"3 BUTTONS",1024);
								break;
							case 6:
								strncpy(Str_Tmp,"6 BUTTONS",1024);
								break;
							default:
								strncpy(Str_Tmp,"UNKNOWN",1024);
								break;
							}
							SendDlgItemMessage(hDlg,IDC_STATIC_CON1_SET,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							switch(SubMovie.PlayerConfig[1])
							{
							case 3:
								strncpy(Str_Tmp,"3 BUTTONS",1024);
								break;
							case 6:
								strncpy(Str_Tmp,"6 BUTTONS",1024);
								break;
							default:
								strncpy(Str_Tmp,"UNKNOWN",1024);
								break;
							}
							SendDlgItemMessage(hDlg,IDC_STATIC_CON2_SET,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							Str_Tmp[0]=0;
							if(((SubMovie.PlayerConfig[0]!=3) && (SubMovie.PlayerConfig[0]!=6)) || ((SubMovie.PlayerConfig[1]!=3) && (SubMovie.PlayerConfig[1]!=6))) //Upth-Modif - since we now load controller settings from the GM
							{
								strcpy(Str_Tmp,"Warning: Controller settings cannot be loaded from movie."); //Upth-Modif - if GMV has unknown controller settings
								ShowWindow(GetDlgItem(hDlg,IDC_STATIC_WARNING_BITMAP),SW_SHOW);
							}
							else if((SubMovie.PlayerConfig[0]!=(((Controller_1_Type & 1) + 1 * 3))) || (SubMovie.PlayerConfig[1]!=(((Controller_2_Type & 1) + 1 * 3))) ) //Upth-Add - if controller settings differ
							{
								strcpy(Str_Tmp,"Controller settings will be loaded from movie.");
							}
							else
							{
								ShowWindow(GetDlgItem(hDlg,IDC_STATIC_WARNING_BITMAP),SW_HIDE);
							}
							SendDlgItemMessage(hDlg,IDC_STATIC_CON_WARNING,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							if(SubMovie.StateRequired)
								strcpy(Str_Tmp,"Savestate required !");
							else
								strcpy(Str_Tmp,"");
							SendDlgItemMessage(hDlg,IDC_STATIC_SAVESTATEREQ,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							if(SubMovie.TriplePlayerHack)
								strcpy(Str_Tmp,"3 players movie.  Configure controllers with Teamplay and 3 buttons setting");
							else
								strcpy(Str_Tmp,"");
							SendDlgItemMessage(hDlg,IDC_STATIC_3PLAYERS,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							if(SubMovie.Vfreq)
								strcpy(Str_Tmp,"50");
							else
								strcpy(Str_Tmp,"60");
							SendDlgItemMessage(hDlg,IDC_STATIC_VFRESH,WM_SETTEXT,0,(LPARAM)Str_Tmp);
						}
						if(SubMovie.Ok!=0 && (SubMovie.UseState==0 || SubMovie.StateOk!=0) && (SubMovie.StateRequired==0 || SubMovie.StateOk!=0))
							EnableWindow(GetDlgItem(hDlg,IDC_OK_PLAY ),TRUE);
						else
							EnableWindow(GetDlgItem(hDlg,IDC_OK_PLAY ),FALSE);
						SubMovie.ReadOnly=(int) (Def_Read_Only) | SubMovie.ReadOnly; //Upth-Add - for the new "Default Read Only" toggle
						if (SubMovie.ReadOnly) //Upth-Add - And we add a check or not depending on whether the movie is read only
							SendDlgItemMessage(hDlg, IDC_CHECK_READ_ONLY, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
						else 
							SendDlgItemMessage(hDlg, IDC_CHECK_READ_ONLY, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
						if(SubMovie.ReadOnly==0 && SubMovie.Ok!=0 && SubMovie.UseState!=0 && SubMovie.StateOk!=0)
							EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD ),TRUE);
						else
							EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD),FALSE);
						break;
					}
					return true;
					break;
				case IDC_EDIT_MOVIE_STATE:
					switch(HIWORD(wParam))
					{
					case EN_CHANGE:
						SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_STATE,WM_GETTEXT,(WPARAM)512,(LPARAM)Str_Tmp);
						GetStateInfo(Str_Tmp,&SubMovie);
						if(SubMovie.StateOk)
						{
							sprintf(Str_Tmp,"%d",SubMovie.StateFrame);
							SendDlgItemMessage(hDlg,IDC_STATIC_STATE_FRAME,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							if(SubMovie.StateFrame>SubMovie.LastFrame)
							{
								strcpy(Str_Tmp,"Warning: The savestate is after the end of the movie");
								SendDlgItemMessage(hDlg,IDC_STATIC_STATE_WARNING,WM_SETTEXT,0,(LPARAM)Str_Tmp);
								SubMovie.StateOk=0;
							}
							else
							{
								strcpy(Str_Tmp,"");
								SendDlgItemMessage(hDlg,IDC_STATIC_STATE_WARNING,WM_SETTEXT,0,(LPARAM)Str_Tmp);
							}
						}
						else
						{
							strcpy(Str_Tmp,"Warning: Invalid save state");
							SendDlgItemMessage(hDlg,IDC_STATIC_STATE_WARNING,WM_SETTEXT,0,(LPARAM)Str_Tmp);
						}
						break;
					}
					if(SubMovie.Ok!=0 && (SubMovie.UseState==0 || SubMovie.StateOk!=0) && (SubMovie.StateRequired==0 || SubMovie.UseState!=0))
						EnableWindow(GetDlgItem(hDlg,IDC_OK_PLAY ),TRUE);
					else
						EnableWindow(GetDlgItem(hDlg,IDC_OK_PLAY ),FALSE);
					if(SubMovie.ReadOnly==0 && SubMovie.Ok!=0 && SubMovie.UseState!=0 && SubMovie.StateOk!=0)
						EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD ),TRUE);
					else
						EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD),FALSE);
					SendDlgItemMessage(hDlg,IDC_RADIO_PLAY_STATE,BM_CLICK,0,0);
					return true;
					break;
				case IDC_RADIO_PLAY_START:
					SendDlgItemMessage(hDlg, IDC_RADIO_PLAY_START, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
					SendDlgItemMessage(hDlg, IDC_RADIO_PLAY_STATE, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					SubMovie.UseState=0;
					if(SubMovie.Ok!=0 && (SubMovie.UseState==0 || SubMovie.StateOk!=0)&& (SubMovie.StateRequired==0 || SubMovie.StateOk!=0))
						EnableWindow(GetDlgItem(hDlg,IDC_OK_PLAY ),TRUE);
					else
						EnableWindow(GetDlgItem(hDlg,IDC_OK_PLAY ),FALSE);
					if(SubMovie.ReadOnly!=0 && SubMovie.Ok!=0 && SubMovie.UseState!=0 && SubMovie.StateOk!=0)
						EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD ),TRUE);
					else
						EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD),FALSE);
					return true;
					break;
				case IDC_RADIO_PLAY_STATE:
					SendDlgItemMessage(hDlg, IDC_RADIO_PLAY_START, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					SendDlgItemMessage(hDlg, IDC_RADIO_PLAY_STATE, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
					SubMovie.UseState=1;
					if(SubMovie.Ok!=0 && (SubMovie.UseState==0 || SubMovie.StateOk!=0)&& (SubMovie.StateRequired==0 || SubMovie.StateOk!=0))
						EnableWindow(GetDlgItem(hDlg,IDC_OK_PLAY ),TRUE);
					else
						EnableWindow(GetDlgItem(hDlg,IDC_OK_PLAY ),FALSE);
					if(SubMovie.ReadOnly==0 && SubMovie.Ok!=0 && SubMovie.UseState!=0 && SubMovie.StateOk!=0)
						EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD ),TRUE);
					else
						EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD),FALSE);
					return true;
					break;
				case IDC_BUTTON_BROWSE_MOVIE:
					strcpy(Str_Tmp,"");
					DialogsOpen++;
					if(Change_File_L(Str_Tmp, Movie_Dir, "Load Movie", "GENs Movie\0*.gmv*\0All Files\0*.*\0\0", "gmv"))
					{
							SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_NAME,WM_SETTEXT,0,(LPARAM)Str_Tmp);
					}
					DialogsOpen--;
					return true;
					break;
				case IDC_BUTTON_STATE_BROWSE:
					strcpy(Str_Tmp,"");
					DialogsOpen++;
					if(Change_File_L(Str_Tmp, Movie_Dir, "Load state", "State Files\0*.gs*\0All Files\0*.*\0\0", "gs0"))
					{
							SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_STATE,WM_SETTEXT,0,(LPARAM)Str_Tmp);
					}
					DialogsOpen--;
					return true;
					break;
				case IDC_CHECK_READ_ONLY:
					if(SubMovie.ReadOnly)
					{
						SubMovie.ReadOnly=0;
						SendDlgItemMessage(hDlg, IDC_CHECK_READ_ONLY, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
					}
					else
					{
						SubMovie.ReadOnly=1;
						SendDlgItemMessage(hDlg, IDC_CHECK_READ_ONLY, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
					}
					if(SubMovie.ReadOnly==0 && SubMovie.Ok!=0 && SubMovie.UseState!=0 && SubMovie.StateOk!=0)
						EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD ),TRUE);
					else
						EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD),FALSE);
					return true;
					break;
				case IDC_OK_PLAY:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					if(SubMovie.Ok)
					{
						if(MainMovie.Status) //Modif N - make sure to close existing movie, if any
							CloseMovieFile(&MainMovie);
						MainMovie.Status=0;
						CopyMovie(&SubMovie,&MainMovie);

						//UpthAdd - Load Controller Settings from Movie File
						if (MainMovie.TriplePlayerHack) {
							Controller_1_Type |= 0x10;
							Controller_1B_Type &= ~1;
							Controller_1C_Type &= ~1;
						}
						else Controller_1_Type &= ~0x10;
						Controller_2_Type &= ~0x10;
						if (MainMovie.PlayerConfig[0]==3) Controller_1_Type &= ~1;
						if (MainMovie.PlayerConfig[0]==6) Controller_1_Type |= 1;
						if (MainMovie.PlayerConfig[1]==3) Controller_2_Type &= ~1;
						if (MainMovie.PlayerConfig[1]==6) Controller_2_Type |= 1;
						Track1_FrameCount = MainMovie.LastFrame;
						Track2_FrameCount = MainMovie.LastFrame;
						Track3_FrameCount = MainMovie.LastFrame;
						Make_IO_Table();
						MainMovie.Status=MOVIE_PLAYING;
						PlayMovieCanceled=0;
					}
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
				case IDC_BUTTON_RESUME_RECORD:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					if(SubMovie.Ok)
					{
						if(MainMovie.Status) //Modif N - make sure to close existing movie, if any
							CloseMovieFile(&MainMovie);
						MainMovie.Status=0;

						CopyMovie(&SubMovie,&MainMovie);
						MainMovie.Status=MOVIE_RECORDING;
						PlayMovieCanceled=0;
					}
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
				case IDC_CANCEL_PLAY:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					PlayMovieCanceled=1;
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			PlayMovieCanceled=1;
			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}
void DoMovieSplice() //Splices saved input back into the movie file
{
	FILE *TempSplice = fopen(TempName,"r+b");
	if (!TempSplice)
	{
		MessageBox(NULL,"Error opening temporary file","ERROR",MB_OK | MB_ICONWARNING);
		return;
	}
	fseek(TempSplice,0,SEEK_END);
	unsigned long size = ftell(TempSplice);
	MainMovie.LastFrame++;
	fseek(MainMovie.File,(MainMovie.LastFrame * 3) + 64,SEEK_SET);
	char *TempBuffer = (char *) malloc(size);
	fseek(TempSplice,0,SEEK_SET);
	fread(TempBuffer,1,size,TempSplice);
	fwrite(TempBuffer,1,size,MainMovie.File);
	free(TempBuffer);
	if (MainMovie.Status == MOVIE_RECORDING) Put_Info("Movie successfully spliced. Resuming playback from now.",2000);
	else Put_Info("Movie successfully spliced.",2000);
	MainMovie.Status = MOVIE_PLAYING;
	fseek(MainMovie.File,64,SEEK_END);
	MainMovie.LastFrame = (ftell(MainMovie.File) / 3);
	SpliceFrame = 0;
	char cfgFile[1024];
	strcpy(cfgFile, Gens_Path);
	strcat(cfgFile, "Gens.cfg");
	WritePrivateProfileString("Splice","SpliceMovie","",cfgFile);
	WritePrivateProfileString("Splice","SpliceFrame","0",cfgFile);
	WritePrivateProfileString("Splice","TempFile","",cfgFile);
	fclose(TempSplice);
	remove(TempName);
	free(TempName);
}
LRESULT CALLBACK PromptSpliceFrameProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //saves all the input from specified frame to a tempfile, so a prior section can be redone
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			//SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			strcpy(Str_Tmp,"Enter the frame you wish to rerecord to.");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			strcpy(Str_Tmp,"When finished, use Tools->Movie->Input Splice.");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT2,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					TempName = tempnam(Gens_Path,"GMtmp");
					FILE *TempSplice = fopen(TempName,"w+b");
					if (TempSplice == NULL)
					{
						MessageBox(NULL,"Error creating temporary file!","Operation canceled",MB_OK | MB_ICONWARNING);
						DialogsOpen--;
						EndDialog(hDlg, true);
						return false;
						break;
					}
					SpliceFrame = GetDlgItemInt(hDlg,IDC_PROMPT_EDIT,NULL,false);
					fseek(MainMovie.File,0,SEEK_END);
                    unsigned long size = ftell(MainMovie.File);
					fseek(MainMovie.File,(((SpliceFrame - 1) * 3) + 64),SEEK_SET);
					size -= ftell(MainMovie.File);
					char *TempBuffer = (char *) malloc(size);
					fread(TempBuffer,1,size,MainMovie.File);
					fwrite(TempBuffer,1,size,TempSplice);
					free(TempBuffer);
					sprintf(Str_Tmp,"Rerecording to frame #%d",SpliceFrame);
					{
						strncpy(Str_Tmp,MainMovie.FileName,512);
						MainMovie.FileName[strlen(MainMovie.FileName)-3]='\0';
						strcat(MainMovie.FileName,"spl.gmv");
						BackupMovieFile(&MainMovie);
						strncpy(MainMovie.FileName,Str_Tmp,512);
					}
					Put_Info(Str_Tmp,2000);
					char cfgFile[1024];
					strcpy(cfgFile, Gens_Path);
					strcat(cfgFile, "Gens.cfg");
					strcpy(SpliceMovie,MainMovie.FileName);
					WritePrivateProfileString("Splice","SpliceMovie",SpliceMovie,cfgFile);
					sprintf(Str_Tmp,"%d",SpliceFrame);
					WritePrivateProfileString("Splice","SpliceFrame",Str_Tmp,cfgFile);
					WritePrivateProfileString("Splice","TempFile",TempName,cfgFile);
					fclose(TempSplice);
					MustUpdateMenu = 1;
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
				}
				case ID_CANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}

					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}
LRESULT CALLBACK PromptSeekFrameProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //Gets seek frame target, and starts frameseek
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			//SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			strcpy(Str_Tmp,"Enter the frame you wish to seek to.");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			strcpy(Str_Tmp,"");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT2,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					SeekFrame = GetDlgItemInt(hDlg,IDC_PROMPT_EDIT,NULL,false);
					sprintf(Str_Tmp,"Seeking to frame %d",SeekFrame);
					Put_Info(Str_Tmp,2000);
					MustUpdateMenu = 1;
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
				}
				case ID_CANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}

					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}
LRESULT CALLBACK EditWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //Gets info for a RAM Watch, and then inserts it into the Watch List
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static int index;
	static char s,t = s = 0;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			//SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			index = (int)lParam;
			sprintf(Str_Tmp,"%08X",rswatches[index].Address);
			SetDlgItemText(hDlg,IDC_EDIT_COMPAREADDRESS,Str_Tmp);
			if (rsaddrs[rswatches[index].Index].comment != NULL)
				SetDlgItemText(hDlg,IDC_PROMPT_EDIT,rsaddrs[rswatches[index].Index].comment);
			s = rswatches[index].Size;
			t = rswatches[index].Type;
			switch (s)
			{
				case 'b':
					SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'w':
					SendDlgItemMessage(hDlg, IDC_2_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'd':
					SendDlgItemMessage(hDlg, IDC_4_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
				default:
					s = 0;
					break;
			}
			switch (t)
			{
				case 's':
					SendDlgItemMessage(hDlg, IDC_SIGNED, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'u':
					SendDlgItemMessage(hDlg, IDC_UNSIGNED, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'h':
					SendDlgItemMessage(hDlg, IDC_HEX, BM_SETCHECK, BST_CHECKED, 0);
					break;
				default:
					t = 0;
					break;
			}
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_SIGNED:
					t='s';
					return true;
				case IDC_UNSIGNED:
					t='u';
					return true;
				case IDC_HEX:
					t='h';
					return true;
				case IDC_1_BYTE:
					s = 'b';
					return true;
				case IDC_2_BYTES:
					s = 'w';
					return true;
				case IDC_4_BYTES:
					s = 'd';
					return true;
				case IDOK:
				{
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					if (s && t)
					{
						AddressWatcher Temp;
						Temp.Size = s;
						Temp.Type = t;
						Temp.WrongEndian = false; //replace this when I get little endian working properly
						GetDlgItemText(hDlg,IDC_EDIT_COMPAREADDRESS,Str_Tmp,1024);
						char *addrstr = Str_Tmp;
						if (strlen(Str_Tmp) > 8) addrstr = &(Str_Tmp[strlen(Str_Tmp) - 9]);
						sscanf(addrstr,"%08X",&(Temp.Address));
						Temp.Index = getSearchIndex(Temp.Address,0,CUR_RAM_MAX);
						if (Temp.Index > -1)
						{
							GetDlgItemText(hDlg,IDC_PROMPT_EDIT,Str_Tmp,80);
							if (index < WatchCount) RemoveWatch(index);
							InsertWatch(Temp,Str_Tmp);
							if(RamWatchHWnd)
							{
								ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
							}
							DialogsOpen--;
							EndDialog(hDlg, true);
						}
						else 
							MessageBox(NULL,"Error: Invalid Address.","ERROR",MB_OK);
					}
					else
					{
						strcpy(Str_Tmp,"Error:");
						if (!s)
							strcat(Str_Tmp," Size must be specified.");
						if (!t)
							strcat(Str_Tmp," Type must be specified.");
						MessageBox(NULL,Str_Tmp,"ERROR",MB_OK);
					}
					return true;
					break;
				}
				case ID_CANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					DialogsOpen--;
					EndDialog(hDlg, false);
					return false;
					break;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			EndDialog(hDlg, false);
			return false;
			break;
	}

	return false;
}
LRESULT CALLBACK PromptWatchNameProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //Gets the description of a watched address
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			//SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			strcpy(Str_Tmp,"Enter a name for this RAM address.");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			strcpy(Str_Tmp,"");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT2,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					GetDlgItemText(hDlg,IDC_PROMPT_EDIT,Str_Tmp,80);
					AddressWatcher Temp;
					Temp = rswatches[WatchCount];
					InsertWatch(Temp,Str_Tmp);
					if(RamWatchHWnd)
					{
						ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
					}
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
				}
				case ID_CANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}

					DialogsOpen--;
					EndDialog(hDlg, false);
					return false;
					break;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			EndDialog(hDlg, false);
			return false;
			break;
	}

	return false;
}
LRESULT CALLBACK RecordMovieProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			//SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
				
			if((Controller_1_Type&1)==1)
				strcpy(Str_Tmp,"6 BUTTONS");
			else
				strcpy(Str_Tmp,"3 BUTTONS");
			SendDlgItemMessage(hDlg,IDC_STATIC_CON1_CURSET,WM_SETTEXT,0,(LPARAM)Str_Tmp);

			if((Controller_2_Type&1)==1)
				strcpy(Str_Tmp,"6 BUTTONS");
			else
				strcpy(Str_Tmp,"3 BUTTONS");
			SendDlgItemMessage(hDlg,IDC_STATIC_CON2_CURSET,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			strncpy(Str_Tmp,Movie_Dir,512);
			strncat(Str_Tmp,Rom_Name,507);
			strcat(Str_Tmp,".gmv");
			SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_NAME,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			SendDlgItemMessage(hDlg,IDC_CHECK_3PLAYER,BM_SETCHECK,BST_UNCHECKED,0);
			SendDlgItemMessage(hDlg,IDC_CHECK_SAVESTATEREQ,BM_SETCHECK,BST_UNCHECKED,0);
			InitMovie(&SubMovie);
			RecordMovieCanceled=1;
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_EDIT_MOVIE_NAME:
					switch(HIWORD(wParam))
					{
					case EN_CHANGE:
						SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_NAME,WM_GETTEXT,(WPARAM)512,(LPARAM)Str_Tmp);
						if(Str_Tmp[0])
							EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD ),TRUE);
						else
							EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_RESUME_RECORD),FALSE);
						GetMovieInfo(Str_Tmp,&SubMovie);
						Str_Tmp[0]=0;
						if(SubMovie.Ok)
							strcpy(Str_Tmp,"Warning: File already exists");
						SendDlgItemMessage(hDlg,IDC_STATIC_WARNING_EXIST,WM_SETTEXT,0,(LPARAM)Str_Tmp);
						break;
					}
					return true;
					break;
				case IDC_BUTTON_BROWSE_MOVIE:
					strncpy(Str_Tmp,Rom_Name,512);
					//strcat(Str_Tmp,".gmv");
					if(Change_File_S(Str_Tmp, Movie_Dir, "Save Movie", "GENs Movie\0*.gmv\0All Files\0*.*\0\0", "gmv"))
					{
							SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_NAME,WM_SETTEXT,0,(LPARAM)Str_Tmp);
					}
					return true;
					break;
				case IDC_OK_RECORD:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
                   
					if(MainMovie.File != NULL) //Modif N - make sure to close existing movie, if any
						CloseMovieFile(&MainMovie);
					MainMovie.Status=0;

					InitMovie(&MainMovie);
					MainMovie.ReadOnly=0;
					MainMovie.Type = TYPEGMV;
					MainMovie.Version='A'-'0';
					SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_NAME,WM_GETTEXT,(WPARAM)512,(LPARAM)Str_Tmp);
					strncpy(MainMovie.FileName,Str_Tmp,512);
					SendDlgItemMessage(hDlg,IDC_EDIT_NOTE,WM_GETTEXT,(WPARAM)512,(LPARAM)Str_Tmp);
					strncpy(MainMovie.Note,Str_Tmp,40);
					MainMovie.Note[40]=0;
					strncpy(MainMovie.Header,"Gens Movie TESTA",17);
					MainMovie.Status=MOVIE_RECORDING;
					RecordMovieCanceled=0;
					if((Controller_1_Type&1)==1)
						MainMovie.PlayerConfig[0]=6;
					else
						MainMovie.PlayerConfig[0]=3;
					if((Controller_2_Type&1)==1)
						MainMovie.PlayerConfig[1]=6;
					else
						MainMovie.PlayerConfig[1]=3;
					if(SendDlgItemMessage(hDlg,IDC_CHECK_3PLAYER,BM_GETCHECK,BST_UNCHECKED,0)==BST_CHECKED)
					{
						MainMovie.TriplePlayerHack=1;
					}
					if(SendDlgItemMessage(hDlg,IDC_CHECK_SAVESTATEREQ,BM_GETCHECK,BST_UNCHECKED,0)==BST_CHECKED)
					{
						MainMovie.StateRequired=1;
					}
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
				case IDC_CANCEL_RECORD:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					RecordMovieCanceled=1;
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			RecordMovieCanceled=1;
			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}


void init_list_box(HWND Box, char Strs[][11], int numColumns, int *columnWidths) //initializes the ram search and/or ram watch listbox
{
	LVCOLUMN Col;
	Col.mask = LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	Col.fmt = LVCFMT_CENTER;
	for (int i = 0; i < numColumns; i++)
	{
		Col.iOrder = i;
		Col.iSubItem = i;
		Col.pszText = Strs[i];
		Col.cx = columnWidths[i];
		ListView_InsertColumn(Box,i,&Col);
	}

	ListView_SetExtendedListViewStyle(Box, LVS_EX_FULLROWSELECT);
}
void Update_RAM_Search() //keeps RAM values up to date in the search and watch windows
{
	static DWORD previousTime = timeGetTime(); //Modif N - give frame advance sound:
	if ((timeGetTime() - previousTime) < 0x40) return;
	int watchChanges[128];
	int changes[128];
	if(RamWatchHWnd)
	{
		// prepare to detect changes to displayed listview items
		HWND lv = GetDlgItem(RamWatchHWnd,IDC_WATCHLIST);
		int top = ListView_GetTopIndex(lv);
		int count = ListView_GetCountPerPage(lv);
		int i;
		for(i = top; i <= top+count; i++)
			watchChanges[i-top] = (int)sizeConv(rswatches[i].Index,rswatches[i].Size);
	}
	if(RamSearchHWnd)
	{
		// prepare to detect changes to displayed listview items
		HWND lv = GetDlgItem(RamSearchHWnd,IDC_RAMLIST);
		int top = ListView_GetTopIndex(lv);
		int count = ListView_GetCountPerPage(lv);
		int i;
		for(i = top; i <= top+count; i++)
			changes[i-top] = rsaddrs[rsresults[i].Index].changes;
	}
	if (RamSearchHWnd || RamWatchHWnd)
	{
		// update all RAM values
		signal_new_frame();
	}
	if (AutoSearch)
	{
		int SetPaused = (!(Paused == 1));
		if (SetPaused) Paused = 1;
		//Clear_Sound_Buffer();
		prune(rs_c,rs_o,rs_t,rs_val,rs_param);

		if(!ResultCount)
		{
			reset_address_info();
			prune(rs_c,rs_o,rs_t,rs_val,rs_param);
			if(ResultCount && rs_c != 'a')
				MessageBox(NULL,"Performing search on all addresses.","Out of results.",MB_OK|MB_ICONINFORMATION);
		}
		if (RamSearchHWnd) ListView_SetItemCount(GetDlgItem(RamSearchHWnd,IDC_RAMLIST),ResultCount);
		if (SetPaused) Paused = 0;
	}
	if (RamSearchHWnd)
	{
		HWND lv = GetDlgItem(RamSearchHWnd,IDC_RAMLIST);
		int top = ListView_GetTopIndex(lv);
		int count = ListView_GetCountPerPage(lv)+1;
		int i;
		// refresh any visible parts of the listview box that changed
		int start = -1;
		bool changed = false;
		for(i = top; i <= top+count; i++)
		{
			if(start == -1)
			{
				if((i != top+count) && (changes[i-top] != rsaddrs[rsresults[i].Index].changes))
				{
					start = i;
					if (!changed)
						changed = true;
				}
			}
			else
			{
				if((i == top+count) || (changes[i-top] == rsaddrs[rsresults[i].Index].changes))
				{
					ListView_RedrawItems(lv, start, i-1);
					start = -1;
				}
			}
		}
		if (changed)
			UpdateWindow(lv);
	}
	if (RamWatchHWnd)
	{
		HWND lv = GetDlgItem(RamWatchHWnd,IDC_WATCHLIST);
		int top = ListView_GetTopIndex(lv);
		int count = ListView_GetCountPerPage(lv)+1;
		int i;
		// refresh any visible parts of the listview box that changed
		int start = -1;
		bool changed = false;
		for(i = top; i <= top+count; i++)
		{
			if(start == -1)
			{
				if((i != top+count) && (watchChanges[i-top] != (int)sizeConv(rswatches[i].Index,rswatches[i].Size)))
				{
					start = i;
					if (!changed)
						changed = true;
				}
			}
			else
			{
				if((i == top+count) || (watchChanges[i-top] == (int)sizeConv(rswatches[i].Index,rswatches[i].Size)))
				{
					ListView_RedrawItems(lv, start, i-1);
					start = -1;
				}
			}
		}
		UpdateWindow(lv);
	}
	previousTime = timeGetTime();
}
LRESULT CALLBACK RamWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static int watchIndex=0;

	switch(uMsg)
	{
		case WM_INITDIALOG: {
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			// push it away from the main window if we can
			const int width = (r.right-r.left); 
			const int width2 = (r2.right-r2.left); 
			if(r.left+width2 + width < GetSystemMetrics(SM_CXSCREEN))
			{
				r.right += width;
				r.left += width;
			}
			else if((int)r.left - (int)width2 > 0)
			{
				r.right -= width2;
				r.left -= width2;
			}

			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

			char names[3][11] = {"Address","Value","Notes"};
			int widths[3] = {62,64,64+51+53};
			init_list_box(GetDlgItem(hDlg,IDC_WATCHLIST),names,3,widths);
			if (!ResultCount)
				reset_address_info();
			else
				signal_new_frame();
			ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);
			if (!noMisalign) SendDlgItemMessage(hDlg, IDC_MISALIGN, BM_SETCHECK, BST_CHECKED, 0);
			if (littleEndian) SendDlgItemMessage(hDlg, IDC_ENDIAN, BM_SETCHECK, BST_CHECKED, 0);
			return true;
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR lP = (LPNMHDR) lParam;
			switch (lP->code)
			{
				case LVN_GETDISPINFO:
				{
					LV_DISPINFO *Item = (LV_DISPINFO *)lParam;
					Item->item.mask = LVIF_TEXT;
					Item->item.state = 0;
					Item->item.iImage = 0;
					const unsigned int iNum = Item->item.iItem;
					static char num[11];
					switch (Item->item.iSubItem)
					{
						case 0:
							sprintf(num,"%08X",rswatches[iNum].Address);
							Item->item.pszText = num;
							return true;
						case 1: {
							int i;
							int size;
							switch (rswatches[iNum].Size)
							{

								case 'w':
									size = 2;
									break;
								case 'd':
									size = 4;
									break;
								case 'b':
								default:
									size = 1;
									break;									
							}
							i = sizeConv(rswatches[Item->item.iItem].Index,rswatches[Item->item.iItem].Size);
//							for (int j = 1; j < size; j++)
//								i |= rsaddrs[rswatches[Item->item.iItem].Index + j].cur << (8 * j);
//							Byte_Swap(&i,size);
							sprintf(num,
								((rswatches[iNum].Type == 's')?
									"%d":
									(rswatches[iNum].Type == 'u')?
										"%u":
										"%X"),
								(rswatches[iNum].Type != 's')?
									i:
									((rswatches[iNum].Size == 'b')?
										(char)(i&0xff):
										(rswatches[iNum].Size == 'w')?
											(short)(i&0xffff):
											(long)(i&0xffffffff)));
							Item->item.pszText = num;
						}	return true;
						case 2:
							Item->item.pszText = rsaddrs[rswatches[iNum].Index].comment ? rsaddrs[rswatches[iNum].Index].comment : "";
							return true;

						default:
							return false;
					}
				}
				//case LVN_ODCACHEHINT: //Copied this bit from the MSDN virtual listbox code sample. Eventually it should probably do something.
				//{
				//	LPNMLVCACHEHINT   lpCacheHint = (LPNMLVCACHEHINT)lParam;
				//	return 0;
				//}
				//case LVN_ODFINDITEM: //Copied this bit from the MSDN virtual listbox code sample. Eventually it should probably do something.
				//{	
				//	LPNMLVFINDITEM lpFindItem = (LPNMLVFINDITEM)lParam;
				//	return 0;
				//}
			}
		}

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_C_SAVE:
					return Save_Watches();
				case IDC_C_LOAD:
					return Load_Watches();
				case IDC_C_RESET:
					ResetWatches();
					return true;
				case IDC_C_SEARCH:
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					RemoveWatch(watchIndex);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);					
					return true;
				case IDC_C_WATCH_EDIT:
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDITWATCH), hDlg, (DLGPROC) EditWatchProc,(LPARAM) watchIndex);
					return true;
				case IDC_C_WATCH:
					rswatches[WatchCount].Address = rswatches[WatchCount].Index = rswatches[WatchCount].WrongEndian = 0;
					rswatches[WatchCount].Size = 'b';
					rswatches[WatchCount].Type = 's';
					DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDITWATCH), hDlg, (DLGPROC) EditWatchProc,(LPARAM) WatchCount);
					return true;
				case IDC_C_WATCH2:
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					rswatches[WatchCount].Address = rswatches[watchIndex].Address;
					rswatches[WatchCount].Index = rswatches[watchIndex].Index;
					rswatches[WatchCount].WrongEndian = rswatches[watchIndex].WrongEndian;
					rswatches[WatchCount].Size = rswatches[watchIndex].Size;
					rswatches[WatchCount].Type = rswatches[watchIndex].Type;
					DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDITWATCH), hDlg, (DLGPROC) EditWatchProc,(LPARAM) WatchCount);
					return true;
				case IDC_C_WATCH_UP:
				{
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					if (watchIndex == 0)
						return true;
					void *tmp = malloc(sizeof(AddressWatcher));
					memcpy(tmp,&(rswatches[watchIndex]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex]),&(rswatches[watchIndex - 1]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex - 1]),tmp,sizeof(AddressWatcher));
					free(tmp);
					ListView_SetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex-1);
					ListView_SetItemState(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex-1,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);
					return true;
				}
				case IDC_C_WATCH_DOWN:
				{
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					if (watchIndex >= WatchCount - 1)
						return true;
					void *tmp = malloc(sizeof(AddressWatcher));
					memcpy(tmp,&(rswatches[watchIndex]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex]),&(rswatches[watchIndex + 1]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex + 1]),tmp,sizeof(AddressWatcher));
					free(tmp);
					ListView_SetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex+1);
					ListView_SetItemState(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex+1,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);
					return true;
				}
				case IDC_C_ADDCHEAT:
				{
//					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST)) | (1 << 24);
//					DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDITCHEAT), hDlg, (DLGPROC) EditCheatProc,(LPARAM) searchIndex);
				}
				case IDOK:
				case IDCANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					DialogsOpen--;
					RamWatchHWnd = NULL;
					EndDialog(hDlg, true);
					return true;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			RamWatchHWnd = NULL;
			EndDialog(hDlg, true);
			return true;
	}

	return false;
}
template <typename T>
T CheatRead (unsigned int address)
{
	T val = 0;
	for(int i = 0 ; i < sizeof(T) ; i++)
		val <<= 8, val |= (T)(M68K_RB(address+i));
	return val;
}
template <typename T>
void CheatWrite (unsigned int address, T value)
{
	for(int i = sizeof(T) - 1; i >=0; i--)
	{
		M68K_WBC(address++, (unsigned char) ((value >> (i<<3))&0xff));
	}
}
#ifdef MAPHACK
#define POWEROFTWO 1
#define SCROLLSPEED 8
void Update_RAM_Cheats()
{
	static const int ratio = 1 << POWEROFTWO;
	// XXX: camhack / maphack, sonic 3
	if(!GetKeyState(VK_SCROLL))
		return;

	bool ksU = (GetAsyncKeyState('I')&0x8000)!=0, ksD = (GetAsyncKeyState('K')&0x8000)!=0, ksL = (GetAsyncKeyState('J')&0x8000)!=0, ksR = (GetAsyncKeyState('L')&0x8000)!=0;
	static bool ksUPrev = ksU, ksDPrev = ksD, ksLPrev = ksL, ksRPrev = ksR;

	static int snapCount = 0;
	static bool snapPast = true;
	static bool sDown = false;
	bool keepgoing = false;
	if(x==xg && y==yg)
	{
		if(!snapPast)
		{
			snapCount++;
			if(snapCount > 3)
			{
				snapCount = 0;
				snapPast = true;
				keepgoing = GetKeyState(VK_CAPITAL) != 0;
			}
		}
	}
	else
	{
		snapCount = 0;
		snapPast = false;
	}
		static const int X = 13824;
		static const int Y = 2048;

	if(!GetKeyState(VK_NUMLOCK))
	{

	// camera movement, IJKL
	if(ksL && (/*!ksLPrev ||*/ snapPast) && xg >  0) xg -= 320/2, xg -= xg % (320/2), snapPast = false;
	if(ksR && (/*!ksRPrev ||*/ snapPast) && xg+160<X*ratio) xg += 320/2, xg -= xg % (320/2), snapPast = false;
	if(ksU && (/*!ksUPrev ||*/ snapPast) && yg >= 0) yg -= 112, yg -= yg % 112, snapPast = false;
	if(ksD && (/*!ksDPrev ||*/ snapPast) && yg+112<Y*ratio) yg += 112, yg -= yg % 112, snapPast = false;
	
	}

	ksUPrev = ksU, ksDPrev = ksD, ksLPrev = ksL, ksRPrev = ksR;

	if(GetAsyncKeyState(VK_OEM_PERIOD))
	{
		//xg = CheatRead<unsigned short>(0xD008) - 160; // reset camera
		//yg = CheatRead<unsigned short>(0xD00C) - 112; // reset camera
		xg = (CheatRead<unsigned short>(0xFFB010) & 0xffffff)-160; // reset camera
		yg = (CheatRead<unsigned short>(0xFFB014) & 0xffffff)-120; // reset camera
		snapPast = false;
		snapCount = 0;
	}

	/*if(GetKeyState(VK_NUMLOCK))
	{
		sDown = false;
		x = CheatRead<unsigned long>(0xFFEE78); // get camera position x
		y = CheatRead<unsigned long>(0xFFEE78+4); // get camera position y
		CheatWrite<unsigned short>(0xFFEE0B, 0); // unlock camera
	}
	else*/ if(!keepgoing)
	{
		if(x != xg)
		{
			if(abs(x - xg) <= SCROLLSPEED)
				x = xg;
			//else if(abs(x - xg) >= 640)
			//{
			//	if(x < xg) x = xg - 320;
			//	else if(x > xg) x = xg + 320;
			//}
			else
			{
				if(x < xg) x += SCROLLSPEED;
				else if(x > xg) x -= SCROLLSPEED;
			}
		}
		if(y != yg)
		{
			if(abs(y - yg) <= SCROLLSPEED)
				y = yg;
			//else if(abs(y - yg) >= 240)
			//{
			//	if(y < yg) y = yg - 120;
			//	else if(y > yg) y = yg + 120;
			//}
			else
			{
				if(y < yg) y += SCROLLSPEED;
				else if(y > yg) y -= SCROLLSPEED;
			}
		}

		if(x < 0)
			x = 0; // prevent crash
		if(xg < 0)
			xg = 0;

		//CheatWrite<unsigned short>(0xFFF700, x);
		CheatWrite<unsigned short>(0xFFEE78, (unsigned short)x); // set camera position x
		CheatWrite<unsigned short>(0xFFEE80, (unsigned short)x);
//		CheatWrite<unsigned long>(0xFFA80C, x);
//		CheatWrite<unsigned long>(0xFFA814, x);
//		CheatWrite<unsigned long>(0xFFA818, x);
		//CheatWrite<unsigned short>(0xFFF704, y);
		CheatWrite<unsigned short>(0xFFEE78+4, (unsigned short)y);  // set camera position y
		CheatWrite<unsigned short>(0xFFEE80+4, (unsigned short)y);
//		CheatWrite<unsigned long>(0xFFA80C+4, y);
//		CheatWrite<unsigned long>(0xFFA814+4, y);
//		CheatWrite<unsigned long>(0xFFA818+4, y);
		if(GetAsyncKeyState(VK_OEM_COMMA))
		{
			//CheatWrite(0xFFD008,x+160);
			CheatWrite<short>(0xFFB010, (short)x+160);
			//CheatWrite(0xFFD00C,y+112);
			CheatWrite<short>(0xFFB014, (short)y+120);
		}
		//CheatWrite<unsigned short>(0xFFB004, CheatRead<unsigned short>(0xB004) & ~0x4); // no death
		//CheatWrite<unsigned short>(0xFFB00B, CheatRead<unsigned short>(0xB00B) & ~0x80); // no death
//		CheatWrite<unsigned short>(0xFFEE0B, 0); // no camera lock on death
		CheatWrite<unsigned short>(0xFFEE0A, 0x100); // yes camera lock always!
	//	CheatWrite<unsigned short>(0xFFFFCE, 0x86A0); // no camera lock(?)

		//CheatWrite<unsigned char>(0xFFB06F, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFB1E1, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFB353, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFB39D, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFB3E7, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFB47B, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFB50F, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFB559, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFB6CB, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFB75F, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFCC2F, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFCCC3, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFCD0A, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFEE26, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFF634, 0); // no animation?
		//CheatWrite<unsigned char>(0xFFF654, 0); // no animation?

		CheatWrite<unsigned char>(0xFFF653, 0); // no burning palette animation
		//CheatWrite<unsigned char>(0xFFFEB2, 0); // no ring animation
		//memset(&(Ram_68k[0xFE70]),0,0x22);
		memset(&(Ram_68k[0xFE70]),0,0x40);	// hold oscilators
	}

	// XXX: screenshot, for map capture
	{
		if(GetAsyncKeyState('S'))
		{
			if(!sDown)
				keepgoing = true;
			sDown = true;
		}
		else
			sDown = false;
		if(!keepgoing)
			return;
		int i, j, tmp, offs, num = -1;
		unsigned long BW;

		SetCurrentDirectory(Gens_Path);

		i = (X * Y * 3) + 54;

		if (!Game) return;

		static unsigned char *DestFull = NULL;
		if(!DestFull)
		{
			DestFull = (unsigned char *) malloc(i);
			memset(DestFull, 0, i);

			// write BMP header
			{
				DestFull[0] = 'B';
				DestFull[1] = 'M';

				DestFull[2] = (unsigned char) ((i >> 0) & 0xFF);
				DestFull[3] = (unsigned char) ((i >> 8) & 0xFF);
				DestFull[4] = (unsigned char) ((i >> 16) & 0xFF);
				DestFull[5] = (unsigned char) ((i >> 24) & 0xFF);

				DestFull[6] = DestFull[7] = DestFull[8] = DestFull[9] = 0;

				DestFull[10] = 54;
				DestFull[11] = DestFull[12] = DestFull[13] = 0;

				DestFull[14] = 40;
				DestFull[15] = DestFull[16] = DestFull[17] = 0;

				DestFull[18] = (unsigned char) ((X >> 0) & 0xFF);
				DestFull[19] = (unsigned char) ((X >> 8) & 0xFF);
				DestFull[20] = (unsigned char) ((X >> 16) & 0xFF);
				DestFull[21] = (unsigned char) ((X >> 24) & 0xFF);

				DestFull[22] = (unsigned char) ((Y >> 0) & 0xFF);
				DestFull[23] = (unsigned char) ((Y >> 8) & 0xFF);
				DestFull[24] = (unsigned char) ((Y >> 16) & 0xFF);
				DestFull[25] = (unsigned char) ((Y >> 24) & 0xFF);

				DestFull[26] = 1;
				DestFull[27] = 0;
				
				DestFull[28] = 24;
				DestFull[29] = 0;

				DestFull[30] = DestFull[31] = DestFull[32] = DestFull[33] = 0;

				i -= 54;
				
				DestFull[34] = (unsigned char) ((i >> 0) & 0xFF);
				DestFull[35] = (unsigned char) ((i >> 8) & 0xFF);
				DestFull[36] = (unsigned char) ((i >> 16) & 0xFF);
				DestFull[37] = (unsigned char) ((i >> 24) & 0xFF);

				DestFull[38] = DestFull[42] = 0xC4;
				DestFull[39] = DestFull[43] = 0x0E;
				DestFull[40] = DestFull[44] = DestFull[41] = DestFull[45] = 0;

				DestFull[46] = DestFull[47] = DestFull[48] = DestFull[49] = 0;
				DestFull[50] = DestFull[51] = DestFull[52] = DestFull[53] = 0;
			}
		}
		if(!DestFull)
			return;

		int mode = (Mode_555 & 1);
		int Hmode = (VDP_Reg.Set4 & 0x01);
		int Vmode = (VDP_Reg.Set2 & 0x08);

		unsigned char *Src = (Bits32?(unsigned char *)(MD_Screen32):(unsigned char *)(MD_Screen));
		if(Vmode)
			Src += 336 * 239 * 2 * (Bits32?2:1);
		else
			Src += 336 * 223 * 2 * (Bits32?2:1);

		unsigned char *Dest = (unsigned char *)(DestFull) + 54;
		unsigned short WaterY = CheatRead<unsigned short>(0xFFF64A) >> POWEROFTWO;
		if (CheatRead<unsigned short>(0xFFF648) > CheatRead<unsigned short>(0xFFF646)) WaterY = CheatRead<unsigned short>(0xFFF646) >> POWEROFTWO;

		if (mode)
		{
			for(offs = Vmode ? 0 : (3*X*8)/4, j = (Vmode ? 240 : 224); j > 0; j-=4, Src -= 336 * 2 *4, offs += (3 * X))
			{
				if (Hmode==0) offs+=96/4;
				for(i = Hmode ? 320 : 256; i > (Hmode ? 320 : 256)>>1; i-=4) // right half only, 4 pixels at a time
				{
					int r=0, g=0, b=0, c=0;
					for(int xp = 0 ; xp < 4 ; xp++)
					{
						for(int yp = 0 ; yp < 4 ; yp++)
						{
							tmp = (unsigned int) (Src[2 * (i+xp) + (yp*336*2) + 16] + (Src[2 * (i+xp) + (yp*336*2) + 17] << 8));
							if(!tmp) continue;
							r += ((tmp >> 7) & 0xF8);
							g += ((tmp >> 2) & 0xF8);
							b += ((tmp << 3) & 0xF8);
							c++;
						}
					}
					if(!c) continue;
					Dest[offs + (3 * (i>>2)) + 2] = r/c;
					Dest[offs + (3 * (i>>2)) + 1] = g/c;
					Dest[offs + (3 * (i>>2))    ] = b/c;
				}
			}
		}
		else
		{
//			Src -= 336 * 2 *4;
			for(offs = (Vmode ? 0 : 3*((X*(8+((Y*ratio-(Vmode ? 240 : 224))-y)))/ratio)) + 3*(x/ratio), j = (Vmode ? 241 : 225) - ratio; j > 0; j-=ratio, Src -= 336 * 2 * ratio * (Bits32?2:1), offs += (3 * X))
			{
				if (Hmode==0) offs+=96/4;
				if(offs > X*Y*3) break;
				for(i = (Hmode ? 320 : 256)-ratio; i >= 0; i-=ratio)
				{
					if(offs + (3 * (i>>POWEROFTWO)) + 2 < 0)
						continue; // don't copy past end of the bitmap

					if(i < 120 && (j < 60 || j >= 224 - 24))
					{
						if(Dest[offs + (3 * (i>>POWEROFTWO)) + 2] || Dest[offs + (3 * (i>>POWEROFTWO)) + 1] || Dest[offs + (3 * (i>>POWEROFTWO))])
							continue; // don't copy score/time/rings/lives displays over anything that's already been copied
					}

					int colors [ratio*ratio];
					int freq [ratio*ratio];
					int rs [ratio*ratio];
					int gs [ratio*ratio];
					int bs [ratio*ratio];

					int r=0, g=0, b=0, c=0, numoff=0, fr=0,fg=0,fb=0,first=0,firstoff=0;
					for(int xp = 0 ; xp < ratio ; xp++)
					{
						for(int yp = 0 ; yp < ratio ; yp++)
						{
							tmp = *(unsigned short *) &(Src[2 * (i+xp) + (yp*336*2) + 16]);
							if (Bits32) tmp = *(unsigned int *) &(Src[2*(2 * (i+xp) + (yp*336*2) + 16)]);
							colors[yp+ratio*xp] = tmp;
							rs[yp+ratio*xp] = (Bits32?((tmp >> 16) & 0xFF):((tmp >> 8) & 0xF8));
							gs[yp+ratio*xp] = (Bits32?((tmp >> 8) & 0xFF):((tmp >> 3) & 0xFC));
							bs[yp+ratio*xp] = (Bits32?(tmp & 0xFF):((tmp << 3) & 0xF8));
							if(!tmp) {numoff++; if(!xp && !yp) firstoff=1; continue;}
							r += (Bits32?((tmp >> 16) & 0xFF):((tmp >> 8) & 0xF8));
							g += (Bits32?((tmp >> 8) & 0xFF):((tmp >> 3) & 0xFC));
							b += (Bits32?(tmp & 0xFF):((tmp << 3) & 0xF8));
//							if(!first) {first=1; fr=r; fg=g; fb=b;}
							c++;
						}
					}
					//if(!r && !g && !b)
					//{
					//	b = 48; r = 32; g = 32; // background, if Layer 1 is turned off
					//}
					if(!c || (numoff>10 && firstoff) /*|| (!fr && !fg && !fb)*/)
					{
						b = 48; r = 32; g = 32; // background, if Layer 1 is turned off
						if (((X * Y * 3) - offs) >= (WaterY * X * 3))
							b = 128;
					}
					else
					{
						for(int ii = 0 ; ii < ratio*ratio; ii++)
						{
							freq[ii] = 0;
						}
						for(int ii = 0 ; ii < ratio*ratio; ii++)
						{
							for(int jj = ii ; jj < ratio*ratio; jj++)
							{
								if(colors[ii] == colors[jj])
								{
									freq[ii]++;
									freq[jj]++;
								}
							}
						}
						fr = (Bits32?((colors[0] >> 16) & 0xFF):((colors[0]>> 3) & 0xFC));
						fg = (Bits32?((colors[0] >> 8) & 0xFF):((colors[0] >> 3) & 0xFC));
						fb = (Bits32?(colors[0] & 0xFF):((colors[0] << 3) & 0xF8));;
						int bestfreq = 0;
						for(int ii = 0 ; ii < ratio*ratio; ii++)
						{
							if(freq[ii] > bestfreq)
							{
								bestfreq = freq[ii];
								fr = rs[ii];
								fg = gs[ii];
								fb = bs[ii];
							}
							else if(bestfreq && freq[ii] == bestfreq)
							{
								if(fr || fg || fb)
								{
									rs[ii] = (fr + rs[ii]) >> 1;
									gs[ii] = (fg + gs[ii]) >> 1;
									bs[ii] = (fb + bs[ii]) >> 1;
								}
								if(rs[ii] || gs[ii] || bs[ii])
								{
									fr = rs[ii];
									fg = gs[ii];
									fb = bs[ii];
								}
							}
						}

	
						r /= c; g /= c; b /= c;

						// we have bilinear filtering so far, but that's a bit blurry, so mix it with point sampling
						if(fr || fg || fb)
						{
							int diff = abs(fr-r) + abs(fg-g) + abs(fb-b);
							while(diff >= 88)
							{
								r = (r*7+fr)/8;
								g = (g*7+fg)/8;
								b = (b*7+fb)/8;
								diff = abs(fr-r) + abs(fg-g) + abs(fb-b);
							}
						}
					}
					Dest[offs + (3 * (i>>POWEROFTWO)) + 2] = r;
					Dest[offs + (3 * (i>>POWEROFTWO)) + 1] = g;
					Dest[offs + (3 * (i>>POWEROFTWO))    ] = b;
				}
			}
		}

		if(GetAsyncKeyState('B'))
		{
			HANDLE ScrShot_File;
			ScrShot_File = CreateFile("mapcap.bmp", GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			WriteFile(ScrShot_File, DestFull, (X * Y * 3) + 54, &BW, NULL);
			CloseHandle(ScrShot_File);
		}
	}

}
#endif
LRESULT CALLBACK RamSearchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static int watchIndex=0;

	switch(uMsg)
	{
		case WM_INITDIALOG: {
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			// push it away from the main window if we can
			const int width = (r.right-r.left); 
			const int width2 = (r2.right-r2.left); 
			if(r.left+width2 + width < GetSystemMetrics(SM_CXSCREEN))
			{
				r.right += width;
				r.left += width;
			}
			else if((int)r.left - (int)width2 > 0)
			{
				r.right -= width2;
				r.left -= width2;
			}

			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			switch(rs_o)
			{
				case '<':
					SendDlgItemMessage(hDlg, IDC_LESSTHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case '>':
					SendDlgItemMessage(hDlg, IDC_MORETHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'l':
					SendDlgItemMessage(hDlg, IDC_NOMORETHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'm':
					SendDlgItemMessage(hDlg, IDC_NOLESSTHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case '=': 
					SendDlgItemMessage(hDlg, IDC_EQUALTO, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case '!':
					SendDlgItemMessage(hDlg, IDC_DIFFERENTFROM, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'd':
					SendDlgItemMessage(hDlg, IDC_DIFFERENTBY, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),true);
					break;
			}
			switch (rs_c)
			{
				case 'r':
					SendDlgItemMessage(hDlg, IDC_PREVIOUSVALUE, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 's':
					SendDlgItemMessage(hDlg, IDC_SPECIFICVALUE, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),true);
					break;
				case 'a':
					SendDlgItemMessage(hDlg, IDC_SPECIFICADDRESS, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),true);
					break;
				case 'n':
					SendDlgItemMessage(hDlg, IDC_NUMBEROFCHANGES, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),true);
					break;
			}
			switch (rs_t)
			{
				case 's':
					SendDlgItemMessage(hDlg, IDC_SIGNED, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'u':
					SendDlgItemMessage(hDlg, IDC_UNSIGNED, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'h':
					SendDlgItemMessage(hDlg, IDC_HEX, BM_SETCHECK, BST_CHECKED, 0);
					break;
			}
			switch (rs_type_size)
			{
				case 'b':
					SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'w':
					SendDlgItemMessage(hDlg, IDC_2_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'd':
					SendDlgItemMessage(hDlg, IDC_4_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
			}
			SendDlgItemMessage(hDlg,IDC_C_AUTOSEARCH,BM_SETCHECK,AutoSearch?BST_CHECKED:BST_UNCHECKED,0);
			char names[5][11] = {"Address","Value","Previous","Changes","Notes"};
			int widths[5] = {62,64,64,51,53};
			if (!ResultCount)
				reset_address_info();
			else
				signal_new_frame();
			init_list_box(GetDlgItem(hDlg,IDC_RAMLIST),names,5,widths);
			ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
			if (!noMisalign) SendDlgItemMessage(hDlg, IDC_MISALIGN, BM_SETCHECK, BST_CHECKED, 0);
			if (littleEndian) SendDlgItemMessage(hDlg, IDC_ENDIAN, BM_SETCHECK, BST_CHECKED, 0);
			return true;
		}	break;

		case WM_NOTIFY:
		{
			LPNMHDR lP = (LPNMHDR) lParam;
			switch (lP->code)
			{
				case LVN_GETDISPINFO:
				{
					LV_DISPINFO *Item = (LV_DISPINFO *)lParam;
					Item->item.mask = LVIF_TEXT;
					Item->item.state = 0;
					Item->item.iImage = 0;
					const unsigned int iNum = Item->item.iItem;
					static char num[11];
					switch (Item->item.iSubItem)
					{
						case 0:
							sprintf(num,"%08X",rsresults[iNum].Address);
							Item->item.pszText = num;
							return true;
						case 1: {
							int i;
							int size;
							switch (rs_type_size)
							{

								case 'w':
									size = 2;
									break;
								case 'd':
									size = 4;
									break;
								case 'b':
								default:
									size = 1;
									break;									
							}
							i = rsresults[Item->item.iItem].cur;
//							for (int j = 1; j < size; j++)
//								i |= rsaddrs[rsresults[Item->item.iItem].Index + j].cur << (8 * j);
//							Byte_Swap(&i,size);
							sprintf(num,
								((rs_t == 's')?
									"%d":
									(rs_t == 'u')?
										"%u":
										"%X"),
								(rs_t != 's')?
									i:
									((rs_type_size == 'b')?
										(char)(i&0xff):
										(rs_type_size == 'w')?
											(short)(i&0xffff):
											(long)(i&0xffffffff)));
							Item->item.pszText = num;
						}	return true;
						case 2: {
							int i;
							int size;
							switch (rs_type_size)
							{

								case 'w':
									size = 2;
									break;
								case 'd':
									size = 4;
									break;
								case 'b':
								default:
									size = 1;
									break;									
							}
							i = rsresults[Item->item.iItem].prev;
//							for (int j = 1; j < size; j++)
//								i |= rsaddrs[rsresults[Item->item.iItem].Index + j].prev << (8 * j);
//							Byte_Swap(&i,size);
							sprintf(num,
								((rs_t == 's')?
									"%d":
									(rs_t == 'u')?
										"%u":
										"%X"),
								(rs_t != 's')?
									i:
									((rs_type_size == 'b')?
										(char)(i&0xff):
										(rs_type_size == 'w')?
											(short)(i&0xffff):
											(long)(i&0xffffffff)));
							Item->item.pszText = num;
						}	return true;
						case 3:
							sprintf(num,"%d",rsaddrs[rsresults[iNum].Index].changes);
							Item->item.pszText = num;
							return true;
						case 4:
							Item->item.pszText = rsaddrs[rsresults[iNum].Index].comment ? rsaddrs[rsresults[iNum].Index].comment : "";
							return true;
						default:
							return false;
					}
				}
				//case LVN_ODCACHEHINT: //Copied this bit from the MSDN virtual listbox code sample. Eventually it should probably do something.
				//{
				//	LPNMLVCACHEHINT   lpCacheHint = (LPNMLVCACHEHINT)lParam;
				//	return 0;
				//}
				//case LVN_ODFINDITEM: //Copied this bit from the MSDN virtual listbox code sample. Eventually it should probably do something.
				//{	
				//	LPNMLVFINDITEM lpFindItem = (LPNMLVFINDITEM)lParam;
				//	return 0;
				//}
			}
		}

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_SIGNED:
					rs_t='s';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_UNSIGNED:
					rs_t='u';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_HEX:
					rs_t='h';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_1_BYTE:
					rs_type_size = 'b';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_2_BYTES:
					rs_type_size = 'w';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_4_BYTES:
					rs_type_size = 'd';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_MISALIGN:
					noMisalign = !noMisalign;
					CompactAddrs();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_ENDIAN:
//					littleEndian = !littleEndian;
//					signal_new_size();
//					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;				
				case IDC_LESSTHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = '<';
					return true;
				case IDC_MORETHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = '>';
					return true;
				case IDC_NOMORETHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = 'l';
					return true;
				case IDC_NOLESSTHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = 'm';
					return true;
				case IDC_EQUALTO:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = '=';
					return true;
				case IDC_DIFFERENTFROM:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = '!';
					return true;
				case IDC_DIFFERENTBY:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),true);
					rs_o = 'd';
					return true;
				case IDC_PREVIOUSVALUE:
					rs_c='r';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),false);
					return true;
				case IDC_SPECIFICVALUE:
					rs_c='s';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),false);
					return true;
				case IDC_SPECIFICADDRESS:
					rs_c='a';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),true);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),false);
					return true;
				case IDC_NUMBEROFCHANGES:
					rs_c='n';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),true);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),false);
					return true;
				case IDC_C_ADDCHEAT:
				{
//					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_RAMLIST));
//					Liste_GG[CheatCount].restore = Liste_GG[CheatCount].data = rsresults[watchIndex].cur;
//					Liste_GG[CheatCount].addr = rsresults[watchIndex].Address;
//					Liste_GG[CheatCount].size = rs_type_size;
//					Liste_GG[CheatCount].Type = rs_t;
//					Liste_GG[CheatCount].oper = '=';
//					Liste_GG[CheatCount].mode = 0;
//					DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDITCHEAT), hDlg, (DLGPROC) EditCheatProc,(LPARAM) 0);
				}
				case IDC_C_SAVE:
					// NYI
					return true;
				case IDC_C_LOAD:
					// NYI
					//strncpy(Str_Tmp,Rom_Name,512);
					////strcat(Str_Tmp,".gmv");
					//if(Change_File_S(Str_Tmp, Movhie_Dir, "Save Movie", "GENs Movie\0*.gmv*\0All Files\0*.*\0\0", "gmv"))
					//{
					//		SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_NAME,WM_SETTEXT,0,(LPARAM)Str_Tmp);
					//}
					return true;
				case IDC_C_RESET:
					reset_address_info();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_C_AUTOSEARCH:
					AutoSearch = !AutoSearch;
					if (!AutoSearch) return true;
				case IDC_C_SEARCH:
				{
					rs_param = ((rs_o == 'd') ? GetDlgItemInt(hDlg,IDC_EDIT_DIFFBY,NULL,false) : 0);

					switch(rs_c)
					{
						case 'r':
						default:
							rs_val = 0;
							break;
						case 's':
							if(rs_t == 'h')
							{
								if(!GetDlgItemText(hDlg,IDC_EDIT_COMPAREVALUE,Str_Tmp,9))
									goto invalid_field;
								rs_val = HexStrToInt(Str_Tmp);
							}
							else
							{
								BOOL success;
								rs_val = GetDlgItemInt(hDlg,IDC_EDIT_COMPAREVALUE,&success,(rs_t == 's'));
								if(!success)
									goto invalid_field;
							}
							if((rs_type_size == 'b' && rs_t == 's' && (rs_val < -128 || rs_val > 127)) ||
							   (rs_type_size == 'b' && rs_t != 's' && (rs_val < 0 || rs_val > 255)) ||
							   (rs_type_size == 'w' && rs_t == 's' && (rs_val < -32768 || rs_val > 32767)) ||
							   (rs_type_size == 'w' && rs_t != 's' && (rs_val < 0 || rs_val > 65535)))
							   goto invalid_field;
							break;
						case 'a':
							if(!GetDlgItemText(hDlg,IDC_EDIT_COMPAREADDRESS,Str_Tmp,9))
								goto invalid_field;
							rs_val = HexStrToInt(Str_Tmp);
							if(rs_val < 0 || rs_val > 0x0603FFFF)
								goto invalid_field;
							break;
						case 'n': {
							BOOL success;
							rs_val = GetDlgItemInt(hDlg,IDC_EDIT_COMPARECHANGES,&success,false);
							if(!success || rs_val < 0)
								goto invalid_field;
						}	break;
					}

					int SetPaused = (!(Paused == 1));
					if (SetPaused) Paused = 1;
					Clear_Sound_Buffer();
					prune(rs_c,rs_o,rs_t,rs_val,rs_param);

					if(!ResultCount)
					{
						reset_address_info();
						prune(rs_c,rs_o,rs_t,rs_val,rs_param);
						if(ResultCount && rs_c != 'a')
							MessageBox(NULL,"Performing search on all addresses.","Out of results.",MB_OK|MB_ICONINFORMATION);
					}

					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					if (SetPaused) Paused = 0;
					return true;

invalid_field:
					MessageBox(NULL,"Invalid or out-of-bound entered value.","Error",MB_OK|MB_ICONSTOP);
					return true;
				}
				case IDC_C_WATCH:
				{
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_RAMLIST));
					rswatches[WatchCount].Index = rsresults[watchIndex].Index;
					rswatches[WatchCount].Address = rsresults[watchIndex].Address;
					rswatches[WatchCount].Size = rs_type_size;
					rswatches[WatchCount].Type = rs_t;
					rswatches[WatchCount].WrongEndian = 0; //Replace when I get little endian working
					DialogBox(ghInstance, MAKEINTRESOURCE(IDD_PROMPT), hDlg, (DLGPROC) PromptWatchNameProc);
					return true;
				}
				case IDC_C_WATCH_EDIT:
				{
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_RAMLIST));
					rsaddrs[rsresults[watchIndex].Index].flags |= RS_FLAG_ELIMINATED;
					ResultCount--;
					signal_new_size();
//					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),0);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				}
				case IDOK:
				case IDCANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					DialogsOpen--;
					RamSearchHWnd = NULL;
					EndDialog(hDlg, true);
					return true;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			RamSearchHWnd = NULL;
			EndDialog(hDlg, true);
			return true;
	}

	return false;
}
LRESULT CALLBACK VolumeProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static unsigned short TempMast, Temp2612, TempPSG, TempDAC, TempPCM, TempCDDA, TempPWM;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			for (int i = IDC_MASTVOL; i <= IDC_CDDAVOL; i++)
			{
				SendDlgItemMessage(hDlg,i,TBM_SETTICFREQ,(WPARAM)32,0);
				SendDlgItemMessage(hDlg,i,TBM_SETRANGE,(WPARAM)TRUE,(LPARAM) MAKELONG(0,256));
				SendDlgItemMessage(hDlg,i,TBM_SETLINESIZE,(WPARAM)0,(LPARAM) 1);
				SendDlgItemMessage(hDlg,i,TBM_SETPAGESIZE,(WPARAM)0,(LPARAM) 16);
			}
			SendDlgItemMessage(hDlg,IDC_MASTVOL,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(256 - MastVol));
			SendDlgItemMessage(hDlg,IDC_2612VOL,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(256 - YM2612Vol));
			SendDlgItemMessage(hDlg,IDC_PSGVOL,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(256 - PSGVol));
			SendDlgItemMessage(hDlg,IDC_DACVOL,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(256 - DACVol));
			SendDlgItemMessage(hDlg,IDC_PCMVOL,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(256 - PCMVol));
			SendDlgItemMessage(hDlg,IDC_CDDAVOL,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(256 - CDDAVol));
			SendDlgItemMessage(hDlg,IDC_PWMVOL,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(256 - PWMVol));
			TempMast = MastVol;
			Temp2612 = YM2612Vol;
			TempPSG = PSGVol;
			TempDAC = DACVol;
			TempPCM = PCMVol;
			TempCDDA = CDDAVol;
			TempPWM = PWMVol;
			return true;

		case WM_COMMAND:
			switch(wParam)
			{
				case IDOK:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					DialogsOpen--;
					VolControlHWnd = NULL;
					EndDialog(hDlg, true);
					return true;
				case ID_CANCEL:
					MastVol = TempMast;
					YM2612Vol = Temp2612;
					PSGVol = TempPSG;
					DACVol = TempDAC;
					PCMVol = TempPCM;
					CDDAVol = TempCDDA;
					PWMVol = TempPWM;
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					DialogsOpen--;
					VolControlHWnd = NULL;
					EndDialog(hDlg, true);
					return true;
				default:
					return true;
			}

		case WM_VSCROLL:
		{
			if (LOWORD(wParam) == SB_ENDSCROLL)
				return true;
			int i = IDC_MASTVOL;
			while (GetDlgItem(hDlg,i) != (HWND) lParam)
				i++;
			switch (i)
			{
				case IDC_MASTVOL:
					MastVol = 256 - (short)SendDlgItemMessage(hDlg,i,TBM_GETPOS,(WPARAM)0,(LPARAM)0);
					break;
				case IDC_2612VOL:
					YM2612Vol = 256 - (short)SendDlgItemMessage(hDlg,i,TBM_GETPOS,(WPARAM)0,(LPARAM)0);
					break;
				case IDC_PSGVOL:
					PSGVol = 256 - (short)SendDlgItemMessage(hDlg,i,TBM_GETPOS,(WPARAM)0,(LPARAM)0);
					break;
				case IDC_DACVOL:
					DACVol = 256 - (short)SendDlgItemMessage(hDlg,i,TBM_GETPOS,(WPARAM)0,(LPARAM)0);
					break;
				case IDC_PCMVOL:
					PCMVol = 256 - (short)SendDlgItemMessage(hDlg,i,TBM_GETPOS,(WPARAM)0,(LPARAM)0);
					break;
				case IDC_CDDAVOL:
					CDDAVol = 256 - (short)SendDlgItemMessage(hDlg,i,TBM_GETPOS,(WPARAM)0,(LPARAM)0);
					break;
				case IDC_PWMVOL:
					PWMVol = 256 - (short)SendDlgItemMessage(hDlg,i,TBM_GETPOS,(WPARAM)0,(LPARAM)0);
					break;
				default:
					break;
			}
			return true;
		}

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			VolControlHWnd = NULL;
			EndDialog(hDlg, true);
			return true;
	}

	return false;
}
LRESULT CALLBACK AboutProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			return true;
			break;

		case WM_COMMAND:
			switch(wParam)
			{
				case IDOK:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}

					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}

			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}

LRESULT CALLBACK ColorProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			Build_Language_String();

			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

			WORD_L(IDC_STATIC_CONT, "Contrast", "", "Contrast");
			WORD_L(IDC_STATIC_BRIGHT, "Brightness", "", "Brightness");
			WORD_L(IDC_CHECK_GREYSCALE, "Greyscale", "", "Greyscale");
			WORD_L(IDC_CHECK_INVERT, "Invert", "", "Invert");

			WORD_L(ID_APPLY, "Apply", "", "&Apply");
			WORD_L(ID_CLOSE, "Close", "", "&Close");
			WORD_L(ID_DEFAULT, "Default", "", "&Default");

			SendDlgItemMessage(hDlg, IDC_SLIDER_CONTRASTE, TBM_SETRANGE, (WPARAM) (BOOL) TRUE, (LPARAM) MAKELONG(0, 200));
			SendDlgItemMessage(hDlg, IDC_SLIDER_CONTRASTE, TBM_SETPOS, (WPARAM) (BOOL) TRUE, (LPARAM) (LONG) (Contrast_Level));
			SendDlgItemMessage(hDlg, IDC_SLIDER_LUMINOSITE, TBM_SETRANGE, (WPARAM) (BOOL) TRUE, (LPARAM) MAKELONG(0, 200));
			SendDlgItemMessage(hDlg, IDC_SLIDER_LUMINOSITE, TBM_SETPOS, (WPARAM) (BOOL) TRUE, (LPARAM) (LONG) (Brightness_Level));

			SendDlgItemMessage(hDlg, IDC_CHECK_GREYSCALE, BM_SETCHECK, (WPARAM) (Greyscale)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK_INVERT, BM_SETCHECK, (WPARAM) (Invert_Color)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_CHECK_XRAY, BM_SETCHECK, (WPARAM) (XRay)?BST_CHECKED:BST_UNCHECKED, 0);

			return true;
			break;

		case WM_COMMAND:
			switch(wParam)
			{
				case ID_CLOSE:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}

					Build_Main_Menu();
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;

				case ID_APPLY:
					Contrast_Level = SendDlgItemMessage(hDlg, IDC_SLIDER_CONTRASTE, TBM_GETPOS, 0, 0);
					Brightness_Level = SendDlgItemMessage(hDlg, IDC_SLIDER_LUMINOSITE, TBM_GETPOS, 0, 0);
					Greyscale = (SendDlgItemMessage(hDlg, IDC_CHECK_GREYSCALE, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;
					Invert_Color = (SendDlgItemMessage(hDlg, IDC_CHECK_INVERT, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;
					XRay = (SendDlgItemMessage(hDlg, IDC_CHECK_XRAY, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;

					Recalculate_Palettes();
					if (Genesis_Started || _32X_Started || SegaCD_Started)
					{
						CRam_Flag = 1;
						Show_Genesis_Screen(HWnd);
					}
					return true;

				case ID_DEFAULT:
					SendDlgItemMessage(hDlg, IDC_SLIDER_CONTRASTE, TBM_SETPOS, (WPARAM) (BOOL) TRUE, (LPARAM) (LONG) (100));
					SendDlgItemMessage(hDlg, IDC_SLIDER_LUMINOSITE, TBM_SETPOS, (WPARAM) (BOOL) TRUE, (LPARAM) (LONG) (100));
					SendDlgItemMessage(hDlg, IDC_CHECK_GREYSCALE, BM_SETCHECK, BST_UNCHECKED, 0);
					SendDlgItemMessage(hDlg, IDC_CHECK_INVERT, BM_SETCHECK, BST_UNCHECKED, 0);
					return true;

				case ID_HELP_HELP:
/*					if (Detect_Format(Manual_Path) != -1)		// can be used to detect if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpmisc.html");
						system(Str_Tmp);
					}
					else
					{
						MessageBox(NULL, "You need to configure the Manual Path to have help", "info", MB_OK);
					}
*/					return true;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}

			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}


LRESULT CALLBACK OptionProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			Build_Language_String();

			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

			WORD_L(IDC_AUTOFIXCHECKSUM, "Auto Fix Checksum", "", "Auto Fix Checksum");
			WORD_L(IDC_AUTOPAUSE, "Auto Pause", "", "Auto Pause");
			WORD_L(IDC_FASTBLUR, "Fast Blur", "", "Fast Blur");
			WORD_L(IDC_SHOWLED, "Show Sega-CD LED", "", "Show Sega-CD LED");
			WORD_L(IDC_ENABLE_FPS, "Enable", "", "Enable");
			WORD_L(IDC_ENABLE_MESSAGE, "Enable", "", "Enable");
			WORD_L(IDC_X2_FPS, "Double Sized", "", "Double Sized");
			WORD_L(IDC_X2_MESSAGE, "Double Sized", "", "Double Sized");
			WORD_L(IDC_TRANS_FPS, "Transparency", "", "Transparency");
			WORD_L(IDC_TRANS_MESSAGE, "Transparency", "", "Transparency");
			WORD_L(IDC_EFFECT_COLOR, "Effect Color", "", "Intro effect color");
			WORD_L(IDC_OPTION_SYSTEM, "System", "", "System");
			WORD_L(IDC_OPTION_FPS, "FPS", "", "FPS");
			WORD_L(IDC_OPTION_MESSAGE, "Message", "", "Message");
			WORD_L(IDC_OPTION_CANCEL, "Cancel", "", "&Cancel");
			WORD_L(IDC_OPTION_OK, "OK", "", "&OK");

			SendDlgItemMessage(hDlg, IDC_COLOR_FPS, TBM_SETRANGE, (WPARAM) (BOOL) TRUE, (LPARAM) MAKELONG(0, 3));
			SendDlgItemMessage(hDlg, IDC_COLOR_FPS, TBM_SETPOS, (WPARAM) (BOOL) TRUE, (LPARAM) (LONG) ((FPS_Style & 0x6) >> 1));
			SendDlgItemMessage(hDlg, IDC_COLOR_MESSAGE, TBM_SETRANGE, (WPARAM) (BOOL) TRUE, (LPARAM) MAKELONG(0, 3));
			SendDlgItemMessage(hDlg, IDC_COLOR_MESSAGE, TBM_SETPOS, (WPARAM) (BOOL) TRUE, (LPARAM) (LONG) ((Message_Style & 0x6) >> 1));
			SendDlgItemMessage(hDlg, IDC_COLOR_EFFECT, TBM_SETRANGE, (WPARAM) (BOOL) TRUE, (LPARAM) MAKELONG(0, 7));
			SendDlgItemMessage(hDlg, IDC_COLOR_EFFECT, TBM_SETPOS, (WPARAM) (BOOL) TRUE, (LPARAM) (LONG) Effect_Color);

			SendDlgItemMessage(hDlg, IDC_AUTOFIXCHECKSUM, BM_SETCHECK, (WPARAM) (Auto_Fix_CS)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_AUTOPAUSE, BM_SETCHECK, (WPARAM) (Auto_Pause)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_FASTBLUR, BM_SETCHECK, (WPARAM) (Fast_Blur)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_SHOWLED, BM_SETCHECK, (WPARAM) (Show_LED)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_ENABLE_FPS, BM_SETCHECK, (WPARAM) (Show_FPS)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_X2_FPS, BM_SETCHECK, (WPARAM) (FPS_Style & 0x10)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_TRANS_FPS, BM_SETCHECK, (WPARAM) (FPS_Style & 0x8)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_ENABLE_MESSAGE, BM_SETCHECK, (WPARAM) (Show_Message)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_X2_MESSAGE, BM_SETCHECK, (WPARAM) (Message_Style & 0x10)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_TRANS_MESSAGE, BM_SETCHECK, (WPARAM) (Message_Style & 0x8)?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_DISABLE_BLUE_SCREEN, BM_SETCHECK, (WPARAM) (Disable_Blue_Screen)?BST_CHECKED:BST_UNCHECKED, 0); //Modif
			SendDlgItemMessage(hDlg, IDC_DISABLE_INTRO_EFFECT, BM_SETCHECK, (WPARAM) (Intro_Style==0)?BST_CHECKED:BST_UNCHECKED, 0); //Modif N
			SendDlgItemMessage(hDlg, IDC_FRAMECOUNTER, BM_SETCHECK, (WPARAM) (FrameCounterEnabled)?BST_CHECKED:BST_UNCHECKED, 0); //Modif
			SendDlgItemMessage(hDlg, IDC_FRAMECOUNTERFRAMES, BM_SETCHECK, (WPARAM) (FrameCounterFrames)?BST_CHECKED:BST_UNCHECKED, 0); //Modif N.
			SendDlgItemMessage(hDlg, IDC_LAGCOUNTER, BM_SETCHECK, (WPARAM) (LagCounterEnabled)?BST_CHECKED:BST_UNCHECKED, 0); //Modif
			SendDlgItemMessage(hDlg, IDC_LAGCOUNTERFRAMES, BM_SETCHECK, (WPARAM) (LagCounterFrames)?BST_CHECKED:BST_UNCHECKED, 0); //Modif N.
			SendDlgItemMessage(hDlg, IDC_CHECK_SHOWINPUT, BM_SETCHECK, (WPARAM) (ShowInputEnabled)?BST_CHECKED:BST_UNCHECKED, 0); //Modif
			SendDlgItemMessage(hDlg, IDC_CHECK_BACKUP, BM_SETCHECK, (WPARAM) (AutoBackupEnabled)?BST_CHECKED:BST_UNCHECKED, 0); //Modif
			SendDlgItemMessage(hDlg, IDC_CHECK_DEF_READ_ONLY, BM_SETCHECK, (WPARAM) (Def_Read_Only)?BST_CHECKED:BST_UNCHECKED, 0); //Upth-Add
			SendDlgItemMessage(hDlg, IDC_CHECK_AUTO_CLOSE, BM_SETCHECK, (WPARAM) (AutoCloseMovie)?BST_CHECKED:BST_UNCHECKED, 0); //Upth-Add
			SendDlgItemMessage(hDlg, IDC_CHECK_USEMOVIESTATES, BM_SETCHECK, (WPARAM) (UseMovieStates)?BST_CHECKED:BST_UNCHECKED, 0); //Upth-Add

			SendDlgItemMessage(hDlg, IDC_TOP_LEFT, BM_SETCHECK, (WPARAM) (FrameCounterPosition==FRAME_COUNTER_TOP_LEFT)?BST_CHECKED:BST_UNCHECKED, 0); //Modif
			SendDlgItemMessage(hDlg, IDC_TOP_RIGHT, BM_SETCHECK, (WPARAM) (FrameCounterPosition==FRAME_COUNTER_TOP_RIGHT)?BST_CHECKED:BST_UNCHECKED, 0); //Modif
			SendDlgItemMessage(hDlg, IDC_BOTTOM_LEFT, BM_SETCHECK, (WPARAM) (FrameCounterPosition==FRAME_COUNTER_BOTTOM_LEFT)?BST_CHECKED:BST_UNCHECKED, 0); //Modif
			SendDlgItemMessage(hDlg, IDC_BOTTOM_RIGHT, BM_SETCHECK, (WPARAM) (FrameCounterPosition==FRAME_COUNTER_BOTTOM_RIGHT)?BST_CHECKED:BST_UNCHECKED, 0); //Modif
			SetDlgItemInt(hDlg,IDC_TEXT_RES_X,Res_X,true); //Upth-Add - These will show the currently configured
			SetDlgItemInt(hDlg,IDC_TEXT_RES_Y,Res_Y,true); //Upth-Add - fullscreen resolution value
			SetDlgItemInt(hDlg,IDC_TEXT_DELAY,DelayFactor,false); //Upth-Add - Frame Advance delay mod
			
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_OPTION_OK:
				{	unsigned int res;

					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}

					res = SendDlgItemMessage(hDlg, IDC_COLOR_FPS, TBM_GETPOS, 0, 0);
					FPS_Style = (FPS_Style & ~0x6) | ((res << 1) & 0x6);
					res = SendDlgItemMessage(hDlg, IDC_COLOR_MESSAGE, TBM_GETPOS, 0, 0);
					Message_Style = (Message_Style & 0xF9) | ((res << 1) & 0x6);
					Effect_Color = SendDlgItemMessage(hDlg, IDC_COLOR_EFFECT, TBM_GETPOS, 0, 0);
					Intro_Style = (SendDlgItemMessage(hDlg, IDC_DISABLE_INTRO_EFFECT, BM_GETCHECK, 0, 0) == BST_CHECKED)?0:(Intro_Style?Intro_Style:2); //Modif N.

					if(Effect_Color == 0) //Modif N. - black color disables intro
						Intro_Style = 0;

					Auto_Fix_CS = (SendDlgItemMessage(hDlg, IDC_AUTOFIXCHECKSUM, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;
					Auto_Pause = (SendDlgItemMessage(hDlg, IDC_AUTOPAUSE, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;
					Fast_Blur = (SendDlgItemMessage(hDlg, IDC_FASTBLUR, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;
					Show_LED = (SendDlgItemMessage(hDlg, IDC_SHOWLED, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;
					Show_FPS = (SendDlgItemMessage(hDlg, IDC_ENABLE_FPS, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;
					res = SendDlgItemMessage(hDlg, IDC_X2_FPS, BM_GETCHECK, 0, 0);
					FPS_Style = (FPS_Style & ~0x10) | ((res == BST_CHECKED)?0x10:0);
					res = SendDlgItemMessage(hDlg, IDC_TRANS_FPS, BM_GETCHECK, 0, 0);
					FPS_Style = (FPS_Style & ~0x8) | ((res == BST_CHECKED)?0x8:0);
					Show_Message = (SendDlgItemMessage(hDlg, IDC_ENABLE_MESSAGE, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;
					res = SendDlgItemMessage(hDlg, IDC_X2_MESSAGE, BM_GETCHECK, 0, 0);
					Message_Style = (Message_Style & ~0x10) | ((res == BST_CHECKED)?0x10:0);
					res = SendDlgItemMessage(hDlg, IDC_TRANS_MESSAGE, BM_GETCHECK, 0, 0);
					Message_Style = (Message_Style & ~0x8) | ((res == BST_CHECKED)?0x8:0);
					Disable_Blue_Screen = (SendDlgItemMessage(hDlg, IDC_DISABLE_BLUE_SCREEN, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;//Modif
					FrameCounterEnabled = (SendDlgItemMessage(hDlg, IDC_FRAMECOUNTER, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;//Modif
					FrameCounterFrames = (SendDlgItemMessage(hDlg, IDC_FRAMECOUNTERFRAMES, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;//Modif N.
					LagCounterEnabled = (SendDlgItemMessage(hDlg, IDC_LAGCOUNTER, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;//Modif
					LagCounterFrames = (SendDlgItemMessage(hDlg, IDC_LAGCOUNTERFRAMES, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;//Modif N.
					ShowInputEnabled = (SendDlgItemMessage(hDlg, IDC_CHECK_SHOWINPUT, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;//Modif
					AutoBackupEnabled = (SendDlgItemMessage(hDlg, IDC_CHECK_BACKUP, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;//Modif
					if(SendDlgItemMessage(hDlg, IDC_TOP_LEFT, BM_GETCHECK, 0, 0) == BST_CHECKED)
						FrameCounterPosition = FRAME_COUNTER_TOP_LEFT;
					else
						if(SendDlgItemMessage(hDlg, IDC_TOP_RIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED)
							FrameCounterPosition = FRAME_COUNTER_TOP_RIGHT;
						else
							if(SendDlgItemMessage(hDlg, IDC_BOTTOM_LEFT, BM_GETCHECK, 0, 0) == BST_CHECKED)
								FrameCounterPosition = FRAME_COUNTER_BOTTOM_LEFT;
							else
								FrameCounterPosition = FRAME_COUNTER_BOTTOM_RIGHT;
					Res_X = GetDlgItemInt(hDlg,IDC_TEXT_RES_X,NULL,true); //Upth-Add - This reconfigures
					Res_Y = GetDlgItemInt(hDlg,IDC_TEXT_RES_Y,NULL,true); //Upth-Add - the fullscreen resolution

					Def_Read_Only = (SendDlgItemMessage(hDlg, IDC_CHECK_DEF_READ_ONLY, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0; //Upth-Add //Modif N. - it shouldn't save if the user hits Cancel instead of OK
					AutoCloseMovie = (SendDlgItemMessage(hDlg, IDC_CHECK_AUTO_CLOSE, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0; //Upth-Add //Modif N. - it shouldn't save if the user hits Cancel instead of OK
					UseMovieStates = (SendDlgItemMessage(hDlg, IDC_CHECK_USEMOVIESTATES, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0; //Upth-Add
					unsigned int TempDelay = GetDlgItemInt(hDlg,IDC_TEXT_DELAY,NULL,false);
					DelayFactor = (TempDelay) ? TempDelay : 1 ;

					Build_Main_Menu();
					
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
				}	break;

				case IDC_OPTION_CANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
				case ID_HELP_HELP:
					if (Detect_Format(Manual_Path) != -1)		// can be used to detect if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpmisc.html");
						system(Str_Tmp);
					}
					else
					{
						MessageBox(NULL, "You need to configure the Manual Path to have help", "info", MB_OK);
					}
					return true;
					break;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}

			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}


LRESULT CALLBACK ControllerProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
    int i; 
	static HWND Tex0 = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

			Tex0 = GetDlgItem(hDlg, IDC_STATIC_TEXT0);

			if (!Init_Input(ghInstance, hDlg)) return false;

			WORD_L(IDC_JOYINFO1, "Controller info 1", "", "Player 1-B 1-C and 1-D are enabled only if a teamplayer is connected to port 1");
			WORD_L(IDC_JOYINFO2, "Controller info 2", "", "Player 2-B 2-C and 2-D are enabled only if a teamplayer is connected to port 2");
			WORD_L(IDC_JOYINFO3, "Controller info 3", "", "Only a few games support teamplayer (games which have 4 players support), so don't forget to use the \"load config\" and \"save config\" possibility :)");

			for(i = 0; i < 2; i++)
			{
				SendDlgItemMessage(hDlg, IDC_COMBO_PORT1 + i, CB_INSERTSTRING, (WPARAM) 0, (LONG) (LPTSTR) "teamplayer");
				SendDlgItemMessage(hDlg, IDC_COMBO_PORT1 + i, CB_INSERTSTRING, (WPARAM) 0, (LONG) (LPTSTR) "pad");
			}

			for(i = 0; i < 8; i++)
			{
				SendDlgItemMessage(hDlg, IDC_COMBO_PADP1 + i, CB_INSERTSTRING, (WPARAM) 0, (LONG) (LPTSTR) "6 buttons");
				SendDlgItemMessage(hDlg, IDC_COMBO_PADP1 + i, CB_INSERTSTRING, (WPARAM) 0, (LONG) (LPTSTR) "3 buttons");
			}
			SendDlgItemMessage(hDlg, IDC_COMBO_NUMLOAD, CB_INSERTSTRING, (WPARAM) 0, (LONG) (LPTSTR) "# Load, Shift-# save, Ctrl-# select");
			SendDlgItemMessage(hDlg, IDC_COMBO_NUMLOAD, CB_INSERTSTRING, (WPARAM) 0, (LONG) (LPTSTR) "# Load, Ctrl-# save, Shift-# select");
			SendDlgItemMessage(hDlg, IDC_COMBO_NUMLOAD, CB_INSERTSTRING, (WPARAM) 0, (LONG) (LPTSTR) "Shift-# Load, # save, Ctrl-# select");
			SendDlgItemMessage(hDlg, IDC_COMBO_NUMLOAD, CB_INSERTSTRING, (WPARAM) 0, (LONG) (LPTSTR) "Shift-# Load, Ctrl-# save, # select");
			SendDlgItemMessage(hDlg, IDC_COMBO_NUMLOAD, CB_INSERTSTRING, (WPARAM) 0, (LONG) (LPTSTR) "Ctrl-# Load, # save, Shift-# select");
			SendDlgItemMessage(hDlg, IDC_COMBO_NUMLOAD, CB_INSERTSTRING, (WPARAM) 0, (LONG) (LPTSTR) "Ctrl-# Load, Shift-# save, # select");

			SendDlgItemMessage(hDlg, IDC_COMBO_PORT1, CB_SETCURSEL, (WPARAM) ((Controller_1_Type >> 4) & 1), (LPARAM) 0);
			SendDlgItemMessage(hDlg, IDC_COMBO_PORT2, CB_SETCURSEL, (WPARAM) ((Controller_2_Type >> 4) & 1), (LPARAM) 0);

			SendDlgItemMessage(hDlg, IDC_COMBO_PADP1, CB_SETCURSEL, (WPARAM) (Controller_1_Type & 1), (LPARAM) 0);
			SendDlgItemMessage(hDlg, IDC_COMBO_PADP1B, CB_SETCURSEL, (WPARAM) (Controller_1B_Type & 1), (LPARAM) 0);
			SendDlgItemMessage(hDlg, IDC_COMBO_PADP1C, CB_SETCURSEL, (WPARAM) (Controller_1C_Type & 1), (LPARAM) 0);
			SendDlgItemMessage(hDlg, IDC_COMBO_PADP1D, CB_SETCURSEL, (WPARAM) (Controller_1D_Type & 1), (LPARAM) 0);
			SendDlgItemMessage(hDlg, IDC_COMBO_PADP2, CB_SETCURSEL, (WPARAM) (Controller_2_Type & 1), (LPARAM) 0);
			SendDlgItemMessage(hDlg, IDC_COMBO_PADP2B, CB_SETCURSEL, (WPARAM) (Controller_2B_Type & 1), (LPARAM) 0);
			SendDlgItemMessage(hDlg, IDC_COMBO_PADP2C, CB_SETCURSEL, (WPARAM) (Controller_2C_Type & 1), (LPARAM) 0);
			SendDlgItemMessage(hDlg, IDC_COMBO_PADP2D, CB_SETCURSEL, (WPARAM) (Controller_2D_Type & 1), (LPARAM) 0);
            SendDlgItemMessage(hDlg, IDC_CHECK_LEFTRIGHT, BM_SETCHECK, (WPARAM) (LeftRightEnabled)?BST_CHECKED:BST_UNCHECKED, 0); //Modif
			if (NumLoadEnabled) EnableWindow(GetDlgItem(hDlg,IDC_COMBO_NUMLOAD),TRUE);
			else EnableWindow(GetDlgItem(hDlg,IDC_COMBO_NUMLOAD),FALSE);
            SendDlgItemMessage(hDlg, IDC_CHECK_NUMLOAD, BM_SETCHECK, (WPARAM) (NumLoadEnabled)?BST_CHECKED:BST_UNCHECKED, 0); //Modif N.
			if (StateSelectCfg > 5)
				StateSelectCfg = 0;
			SendDlgItemMessage(hDlg, IDC_COMBO_NUMLOAD, CB_SETCURSEL, (WPARAM) (StateSelectCfg), (LPARAM) 0);			return true;
			break;

		case WM_COMMAND:
			switch(wParam)
			{
				case IDOK:
					Controller_1_Type = (SendDlgItemMessage(hDlg, IDC_COMBO_PORT1, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1);
					Controller_1_Type <<= 4;

					Controller_1_Type |= (SendDlgItemMessage(hDlg, IDC_COMBO_PADP1, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1);
					Controller_1B_Type = (SendDlgItemMessage(hDlg, IDC_COMBO_PADP1B, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1);
					Controller_1C_Type = (SendDlgItemMessage(hDlg, IDC_COMBO_PADP1C, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1);
					Controller_1D_Type = (SendDlgItemMessage(hDlg, IDC_COMBO_PADP1D, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1);

					
					Controller_2_Type = (SendDlgItemMessage(hDlg, IDC_COMBO_PORT2, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1);
					Controller_2_Type <<= 4;

					Controller_2_Type |= (SendDlgItemMessage(hDlg, IDC_COMBO_PADP2, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1);
					Controller_2B_Type = (SendDlgItemMessage(hDlg, IDC_COMBO_PADP2B, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1);
					Controller_2C_Type = (SendDlgItemMessage(hDlg, IDC_COMBO_PADP2C, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1);
					Controller_2D_Type = (SendDlgItemMessage(hDlg, IDC_COMBO_PADP2D, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1);
                    LeftRightEnabled = (SendDlgItemMessage(hDlg, IDC_CHECK_LEFTRIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;//Modif
                    NumLoadEnabled = (SendDlgItemMessage(hDlg, IDC_CHECK_NUMLOAD, BM_GETCHECK, 0, 0) == BST_CHECKED)?1:0;//Modif N.
					StateSelectCfg = (unsigned char)SendDlgItemMessage(hDlg, IDC_COMBO_NUMLOAD, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
					Make_IO_Table();
					End_Input();
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
				case IDC_CHECK_NUMLOAD:
					if (SendDlgItemMessage(hDlg, IDC_CHECK_NUMLOAD, BM_GETCHECK, 0, 0) == BST_CHECKED) EnableWindow(GetDlgItem(hDlg,IDC_COMBO_NUMLOAD),TRUE);
					else EnableWindow(GetDlgItem(hDlg,IDC_COMBO_NUMLOAD),FALSE);
					return true;
					break;
				case IDC_BUTTON_REDEFINE_SKIP_KEY:	//Modif
					SetWindowText(Tex0, "SETTING KEY: ADVANCE NEXT FRAME"); //Upth-Modif - We're setting one key. Not multiple keys. This has been bugging me for a while
					Setting_Skip_Key(hDlg);
					SetWindowText(Tex0, ""); //Upth-Modif - We aren't setting keys anymore, so why say "SETTING KEYS"?
					return true;
					break;
				case IDC_BUTTON_REMOVE_SKIP_KEY:	//Modif
					SkipKey=0;
					return true;
					break;
				case IDC_BUTTON_REDEFINE_SLOW_KEY:	//Modif
					SetWindowText(Tex0, "SETTING KEY: TOGGLE SLOW MODE");
					Setting_Slow_Key(hDlg);
					SetWindowText(Tex0, "");
					return true;
					break;
				case IDC_BUTTON_REMOVE_SLOW_KEY:	//Modif
					SlowDownKey=0;
					return true;
					break;
				case IDC_BUTTON_REDEFINE_PAUSE_KEY:	//Modif
					SetWindowText(Tex0, "SETTING KEY: PAUSE");
					Setting_Quickpause_Key(hDlg);
					SetWindowText(Tex0, "");
					return true;
					break;
				case IDC_BUTTON_REMOVE_PAUSE_KEY:	//Modif
					QuickPauseKey=0;
					return true;
					break;
				case IDC_BUTTON_REMOVE_QUICK_KEY: //Modif
					QuickLoadKey=0;
					QuickSaveKey=0;
					return true;
					break;
				case IDC_BUTTON_QUICKLOAD_KEY:	//Modif
					SetWindowText(Tex0, "SETTING KEY: QUICKLOAD");
					Setting_Quickload_Key(hDlg);
					SetWindowText(Tex0, "");
					return true;
					break;
				case IDC_BUTTON_QUICKSAVE_KEY:	//Modif
					SetWindowText(Tex0, "SETTING KEY: QUICKSAVE");
					Setting_Quicksave_Key(hDlg);
					SetWindowText(Tex0, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_AUTOFIRE_KEY:	//Modif N.
					SetWindowText(Tex0, "SETTING KEYS AUTO-FIRE");
					Setting_Autofire_Key(hDlg);
					SetWindowText(Tex0, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_AUTOHOLD_KEY:	//Modif N.
					SetWindowText(Tex0, "SETTING KEYS AUTO-HOLD");
					Setting_Autohold_Key(hDlg);
					SetWindowText(Tex0, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_AUTOCLEAR_KEY:	//Modif N.
					SetWindowText(Tex0, "SETTING KEYS CLEARING AUTOS");
					Setting_Autoclear_Key(hDlg);
					SetWindowText(Tex0, "");
					return true;
					break;
				case IDC_BUTTON_REMOVE_AUTO_KEY: //Modif N.
					AutoFireKey=0;
					AutoHoldKey=0;
					AutoClearKey=0;
					return true;
					break;
				case IDC_BUTTON_SETKEYSP1:
					Setting_Keys(hDlg, 0, (SendDlgItemMessage(hDlg, IDC_COMBO_PADP1, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1));
					return true;
					break;

				case IDC_BUTTON_SETKEYSP1B:
					Setting_Keys(hDlg, 2, (SendDlgItemMessage(hDlg, IDC_COMBO_PADP1B, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1));
					return true;
					break;

				case IDC_BUTTON_SETKEYSP1C:
					Setting_Keys(hDlg, 3, (SendDlgItemMessage(hDlg, IDC_COMBO_PADP1C, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1));
					return true;
					break;

				case IDC_BUTTON_SETKEYSP1D:
					Setting_Keys(hDlg, 4, (SendDlgItemMessage(hDlg, IDC_COMBO_PADP1D, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1));
					return true;
					break;

				case IDC_BUTTON_SETKEYSP2:
					Setting_Keys(hDlg, 1, (SendDlgItemMessage(hDlg, IDC_COMBO_PADP2, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1));
					return true;
					break;

				case IDC_BUTTON_SETKEYSP2B:
					Setting_Keys(hDlg, 5, (SendDlgItemMessage(hDlg, IDC_COMBO_PADP2B, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1));
					return true;
					break;

				case IDC_BUTTON_SETKEYSP2C:
					Setting_Keys(hDlg, 6, (SendDlgItemMessage(hDlg, IDC_COMBO_PADP2C, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1));
					return true;
					break;

				case IDC_BUTTON_SETKEYSP2D:
					Setting_Keys(hDlg, 7, (SendDlgItemMessage(hDlg, IDC_COMBO_PADP2D, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) & 1));
					return true;
					break;

				case ID_HELP_HELP:
					if (Detect_Format(Manual_Path) != -1)		// can be used to detect if file exist
					{
						strcpy(Str_Tmp, Manual_Path);
						strcat(Str_Tmp, " helpjoypads.html");
						system(Str_Tmp);
					}
					else
					{
						MessageBox(NULL, "You need to configure the Manual Path to have help", "info", MB_OK);
					}
					return true;
					break;
			}
			break;
		case WM_CLOSE:
			End_Input();
			DialogsOpen--;
			EndDialog(hDlg, true);
			return true;
			break;
	}

	return false;
}
int SaveFlags ()
{
	int flags = Z80_State & 0x1;
	flags |= YM2612_Enable << 1;
	flags |=    PSG_Enable << 2;
	flags |=    DAC_Enable << 3;
	flags |=    PCM_Enable << 4;
	flags |=    PWM_Enable << 5;
	flags |=   CDDA_Enable << 6;
	flags |= YM2612_Improv << 7;
	flags |=    PSG_Improv << 8;
	flags |=    DAC_Improv << 9;
	flags |=  ((Sound_Rate / 22050) << 10);
	flags |= LeftRightEnabled << 16;
	return flags;
}
void LoadFlags(int flags)
{
	if (flags & 0x1)
		Z80_State |= 0x1;
	else
		Z80_State &= ~0x1;
	YM2612_Enable = (flags >> 1) & 1;
	PSG_Enable    = (flags >> 2) & 1;
	DAC_Enable    = (flags >> 3) & 1;
	PCM_Enable    = (flags >> 4) & 1;
	PWM_Enable    = (flags >> 5) & 1;
	CDDA_Enable   = (flags >> 6) & 1;
	YM2612_Improv = (flags >> 7) & 1;
	DAC_Improv    = (flags >> 8) & 1;
	PSG_Improv    = (flags >> 9) & 1;
	Change_Sample_Rate(HWnd,(flags >> 10) & 0x3);
	LeftRightEnabled = (flags >> 16) & 1;
	return;
}