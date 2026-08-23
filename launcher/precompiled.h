//======================== reconstructed by oxi, 2026 ========================
//
// re-won-launcher-1792
// WON Half-Life launcher, build 1792
//
// This is a source-level reconstruction of hl.exe, the WON-era Half-Life
// launcher, build 1792 (Sep 20 2001), rebuilt from the retail binary.  It
// exists for educational and archival purposes.  It is non-commercial hobby
// work and is not affiliated with Valve.
//
// $NoKeywords: $
//=============================================================================

#ifndef LAUNCHER_PRECOMPILED_H
#define LAUNCHER_PRECOMPILED_H
#ifdef _WIN32
#pragma once
#endif

// ---------------------------------------------------------------------------
// MFC.  afxwin.h pulls in windows.h itself, and must be the first Windows
// header seen -- so it leads the whole file.
// ---------------------------------------------------------------------------
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>
#include <afxsock.h>
#include <afxinet.h>
#include <afxpriv.h>
#include <afxmt.h>

// ---------------------------------------------------------------------------
// Win32 / multimedia / sockets.
//
// winsock.h and wsipx.h come after MFC on purpose: afxsock.h has already
// brought in winsock2.h, whose _WINSOCK2API_ guard empties winsock.h -- the
// reverse order is the one that collides.
//
// ddraw.h is safe here (it does not reach d3dtypes.h), but d3d.h and dsound.h
// are not: the DX6 SDK copies in external/dx6sdk redefine what the Windows
// SDK's d3d9types.h declares, and MFC's afxwin.h drags that in unconditionally
// through afxrendertarget.h.  The TUs that need those two headers are the five
// excluded from this PCH.
// ---------------------------------------------------------------------------
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <winsock2.h>
#include <winsock.h>
#include <wsipx.h>
#include <ddraw.h>

// ---------------------------------------------------------------------------
// C runtime.
// ---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <mbstring.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <io.h>
#include <direct.h>
#include <intrin.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

// ---------------------------------------------------------------------------
// VGUI 1 (launcher/vgui1/include) -- used by the VGUI menu shim.
// ---------------------------------------------------------------------------
#include <VGUI.h>
#include <VGUI_App.h>

// ---------------------------------------------------------------------------
// Resource ids: shell/hl_res/resource.h and the launcher's own dialog ids.
// ---------------------------------------------------------------------------
#include "resource.h"
#include "resource_dlg.h"

// ---------------------------------------------------------------------------
// Engine-lineage and vendored headers reached from outside launcher/.
// ---------------------------------------------------------------------------
#include "re.h"
#include "protocol.h"
#include "version.h"
#include "dll_state.h"
#include "unzip.h"

// ---------------------------------------------------------------------------
// WON: the crypto/auth wrapper (hlwon/cryptapi), the TitanApi message types
// and the directory / factory / message client (hlwon/won).
// ---------------------------------------------------------------------------
#include "cryptapi.h"
#include "ReadBuffer.h"
#include "WriteBuffer.h"
#include "AuthRequest.h"
#include "EasySocket.h"
#include "EasyTitanSocket.h"
#include "TitanRequest.h"
#include "msg/HeaderTypes.h"
#include "msg/Chat/TMsgChat.h"
#include "won_msg.h"
#include "won_dir.h"
#include "won_factoryreq.h"
#include "../hlwon/cryptapi/crc.h"

// ---------------------------------------------------------------------------
// The launcher's own headers -- app-wide state and helpers first, then the
// controls, dialogs and services in dependency order.
// ---------------------------------------------------------------------------
#include "launcher.h"
#include "strings.h"
#include "common.h"
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "scriptobject.h"
#include "AdvancedMPDlg.h"
#include "ODSlider.h"
#include "AudioDlg.h"
#include "BorderlessEdit.h"
#include "HLMainDlg.h"
#include "ConfigureDlg.h"
#include "CreateRoomDialog.h"
#include "ODComboBox.h"
#include "ODScrollBar.h"
#include "ODListBox.h"
#include "CreateServerDlg.h"
#include "DlgConnectableBase.h"
#include "DlgPopupBase.h"
#include "ODMenu.h"
#include "FilterDialog.h"
#include "mod.h"
#include "GameOptionsDlg.h"
#include "GoreDlg.h"
#include "MessageBuffer.h"
#include "Token.h"
#include "ServerInfo.h"
#include "serverconnection.h"
#include "HLAsyncSocket.h"
#include "HLChatLineCtrl.h"
#include "LogoDlg.h"
#include "Profile.h"
#include "ODListCtrl.h"
#include "chatclient.h"
#include "rooms.h"
#include "RoomDialog.h"
#include "NetGame.h"
#include "HLLanAsyncSocket.h"
#include "HLMasterAsyncSocket.h"
#include "HLModSocket.h"
#include "InputDlg.h"
#include "KeyboardDlg.h"
#include "ODHLListCtrl.h"
#include "LanDlg.h"
#include "LauncherServers.h"
#include "LoadDlg.h"
#include "LoadSaveDlg.h"
#include "LoginDlg.h"
#include "ModDlg.h"
#include "ModDownloadDlg.h"
#include "ModHttpDownloadDlg.h"
#include "ModInfoSocket.h"
#include "ModReqDlg.h"
#include "MultiSelectDlg.h"
#include "NewGameDlg.h"
#include "ODChatEdit.h"
#include "ODEdit.h"
#include "ODIRCUserListCtrl.h"
#include "ODTabCtrl.h"
#include "odlistctrls.h"
#include "PlayerInfoDlg.h"
#include "PlayerProfileDlg.h"
#include "PromptDlg.h"
#include "ReadmeDlg.h"
#include "RefreshDlg.h"
#include "SaveDlg.h"
#include "SetInfoDlg.h"
#include "SpecGameDlg.h"
#include "VidSelectDlg.h"
#include "VideoDlg.h"
#include "engine.h"
#include "vid.h"
#include "VideoModeDlg.h"
#include "cd_win.h"
#include "connect.h"
#include "dibapi.h"
#include "gameui.h"
#include "info.h"
#include "joystick.h"
#include "keylist.h"
#include "scheme.h"
#include "snd_mem.h"
#include "snd_win.h"
#include "sys_cpu.h"
#include "sys_win.h"
#include "viddef.h"
#include "volume.h"
#include "wavelib.h"

#endif // LAUNCHER_PRECOMPILED_H
