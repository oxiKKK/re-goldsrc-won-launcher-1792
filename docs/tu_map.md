# `hl.exe` build 1792 memory map

Retail `hl.exe` laid out by address: headers, sections, launcher source files, static libraries.

Source: PE headers (entry `0x484282`, image size `0x119000`) and the IDA database.

Addresses are virtual (base `0x400000`) unless marked *file*.

---

## 0. At a glance

```
0x400000 ┌──────────────────────────────┐  file 0x0000
         │ PE headers                   │  DOS hdr, stub, Rich, PE/COFF/optional, section table
0x401000 ├──────────────────────────────┤  file 0x1000   .text  (r-x)
         │ launcher TUs (link order)    │  0x401000-0x4692e0   ~85 bands, 1866 fn
         │ vgui Dar<T> + launcher tail  │  0x4692e0-0x46e850   vgui1 glue, video/audio pages
         │ WON WriteBuffer              │  0x46e850-0x46eaa0
         │ Info-ZIP UnZip (static)      │  0x46eaa0-0x47a558
         │ MFC slice                    │  0x47a558-0x47b2e0
         │ vgui/DDRAW import stubs      │  0x47b2e0-0x47b450
         │ sys_cpu + WON auth/crypt     │  0x47b450-0x480c90
         │ MSVC6 CRT (static)           │  0x480c90-0x490760
         │ import stubs, STL, CTL3D     │  0x490760-0x494297
         │ MFC 4.2 (static)             │  0x494297-0x4a2b80
         │ C++ EH unwind funclets       │  0x4a2b80-0x4aba2e
         │ (section padding)            │  0x4aba2e-0x4ac000
0x4ac000 ├──────────────────────────────┤  file 0xac000  .rdata (r--)
         │ IAT                          │  0x4ac000-0x4ac8cc
         │ per-object .rdata (app, lib) │  0x4ac8cc-0x4b72b8   message maps, vftables, consts
         │ RTTI                         │  0x4b72b8-0x4bab70
         │ C++ EH tables                │  0x4bab70-0x4c44b8
         │ import directory + names     │  0x4c44b8-0x4c7f70
         │ export directory             │  0x4c7f70-0x4cc526
0x4cd000 ├──────────────────────────────┤  file 0xcd000  .data  (rw-)
         │ initialized data             │  0x4cd000-0x4d9710   strings, RTTI type descriptors, globals
         │ zero-initialized (BSS)       │  0x4d9710-0x50cd08   not in the file
0x50d000 ├──────────────────────────────┤  file 0xda000  .rsrc  (r--)
         │ resources                    │  0x50d000-0x518498
0x519000 └──────────────────────────────┘  SizeOfImage 0x119000
```

---

## 1. PE headers — `0x400000–0x401000` (file `0x0000–0x1000`)

| file range | region | notes |
|---|---|---|
| `0x0000–0x0040` | `IMAGE_DOS_HEADER` | `e_lfanew` = `0x118` |
| `0x0040–0x0080` | DOS stub | "This program cannot be run in DOS mode" |
| `0x0080–0x0110` | Rich header | 15 tool entries, see below |
| `0x0118–0x011c` | `PE\0\0` | |
| `0x011c–0x0130` | `IMAGE_FILE_HEADER` | i386, 4 sections, `TimeDateStamp` 2001-09-11 19:18:51 UTC |
| `0x0130–0x0210` | `IMAGE_OPTIONAL_HEADER32` | linker 6.0, GUI subsystem, entry `0x484282`, base `0x400000`, align `0x1000`/`0x1000`, stack/heap reserve `0x100000` |
| `0x0210–0x02b0` | section table | 4 × 40 bytes |
| `0x02b0–0x1000` | zero padding | `SizeOfHeaders` = `0x1000` |

**Data directories**

| directory | address | size |
|---|---|---|
| Export | `0x4c7f70` | `0x45b6` |
| Import | `0x4c44b8` | `0x140` (15 DLLs + terminator) |
| Resource | `0x50d000` | `0xb498` |
| IAT | `0x4ac000` | `0x8cc` |

No relocations, no debug directory, no TLS, no load config.

**Rich header** (`prodid` / build / count, as stored):
`12/7291 ×18` · `11/8047 ×13` · `14/7299 ×34` · `10/8047 ×187` · `49/9044 ×8` ·
`11/8168 ×1` · `19/8034 ×22` · `1/0 ×618` · `2/7303 ×38` · `11/8569 ×70` ·
`0/0 ×59` · `11/8966 ×125` · `10/8966 ×16` · `6/1735 ×1` (cvtres) · `4/8447 ×3` (link).

---

## 2. Sections

| section | VA range | virtual size | file range | raw size | flags |
|---|---|---|---|---|---|
| `.text`  | `0x401000–0x4aba2e` | `0xaaa2e` | `0x01000–0xac000` | `0xab000` | `60000020` code, r-x |
| `.rdata` | `0x4ac000–0x4cc526` | `0x20526` | `0xac000–0xcd000` | `0x21000` | `40000040` r-- |
| `.data`  | `0x4cd000–0x50cd08` | `0x3fd08` | `0xcd000–0xda000` | `0xd000`  | `c0000040` rw- |
| `.rsrc`  | `0x50d000–0x518498` | `0xb498`  | `0xda000–0xe6000` | `0xc000`  | `40000040` r-- |

No `.idata` section. The IAT is at the start of `.rdata` (`0x4ac000–0x4ac8cc`); IDA shows it as a fake `.idata` segment. File size `0xe6000` (942,080 bytes). Only `.rsrc` has a file offset different from its RVA.

---

## 3. `.text` — `0x401000–0x4ac000`

Link order: app objects, then static libs, then EH funclets. "fn" = IDA function count.

| range | size | fn | region | notes |
|---|---|---|---|---|
| `0x401000–0x4692e0` | `0x682e0` | 1866 | **Launcher TUs** | one band per file — §3.1 |
| `0x4692e0–0x46a220` | `0xf40` | 35 | **vgui1 `Dar<T>`** | `vgui::Dar<T>` / `App` / `Cursor` template code, exported (§5.5). Contains `vgui_menu.cpp` (`0x4696a0–0x469720`) and `LauncherApp::LauncherApp` `0x46a1e0` |
| `0x46a220–0x46e850` | `0x4630` | 67 | **Launcher TUs (tail)** | `VideoDlg`, `VideoModeDlg`, `VidSelectDlg`, `volume`, `wavelib` — §3.1 |
| `0x46e850–0x46eaa0` | `0x250` | 15 | **WON `WriteBuffer`** | `hlwon/cryptapi/WriteBuffer.cpp` |
| `0x46eaa0–0x47a558` | `0xbab8` | 96 | **Info-ZIP UnZip** | `external/unzip`, for mod downloads. Entry `Launcher_Unzip` `0x477490` |
| `0x47a558–0x47b2e0` | `0xd88` | 73 | **MFC slice** | multimon stubs, `CFileFind`, `CCriticalSection`, `CSyncObject`, `CDC` text helpers. Not 16-aligned, so not ours |
| `0x47b2e0–0x47b450` | `0x170` | 61 | **Import stubs (vgui, DDRAW)** | `jmp [IAT]` thunks |
| `0x47b450–0x47b630` | `0x1e0` | 5 | **`sys_cpu.cpp`** | `Sys_CPUIDSupported` … `Sys_CheckMMXTechnology` |
| `0x47b630–0x47b830` | `0x200` | 2 | *(unknown)* | `sub_47B630`, folded `std::ios_base::~ios_base` |
| `0x47b830–0x480c90` | `0x5460` | 57 | **WON auth / crypt** | `WONAuth_*`, `AuthRequest.cpp` `0x47b8c0`, `CRC32_*` `0x47e050`, `MD5*` `0x47e2e0`, `Crypt_ReturnAPI` `0x47f010` |
| `0x480c90–0x490760` | `0xfad0` | 412 | **MSVC6 CRT** | EH (`___CxxFrameHandler` `0x480d66`), RTTI, stdio, heap, entry `start` `0x484282` |
| `0x490760–0x490808` | `0xa8` | 28 | **Import stubs** | `RtlUnwind`, `GetFileTitleA`, WINSPOOL, 23 `WSOCK32` calls |
| `0x490808–0x490b60` | `0x358` | 17 | **STL** | `std::_Xlen`, `std::_Xran` |
| `0x490b60–0x494297` | `0x3737` | 57 | **CTL3D** | `ctl3d32s` |
| `0x494297–0x4a2b80` | `0xe8e9` | 843 | **MFC 4.2** | `_WinMain@16` `0x494297` … `AfxWinTerm` `0x4a2ac1` |
| `0x4a2b80–0x4aba2e` | `0x8eae` | — | **EH funclets** | `.text$x` unwind code |
| `0x4aba2e–0x4ac000` | `0x5d2` | — | padding | |

### 3.1 Launcher TUs

`#` = band number (`Band:` in the file header). Paths are under `shell/netgame/` unless given. Out-of-order bands (2b, 2c, 62b, 78b, 79) are files the linker placed out of alphabetical order.

| range | # | file | examples |
|---|---|---|---|
| `0x401000–0x4020a0` | 1 | `AdvancedMPDlg.cpp` | `CDKey_Checksum`, `CAdvancedMPDlg::DoDataExchange`, `CAdvancedMPDlg::DestroyControls`, `CAdvancedMPDlg::OnPrevPage`, `CServerDescription::CServerDescription` |
| `0x4020a0–0x402db0` | 2 | `AudioDlg.cpp` | `CAudioDlg::CAudioDlg`, `CAudioDlg::OnInitDialog`, `CAudioDlg::OnOK`, `CAudioDlg::OnUseCD`, `CAudioDlg::OnEAX` |
| `0x402db0–0x4036d0` | 3 | `BorderlessEdit.cpp` | `CBorderLessEdit::CBorderLessEdit`, `CBorderLessEdit::OnEraseBkgnd`, `CBorderLessEdit::SetBorderColor`, `CBorderLessEdit::OnSize`, `CBorderLessEdit::SetPasswordMode` |
| `0x4036d0–0x4041e0` | 4 | `cd_win.cpp` (first half) | `CDAudio_ResetTiming`, `CDAudio_GetAudioDiskInfo`, `CDAudio_Pause`, `CDAudio_SwitchToEngineCommand`, `CDAudio_OpenDevice` |
| `0x4041e0–0x404840` | 5–6 | `chatclient.cpp` | `chatroom_t::chatroom_t`, `CRoomList::Unlink`, `CChatClient::~CChatClient`, `CChatClient::GetSelfStatusRaw`, `CChatUser::~CChatUser` |
| `0x404840–0x4055d0` | 7 | `ConfigureDlg.cpp` | `CConfigureDlg::CConfigureDlg`, `CConfigureDlg::DoDataExchange`, `CConfigureDlg::OnAudio`, `CConfigureDlg::OnAutopatch`, `CConfigureDlg::OnOK` |
| `0x4055d0–0x406960` | 7b | `connect.cpp` | `Connect_GetTimeout`, `Connect_ServiceStage`, `Connect_BeginPingStage`, `Connect_ServiceConnecting`, `Launcher_HandleConnectFailure` |
| `0x406960–0x407070` | 8 | `CreateRoomDialog.cpp` | `CCreateRoomDialog::CCreateRoomDialog`, `CCreateRoomDialog::SetupButtonStrips`, `CCreateRoomDialog::DoDataExchange`, `CCreateRoomDialog::OnInitDialog`, `CCreateRoomDialog::RMLPreIdle` |
| `0x407070–0x408300` | 9 | `CreateServerDlg.cpp` | `CCreateServerDlg::CCreateServerDlg`, `CCreateServerDlg::LoadButtonStrips`, `CCreateServerDlg::RefreshAndShow`, `CCreateServerDlg::OnAdvanced`, `CCreateServerDlg::OnDedicated` |
| `0x408310–0x409090` | 10 | `ddraw.cpp` | `DDraw_QueryD3D`, `DDraw_EnumCallback`, `DDraw_ReleaseSurfaces`, `DDraw_CreateSurfaces`, `DDraw_ErrorString` |
| `0x409090–0x409390` | 11 | `dibapi.cpp` | `DIB_LoadBitmapFile`, `DIB_FindBits`, `DIB_Width`, `DIB_Height`, `DIB_NumColors` |
| `0x409390–0x4099f0` | 12 | `hlwon/won/won_dir.cpp` + `hlwon/won/won_dirparse.cpp` | `DirRequest::getDirectory`, `WONComm_GetDirectory`, `WON_ParseDirReply` |
| `0x4099f0–0x40af40` | 13 | `DlgBase.cpp` | `CDlgBase::RunModalLoop`, `CDlgBase::SetFramePump`, `CDlgBase::InitChildDialog`, `CDlgBase::PreTranslateMessage`, `Dlg_CenterWindow` |
| `0x40af40–0x40ba50` | 14 | `DlgConnectableBase.cpp` | `CDlgConnectableBase::CDlgConnectableBase`, `CDlgConnectableBase::RefreshAfterGameDirChange`, `CDlgConnectableBase::VerifyServerBeforeConnect`, `CDlgConnectableBase::CheckModVersion`, `CDlgConnectableBase::ConnectToSelectedServer` |
| `0x40ba50–0x40c110` | 15 | `DlgPopupBase.cpp` | `CDlgPopupBase::RunModalLoop`, `CDlgPopupBase::DoModal`, `CDlgPopupBase::CDlgPopupBase`, `CDlgPopupBase::~CDlgPopupBase`, `CDlgPopupBase::OnPaint` |
| `0x40c110–0x40d0d0` | 16 | `hlwon/cryptapi/EasySocket.cpp` | `EasySocket::EasySocket`, `EasySocket::stopWinsock`, `EasySocket::getNewDescriptor`, `EasySocket::recvBufferFrom`, `EasySocket::WSAErrorToEnum` |
| `0x40d0d0–0x40e1f0` | 17a | `hlwon/cryptapi/EasySocketError.cpp` | `ES_ErrorTypeToString` |
| `0x40e1f0–0x40e440` | 17b | `hlwon/cryptapi/EasyTitanSocket.cpp` | `EasyTitanSocket::setMaxMsgSize`, `EasyTitanSocket::EasyTitanSocket`, `EasyTitanSocket::~EasyTitanSocket`, `EasyTitanSocket::recvTMessage` |
| `0x40e440–0x40f330` | 18 | `engine.cpp` | `Sys_Init`, `Eng_Shutdown`, `Eng_StartupEngine`, `AntiCheat_ScanForCheatWindows`, `Eng_ConnectToServer` |
| `0x40f330–0x40f560` | — | `hlwon/won/won_factory.cpp` | `WONComm_StartProcess` |
| `0x40f560–0x4112e0` | 19 | `FilterDialog.cpp` | `CFilterDialog::CFilterDialog`, `CFilterDialog::SetupHeaderButtons`, `CFilterDialog::OnDisplayChange`, `CFilterDialog::OnSelectPing`, `CFilterDialog::OnIsNotProxy` |
| `0x4112e0–0x411400` | — | shared popup paint handlers (map `0x4ADD38`) | `CLoginDlg::OnEraseBkgnd`, `CModReqDlg::OnEraseBkgnd`, `CLoginDlg::OnPaint`, `CModHttpDownloadDlg::OnPaint`, `CRefreshDlg::OnPaint` |
| `0x411400–0x412b90` | 20 | `GameOptionsDlg.cpp` | `CGameOptionsDlg::CGameOptionsDlg`, `CGameOptionsDlg::OnEraseBkgnd`, `CGameOptionsDlg::OnJoystick`, `CGameOptionsDlg::OnMouseFilter`, `CGameOptionsDlg::OnSliderScroll` |
| `0x412b90–0x413e90` | 21 | `gameui.cpp` | `AllocGameMem`, `SleepUntilInput`, `Vid_D3DSupported`, `D_SurfaceCacheForRes`, `Vid_BuildModeList` |
| `0x413e90–0x4148c0` | 22 | `GoreDlg.cpp` | `CGoreDlg::CGoreDlg`, `CGoreDlg::SetupButtonStrip`, `CGoreDlg::OnOK`, `CGoreDlg::OnInitDialog`, `CGoreDlg::OnGoreCheck` |
| `0x4148c0–0x415130` | 23a | `HLAsyncSocket.cpp` | `CHLAsyncSocket::CHLAsyncSocket`, `CHLAsyncSocket::OnReceive`, `CHLAsyncSocket::ParseInfoResponse`, `CHLAsyncSocket::ParsePlayerList`, `CHLAsyncSocket::ParsePingReply` |
| `0x415130–0x4155b0` | 23b | `HLChatLineCtrl.cpp` | `CHLChatLineCtrl::CHLChatLineCtrl`, `CHLChatLineCtrl::~CHLChatLineCtrl`, `CHLChatLineCtrl::Submit`, `CHLChatLineCtrl::ClearEditSelection`, `CHLChatLineCtrl::PreTranslateMessage` |
| `0x4155b0–0x415d70` | 24 | `HLLanAsyncSocket.cpp` | `CHLLanAsyncSocket::CHLLanAsyncSocket`, `CHLLanAsyncSocket::OnSend`, `CHLLanAsyncSocket::OnReceive`, `CHLLanAsyncSocket::PublishLanServer`, `CHLLanAsyncSocket::Open` |
| `0x415d70–0x41a1a0` | 25 | `HLMainDlg.cpp` | `Eng_DeferRelayout1`, `Eng_DeferMenuButton`, `CHLMainDlg::OnInitDialog`, `CHLMainDlg::LayoutMainMenu`, `CHLMainDlg::OnTfcManual` |
| `0x41a1a0–0x41aa20` | 26 | `HLMasterAsyncSocket.cpp` | `CHLMasterAsyncSocket::CHLMasterAsyncSocket`, `CHLMasterAsyncSocket::SetListDone`, `CHLMasterAsyncSocket::RequestServerBatch`, `CHLMasterAsyncSocket::BeginFetch`, `CHLMasterAsyncSocket::FlushSend` |
| `0x41aa20–0x41aed0` | 27 | `HLModSocket.cpp` | `CHLModSocket::CHLModSocket`, `CHLModSocket::StartList`, `CHLModSocket::SendModListRequest`, `CHLModSocket::ParseModList`, `CHLModSocket::SendInstallNotify` |
| `0x41aed0–0x41b250` | 27b | `info.cpp` | `Info_ValueForKey`, `Info_RemoveKey`, `Info_SetValueForStarKey`, `Info_SetValueForKey` |
| `0x41b250–0x41b900` | 28 | `InputDlg.cpp` | `CInputEdit::CInputEdit`, `CInputDlg::CInputDlg`, `CInputDlg::DoDataExchange`, `CInputDlg::OnInitDialog`, `CInputDlg::SetPasswordMode` |
| `0x41b900–0x41be30` | 29 | `joystick.cpp` | `Joy_Detect`, `Joy_RawValuePointer`, `Joy_AdvancedUpdate`, `Joy_ReadJoystick`, `Joy_GetPressedButton` |
| `0x41be30–0x420390` | 30 | `KeyboardDlg.cpp` | `CODKeySearchComboBox::CODKeySearchComboBox`, `CODKeyBindingCtrl::SelectNextPage`, `CKeyboardDlg::DoDataExchange`, `CKeyboardDlg::OnOK`, `CKeyboardDlg::GetKeyDisplayName` |
| `0x420390–0x421850` | 31 | `LanDlg.cpp` | `CLan::CLan`, `CLan::InitButtonStrips`, `CLan::OnConnect`, `CLan::OnSpectate`, `CLan::OnServerInfo` |
| `0x421850–0x424cf0` | 32 | `launcher.cpp` | `RunSierraUpdate`, `Launcher_GetGLDriver`, `CNetGameApp::InitInstance`, `Launcher_LoadSplashBitmap`, `Launcher_WriteProfileString` |
| `0x424cf0–0x425850` | 33 | `LauncherServers.cpp` | `CServerAddr::CServerAddr`, `CFavorites::Dedup`, `CFavorites::AddServerMod`, `CFavorites::NextMasterList`, `CFavorites::GetAuthServers` |
| `0x425850–0x426d70` | 34 | `LoadDlg.cpp` | `CLoadDlg::CLoadDlg`, `CLoadDlg::~CLoadDlg`, `Launcher_ParseSaveFile`, `CLoadDlg::UpdateButtonStates`, `Launcher_GetPlayerName` |
| `0x426d70–0x427480` | 35 | `LoadSaveDlg.cpp` | `CLoadSaveDlg::CLoadSaveDlg`, `CLoadSaveDlg::~CLoadSaveDlg`, `CLoadSaveDlg::DoDataExchange`, `CLoadSaveDlg::OnSaveGame`, `CLoadSaveDlg::OnInitDialog` |
| `0x427480–0x427d30` | 36 | `LoginDlg.cpp` | `CLoginDlg::CLoginDlg`, `CLoginDlg::DoDataExchange`, `CLoginDlg::OnCtlColor`, `CLoginDlg::OnCancel`, `CLoginDlg::SetStatusLine` |
| `0x427d30–0x429010` | 37 | `LogoDlg.cpp` | `Launcher_FormatMCIError`, `CLogoDlg::~CLogoDlg`, `CLogoDlg::FlushInputAndClose`, `MasterFetch_Init`, `MasterFetch_Service` |
| `0x429010–0x4293c0` | 38 | `MessageBuffer.cpp` | `CMessageBuffer::CMessageBuffer`, `CMessageBuffer::MSG_WriteString`, `CMessageBuffer::MSG_ReadShort`, `CMessageBuffer::SZ_Alloc`, `CMessageBuffer::SZ_Write` |
| `0x4293c0–0x42a840` | 39–40 | `mod.cpp` | `mod_t::Init`, `mod_t::GetKeyString`, `Mod_ParseLiblist`, `Launcher_OnGameDirChanged`, `CODModListCtrl::OnLButtonDown` |
| `0x42a840–0x42c430` | 41 | `ModDlg.cpp` | `CModDlg::CModDlg`, `CModDlg::OnInitDialog`, `CModDlg::UpdateButtons`, `CModDlg::OnInstall`, `CModDlg::UpdateButtonStates` |
| `0x42c430–0x42da20` | 42 | `ModDownloadDlg.cpp` | `CModDownloadDlg::CModDownloadDlg`, `CModDownloadDlg::OnInitDialog`, `CModDownloadDlg::ServiceDownload`, `ModDownload_NewFile`, `CModDownloadDlg::ExtractArchive` |
| `0x42da20–0x42df10` | 42b | `modes.cpp` | `VID_GetModePtr`, `VID_Update`, `VID_UnlockBuffer`, `VID_GetVID`, `VID_GetExtModeDescription` |
| `0x42df10–0x42f7b0` | 43 | `ModHttpDownloadDlg.cpp` | `CModHttpDownloadDlg::CModHttpDownloadDlg`, `CModHttpDownloadDlg::OnInitDialog`, `CModHttpDownloadDlg::ServiceDownload`, `ModHttp_NewFile`, `CModHttpDownloadDlg::ExtractArchive` |
| `0x42f7b0–0x42fd00` | 44 | `ModInfoSocket.cpp` | `CModInfoSocket::CModInfoSocket`, `CModInfoSocket::OnReceive`, `CModInfoSocket::Pump`, `CModInfoSocket::SendRequest`, `CModInfoSocket::ParseModList` |
| `0x42fd00–0x430370` | 45 | `ModReqDlg.cpp` | `CModReqDlg::CModReqDlg`, `CModReqDlg::~CModReqDlg`, `CModReqDlg::DoDataExchange`, `CModReqDlg::StartCustomQuery`, `CModReqDlg::OnInitDialog` |
| `0x430370–0x431d00` | 46 | `MultiSelectDlg.cpp` | `CMultiSelectDlg::CMultiSelectDlg`, `CMultiSelectDlg::OnSpectateBtn`, `CMultiSelectDlg::OnOK`, `CMultiSelectDlg::RMLPreIdle`, `CMultiSelectDlg::OnControls` |
| `0x431d00–0x43e030` | 47–48 | `NetGame.cpp` | `AntiCheat_EncodeString`, `CServerBrowserDlg::SetFavoriteOnSelection`, `NetGame_CreateSheet`, `ServerBrowser_CompareAddr`, `CNetGameDlg::Pump` |
| `0x43e030–0x43e920` | 49 | `NewGameDlg.cpp` | `CNewGameDlg::CNewGameDlg`, `CNewGameDlg::DoDataExchange`, `CNewGameDlg::OnSkillEasy`, `CNewGameDlg::OnSkillMedium`, `CNewGameDlg::StartNewGame` |
| `0x43e920–0x442890` | 50 | `ODButton.cpp` | `CODBitmapButton::CODBitmapButton`, `CODBlendBtn::BlendStates`, `CODBlendBtn::EnsureSkinLoaded`, `CODBlendBtn::OnLButtonDown`, `CODBlendStatic::OnTimer` |
| `0x442890–0x443fb0` | 51 | `ODChatEdit.cpp` | `CODChatEdit::CODChatEdit`, `CODChatEdit::OnNcDestroy`, `CODChatEdit::DrawLine`, `CODChatEdit::UpdateScrollbarVisibility`, `CODChatEdit::OnCreate` |
| `0x443fb0–0x446e10` | 52 | `ODComboBox.cpp` | `CODComboBox::CODComboBox`, `CODComboBox::SetCurSel`, `CODComboBox::OnMouseMove`, `CODComboBox::DropDown`, `CODDriverComboBox::Paint` |
| `0x446e10–0x447940` | 53 | `ODEdit.cpp` | `CODEdit::CODEdit`, `CODEdit::OnCreate`, `CODEdit::OnPaint`, `CODEdit::VisibleLines`, `ODList_CompareRefreshOrder` |
| `0x447940–0x448ec0` | 54 | `ODHLListCtrl.cpp` | `CODHLListCtrl::CODHLListCtrl`, `CODHLListCtrl::DrawRow`, `CODHLListCtrl::OnSetFocus`, `CODHLListCtrl::OnLButtonDblClk`, `CODHLListCtrl::OnLButtonDown` |
| `0x448ec0–0x449400` | 55 | `ODIRCUserListCtrl.cpp` | `CODIRCUserListCtrl::CODIRCUserListCtrl`, `CODIRCUserListCtrl::~CODIRCUserListCtrl`, `CODIRCUserListCtrl::AddRow`, `CODIRCUserListCtrl::DrawRow` |
| `0x449400–0x44a5e0` | 56 | `ODListBox.cpp` | `CODListBox::CODListBox`, `CODListBox::NavLineUp`, `CODListBox::OnVScroll`, `CODListBox::ClampTopIndex`, `CODListBox::OnKeyDown` |
| `0x44a5e0–0x44cc70` | 57 | `ODListCtrl.cpp` | `CODListCtrl::CODListCtrl`, `CODListCtrl::OnPaint`, `CODList_CompareRows`, `CODListCtrl::GetColumnCount`, `CODColorPane::OnEraseBkgnd` |
| `0x44cc70–0x44db60` | 58 | `ODMenu.cpp` | `CODMenu::CODMenu`, `CODMenu::MeasureItem`, `CODPingComboBox::DrawRow`, `CODRuleListCtrl::DrawRow`, `CODSaveGameListCtrl::DrawRow` |
| `0x44db60–0x44f9f0` | 59 | `ODScrollBar.cpp` | `CODScrollBar::CODScrollBar`, `CODScrollBar::DrawDnArrow`, `CODScrollBar::OnMouseMove`, `CODScrollBar::HitTestDnArrow`, `CODScrollBar::OnSetFocus` |
| `0x44f9f0–0x450670` | 60 | `ODSlider.cpp` | `CODSlider::CODSlider`, `CODSlider::SetRange`, `CODSlider::DrawThumb`, `CODSlider::GetThumbRect`, `CODSlider::OnSetFocus` |
| `0x450670–0x450be0` | 61 | `ODStatic.cpp` | `CODStatic::CODStatic`, `CODStatic::OnPaint`, `CODStatic::SetWindowText`, `CODStatic::SetFontSize`, `CODStatic::SetOffsets` |
| `0x450be0–0x451a50` | 62 | `ODTabCtrl.cpp` | `CODTabCtrl::CODTabCtrl`, `CODTabCtrl::OnNcDestroy`, `CODTabCtrl::AddTab`, `CODTabCtrl::GetTabRectSingleRow`, `CODTabCtrl::OnKeyDown` |
| `0x451a50–0x451c70` | 62b | `VideoModeDlg.cpp` (displaced) | `CODVideoList::DrawRow` |
| `0x451c70–0x452610` | 63 | `PlayerInfo.cpp` | `CPlayerInfo::Save`, `CPlayerInfo::SetKey`, `CPlayerInfo::Parse`, `CPlayerInfo::~CPlayerInfo`, `CODPlayerListCtrl::DrawRow` |
| `0x452610–0x4530e0` | 64 | `PlayerInfoDlg.cpp` | `CPlayerInfoDlg::CPlayerInfoDlg`, `CPlayerInfoDlg::LoadHeaderStrip`, `CPlayerInfoDlg::~CPlayerInfoDlg`, `CPlayerInfoDlg::DoDataExchange`, `CPlayerInfoDlg::OnInitDialog` |
| `0x4530e0–0x453690` | 64 | thunk `0x4530e0`; `CServerBrowser` `0x453100` (`NetGame.cpp`); `CGameConfig_IsEqual` `0x4532e0` (`Profile.cpp`) | `CServerBrowser::CServerBrowser`, `CServerBrowser::~CServerBrowser`, `CServerBrowser::SaveFavoriteServers`, `ServerBrowser_CopyConfig`, `CGameConfig_IsEqual` |
| `0x453690–0x4582b0` | 65 | `PlayerProfileDlg.cpp` | `CPlayerProfileDlg::CPlayerProfileDlg`, `PopulateColorCombo`, `CPlayerProfileDlg::OnOK`, `CPlayerProfileDlg::ApplyModelPalette`, `CPlayerProfileDlg::OnVoiceEnable` |
| `0x4582b0–0x45a2d0` | 66 | `Profile.cpp` | `CFG_FreeBindings`, `Launcher_SavePlayerInfoTo`, `CFG_FreeCmdList`, `CGameConfig_ParseKey`, `CFG_LoadFile` |
| `0x45a2d0–0x45ab30` | 67 | `PromptDlg.cpp` | `CPromptDlg::CPromptDlg`, `CPromptDlg::SetMessage`, `CPromptDlg::SetTextAlign`, `CPromptDlg::SetCheckboxShown`, `CPromptDlg::SetTitle` |
| `0x45ab30–0x45adb0` | 68 | `hlwon/won/won_msg.cpp` | `CWONMsg::CWONMsg`, `CWONMsg::SetBuffer`, `CWONMsg::ReadWString`, `CWONMsg::ReadRemaining` |
| `0x45adb0–0x45b1f0` | 69 | `ReadmeDlg.cpp` | `CReadmeDlg::CReadmeDlg`, `CReadmeDlg::DoDataExchange`, `CReadmeDlg::~CReadmeDlg`, `CReadmeDlg::OnInitDialog` |
| `0x45b1f0–0x45c150` | 70 | `RefreshDlg.cpp` | `CRefreshDlg::CRefreshDlg`, `CRefreshDlg::StampServerRecords`, `CRefreshDlg::OnCancel`, `CRefreshDlg::DrivePass`, `CRefreshDlg::Pump` |
| `0x45c150–0x45c8f0` | 71 | `rooms.cpp` | `Rooms_Load`, `CODRoomListCtrl::AddRoomRow`, `CODRoomListCtrl::DrawRow` |
| `0x45c8f0–0x45dd70` | 72 | `RoomDialog.cpp` | `CRoomDialog::CRoomDialog`, `CRoomDialog::GetPickedRoomName`, `CRoomDialog::OnCancel`, `CRoomListCtrl::OnSetTextColor`, `CRoomStatic::SetText` |
| `0x45dd70–0x45f410` | 73 | `SaveDlg.cpp` | `CSaveDlg::CSaveDlg`, `CSaveDlg::OnInitDialog`, `Save_GetSaveDir`, `CSaveDlg::UpdateButtonStates`, `Save_ClearRecord` |
| `0x45f410–0x460770` | 74 | `scriptobject.cpp` | `CScriptListItem::CScriptListItem`, `UTIL_StripInvalidCharacters`, `CScriptObject::ReadFromBuffer`, `CDescription::InitFromFile`, `CDescription::TransferCurrentValues` |
| `0x460770–0x462530` | 75 | `ServerInfo.cpp` | `CServerRule::CServerRule`, `CServerInfo::SetKeyValue`, `CServerInfo::SendInfoRequest`, `CServerInfo::BeginPlayerQuery`, `CServerInfo::GetPingTime` |
| `0x462530–0x463740` | 75a | `SetInfoDlg.cpp` | `CSetInfoDlg::CSetInfoDlg`, `CSetInfoDlg::ApplyToDescription`, `CSetInfoDlg::OnInitDialog`, `CSetInfoDlg::OnPrevPage`, `CInfoDescription::CInfoDescription` |
| `0x463740–0x464680` | 76 | `vid_win.cpp` | `VID_DestroyDIB`, `VID_SetMode`, `VID_ChangeDisplaySettings`, `VID_SwitchToLauncher`, `VID_ShowEngineWindow` |
| `0x464680–0x4648a0` | 77 | `keylist.cpp` | `KeyList_Remove`, `KeyList_Set`, `KeyList_GetKey`, `KeyList_Clear`, `KeyList_ToString` |
| `0x4648a0–0x464b50` | 78 | `snd_mem.cpp` | `GetLittleShort`, `GetLittleLong`, `FindNextChunk`, `FindChunk`, `GetWavinfo` |
| `0x464b50–0x4652d0` | 78 | `snd_win.cpp` | `Snd_AcquireSlot`, `Snd_PlayMenuSound`, `Snd_AllocBuffers`, `Eng_PreLoad`, `Snd_ResetSlots` |
| `0x4652d0–0x4654d0` | 78b | `SpecGameDlg.cpp` (displaced) | `CSpecGameDlg::CSpecGameDlg`, `CSpecGameDlg::SetupJoinButtonStrip`, `CSpecGameDlg::ApplyFilter`, `CSpecGameDlg::LoadFilter`, `CSpecGameDlg::GetJoinCaptionId` |
| `0x4654d0–0x465680` | 79 | `cd_win.cpp` (second half, displaced) | `CDAudio_WorkerThread`, `CDAudio_Shutdown`, `CDAudio_Init`, `CDAudio_QueueCommand` |
| `0x465680–0x465930` | — | `TitanRequest` ctors / dtor | `TitanRequest__TitanRequest`, `TitanRequest__scalar_dtor`, `TitanRequest__TitanRequest_addrport`, `TitanRequest__dtor` |
| `0x465930–0x466300` | 80 | `hlwon/cryptapi/TitanRequest.cpp` | `WON_Authenticate`, `WON_BuildRequest`, `WON_Exchange`, `TitanRequest__setAuth`, `TitanRequest__setAddrPort` |
| `0x466300–0x466700` | 81 | `Token.cpp` | `CToken::CToken`, `CToken::ParseNextToken`, `CToken::GetRemainder`, `CToken::ParseNextQuoteToken`, `CToken::GetData` |
| `0x466700–0x4671b0` | 82 | `strings.cpp` | `Launcher_FreeStrings`, `Launcher_BuildResourcePath`, `Launcher_ShowMessageByIdEx`, `Launcher_StringHeight`, `Launcher_FormatString` |
| `0x4671b0–0x4689a0` | 83 | `common.cpp` | `Sys_Error`, `COM_FindFile`, `COM_FixSlashes`, `LongSwap`, `COM_GetPlayerModelList` |
| `0x4689a0–0x468f70` | 84 | `sys_win.cpp` | `Sys_RegDeleteSubkeys`, `Sys_ResetSettings`, `Sys_StripCmdLineParm`, `Launcher_BinToHex`, `Sys_StripQuotesAndPercents` |
| `0x468f70–0x4692e0` | 85 | `scheme.cpp` | `Scheme_GetDefaultColor`, `Scheme_GetColor`, `Scheme_Free`, `Launcher_LoadScheme` |
| *`0x4692e0–0x46a220`* | | *vgui1 `Dar<T>` (§3), contains:* | `VGui_Start`, `VGui_Frame`, `LauncherApp::LauncherApp` |
| `0x4696a0–0x469720` | — | `vgui_menu.cpp` | `VGui_Start`, `VGui_Frame` |
| `0x46a220–0x46b760` | — | `VideoDlg.cpp` | `CVideoDlg::CVideoDlg`, `Vid_ComputeScreenRect`, `CVideoDlg::DrawGammaDib`, `CVideoDlg::RedrawGammaImage`, `CVideoDlg::DrawDialogOverlay` |
| `0x46b760–0x46d780` | — | `VideoModeDlg.cpp` | `Vid_SetRendererFlags`, `CVideoModeDlg::AddModeColumns`, `CVideoModeDlg::RebuildModeList`, `CVideoModeDlg::LoadGLDrivers`, `CVideoModeDlg::On3DInfoSite` |
| `0x46d780–0x46ddb0` | — | `VidSelectDlg.cpp` | `CVidSelectDlg::CVidSelectDlg`, `CVidSelectDlg::DoDataExchange`, `CVidSelectDlg::OnVideoModes`, `CVidSelectDlg::~CVidSelectDlg`, `CVidSelectDlg::OnInitDialog` |
| `0x46ddb0–0x46e490` | 2b | `volume.cpp` (displaced) | `CMixerControls::CMixerControls`, `CMixerControls::Close`, `CMixerControls::Release`, `CMixerControls::SetBoolControlValue`, `CreateMixerControls` |
| `0x46e490–0x46e850` | 2c | `wavelib.cpp` (displaced) | `WaveOpenFile`, `WaveStartDataRead`, `WaveReadFile`, `WaveLoadFile` |

---

## 4. `.rdata` — `0x4ac000–0x4cd000`

| range | region | what it is |
|---|---|---|
| `0x4ac000–0x4ac8cc` | **IAT** | §5.4 |
| `0x4ac8cc–~0x4b4884` | **App `.rdata`** | message maps, 61 vftables (`CServerDescription` `0x4aca50` … `WriteBuffer` `0x4b4334`), constants |
| `~0x4b4884–0x4b72b8` | **Library `.rdata`** | MFC strings and vftables, CRT/STL strings |
| `0x4b72b8–0x4bab70` | **RTTI** | `??_R4`/`??_R3`/`??_R2`/`??_R1`, 683 entries |
| `0x4bab70–0x4c44b8` | **C++ EH tables** | 549 `FuncInfo` blocks; throw info from `0x4c2a90` |
| `0x4c44b8–0x4c45f8` | **Import directory** | 15 `IMAGE_IMPORT_DESCRIPTOR`s + terminator |
| `0x4c45f8–0x4c7f70` | **Import lookup tables + names** | |
| `0x4c7f70–0x4cc526` | **Export directory** | 276 names — §5.5 |
| `0x4cc526–0x4cd000` | padding | |

---

## 5. Imports and exports

### 5.4 Import address table

| DLL | IAT range | imports |
|---|---|---|
| `ADVAPI32.dll` | `0x4ac000–0x4ac06c` | 27 |
| `COMCTL32.dll` | `0x4ac070–0x4ac078` | 2 |
| `DDRAW.dll` | `0x4ac07c–0x4ac084` | 2 |
| `GDI32.dll` | `0x4ac088–0x4ac154` | 51 |
| `KERNEL32.dll` | `0x4ac158–0x4ac3cc` | 157 |
| `SHELL32.dll` | `0x4ac3d0–0x4ac3d4` | 1 |
| `USER32.dll` | `0x4ac3d8–0x4ac614` | 143 |
| `WININET.dll` | `0x4ac618–0x4ac660` | 18 |
| `WINMM.dll` | `0x4ac664–0x4ac6d4` | 28 |
| `WINSPOOL.DRV` | `0x4ac6d8–0x4ac6e4` | 3 |
| `WONAuth.dll` | `0x4ac6e8–0x4ac730` | 18 |
| `WONCrypt.dll` | `0x4ac734–0x4ac770` | 15 |
| `WSOCK32.dll` | `0x4ac774–0x4ac7d0` | 23 |
| `comdlg32.dll` | `0x4ac7d4–0x4ac7d8` | 1 |
| `vgui.dll` | `0x4ac7dc–0x4ac8c8` | 59 |

MFC and CRT are static (§3).

### 5.5 Exports

276 exports, all vgui1 template code (`vgui::Dar<T>` for 18 types, `App` / `Cursor` members and vftables). Source: `VGUIAPI` (`__declspec(dllexport)`), no `.def`. ICF folds them to 32 addresses: 30 functions in `0x4692e0–0x46a1d7` and 2 vftables (`0x4b3ae0`, `0x4b3acc`).

---

## 6. `.data` — `0x4cd000–0x50d000`

| range | region | what it is |
|---|---|---|
| `0x4cd000–~0x4d2a00` | **App data** | per TU: RTTI type descriptors, strings, globals |
| `0x4d2c00–~0x4d5100` | **Static-lib data** | UnZip, WON auth strings |
| `~0x4d5880–0x4d9710` | **CRT/MFC data** | `__ctype` `0x4d6314`, locale strings |
| `0x4d9710–0x50cd08` | **BSS** | not in file. App globals first (`0x4d9710`), library statics last |
| `0x50cd08–0x50d000` | padding | |

---

## 7. `.rsrc` — `0x50d000–0x518498` (file `0xda000–0xe6000`)

| type | id | count |
|---|---|---|
| `RT_CURSOR` | 1 | 7 |
| `RT_BITMAP` | 2 | 6 |
| `RT_ICON` | 3 | 4 |
| `RT_MENU` | 4 | 2 |
| `RT_DIALOG` | 5 | 44 |
| `RT_STRING` | 6 | 27 tables |
| `RT_ACCELERATOR` | 9 | 2 |
| `RT_GROUP_CURSOR` | 12 | 4 |
| `RT_GROUP_ICON` | 14 | 2 |
| `RT_VERSION` | 16 | 1 |
| `RT_TOOLBAR` | 241 | 1 |

Skin resources load from `HL_Res.dll`; menu art from `gfx/shell/`.
