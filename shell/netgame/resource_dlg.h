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
// Purpose: declares the dialog template and control ids of hl.exe build 1792.
//
// Verified against all 44 launcher RT_DIALOG templates extracted from the retail
// PE.  Repeated *values* across different IDD_ templates are correct and expected:
// each MFC dialog template has its own control-id space, so e.g. 1218 names one
// control on template 233 and a different one on 237.  Templates 217, 219, 243 and
// 244 exist in the resources but nothing in the code constructs them.
//
// $NoKeywords: $
//=============================================================================

#ifndef RESOURCE_DLG_H
#define RESOURCE_DLG_H

// - IDD_PLAYERINFO (143) ---
#define IDD_PLAYERINFO                     143
#define IDC_PLAYERINFO_SERVER_NAME         1110
#define IDC_PLAYERINFO_SERVER_IP           1111
#define IDC_PLAYERINFO_SERVERNAME          1192
#define IDC_PLAYERINFO_SERVERIP            1194
#define IDC_PLAYERINFO_SERVERPING          1196
#define IDC_PLAYERINFO_SERVER_PING         1112
// the two report lists are not on the template; OnInitDialog creates them
#define IDC_PLAYERINFO_PLAYERLIST          110
#define IDC_PLAYERINFO_RULELIST            117

// - IDD_FINDPLAYER (147) ---
#define IDD_FINDPLAYER                     147
#define IDC_FINDPLAYER_PLAYER              1114
#define IDC_FINDPLAYER_SERVER              1115
#define IDC_FINDPLAYER_SERVEREDIT          1141
#define IDC_FINDPLAYER_PLAYEREDIT          1142
#define IDC_FINDPLAYER_TITLE               1116

// - IDD_ROOM (150) ---
#define IDD_ROOM                           150
// Not on template 150; RoomDialog routes it with ON_COMMAND.
#define IDC_ROOM_CREATEGAME                1002
#define IDC_ROOM_LIST                      1049
#define IDC_ROOM_CREATE_ROOM               1188
#define IDC_ROOM_PERMANENT                 1043
#define IDC_ROOM_USER                      1044

// - IDD_FILTER (152) ---
#define IDD_FILTER                         152
#define IDC_FILTER_RESPONDING              1008
#define IDC_FILTER_RESPONSETIME            1009
#define IDC_FILTER_NOTFULL                 1010
#define IDC_FILTER_NOTEMPTY                1011
#define IDC_FILTER_ONFAVORITES             1013
#define IDC_FILTER_HEADING                 1105
#define IDC_FILTER_BYGAME                  1014
#define IDC_FILTER_BYMAP                   1015
#define IDC_FILTER_BYOS                    1016
#define IDC_FILTER_BYDEDICATED             1017
#define IDC_FILTER_ISPROXY                 1019
#define IDC_FILTER_ISNOTPROXY              1020
// the two value combos are not on the template; OnInitDialog creates them
#define IDC_FILTER_PINGCOMBO               111
#define IDC_FILTER_GAMECOMBO               128

// - IDD_CREATEROOM (154) ---
#define IDD_CREATEROOM                     154
#define IDC_CREATEROOM_ROOMNAME            1117
#define IDC_CREATEROOM_ROOMPASSWORD        1118
// the two entry fields are not on the template; OnInitDialog creates them
#define IDC_CREATEROOM_NAMEEDIT            102
#define IDC_CREATEROOM_PASSWORDEDIT        103

// - IDD_MAIN (155) ---
#define IDD_MAIN                           155
#define IDC_MAIN_RETURN_TO_GAME            1019
#define IDC_MAIN_NEW_GAME                  1016
#define IDC_MAIN_QUICK_START               1070
#define IDC_MAIN_INTERNET_GAMES            1023
#define IDC_MAIN_LAN_GAMES                 1026
#define IDC_MAIN_CHAT_ROOMS                1027
#define IDC_MAIN_CONFIGURE_HALF_LIFE       1017
#define IDC_MAIN_LOAD_GAME                 1021
#define IDC_MAIN_LOAD_OR_SAVE_GAME         1022
#define IDC_MAIN_MULTIPLAYER               1015
#define IDC_MAIN_VIEW_README               1018
#define IDC_MAIN_ORDER_HALF_LIFE           1020
#define IDC_BTN_PREVIEWS                   1024
#define IDC_MAIN_CONSOLE                   1167
#define IDC_MAIN_FRIENDS                   1221
#define IDC_MAIN_MINIMIZE                  1173
#define IDC_MAIN_CLOSE                     1174
#define IDC_MAIN_RETURNHELP                1087
#define IDC_MAIN_NEWGAMEHELP               1081
#define IDC_MAIN_TRAININGHELP              1086
#define IDC_MAIN_LOADHELP                  1084
#define IDC_MAIN_LOADSAVEHELP              1085
#define IDC_MAIN_CONFIGUREHELP             1083
#define IDC_MAIN_READMEHELP                1123
#define IDC_MAIN_QUITHELP                  1124
#define IDC_MAIN_MULTIPLAYERHELP           1082
#define IDC_MAIN_ORDERHELP                 1125
#define IDC_MAIN_PREVIEWSHELP              1126
#define IDC_MAIN_CUSTOM_GAME               1025
#define IDC_MAIN_CUSTOMHELP                1088
#define IDC_MAIN_HAZARD_COURSE             1223
#define IDC_QUICKSTART                     1089
#define IDC_INTERNET_GAMES                 1090
#define IDC_LAN_GAMES                      1091
#define IDC_CHAT_ROOMS                     1092
#define IDC_MAIN_TFC_MANUAL                1029
#define IDC_MAIN_MANUALHELP                1093

// - IDD_DLG156 (156) ---
#define IDD_DLG156                         156	// CServerBrowserDlg (NetGame.h binds it)
#define IDC_BTN_LISTROOMS                  1108
#define IDC_BTN_FIND                       1106
#define IDC_BTN_CONNECT                    1208
#define IDC_BTN_CREATESV                   1048
#define IDC_BTN_INFO                       1049
#define IDC_BTN_REFRESH                    1112
#define IDC_BTN_UPDATE                     1114
#define IDC_BTN_FILTER                     1107
#define IDC_BTN_ADDSERVER                  1115
#define IDC_BTN_LISTMODE                   1175
#define IDC_DLG156_STATIC1105              1105	// blank static, no design-time text; unreferenced
#define IDC_NET_STATUS_LEFT              1176
#define IDC_NET_STATUS_RIGHT                       1177
#define IDC_DLG156_ROOM                    1190
#define IDC_DLG156_M                       1173
#define IDC_BTN_RESUME                     1209
#define IDC_BTN_DISCONNECT                 1210
#define IDC_DLG156_TOTAL                   1191
#define IDC_DLG156_TOTAL2                  1192

// - IDD_LOADGAME (159) ---
#define IDD_LOADGAME                       159
#define IDC_LOADGAME_LOAD_SAVED_GAME       1020
#define IDC_LOADGAME_DELETE_GAME           1021

// - IDD_CONFIGURE (160) ---
#define IDD_CONFIGURE                      160
#define IDC_CONFIGURE_CONTROLS             39
#define IDC_CONFIGURE_AUDIO                36
#define IDC_CONFIGURE_VIDEO                31
#define IDC_BTN_GORE                       30
#define IDC_BTN_AUTOPATCH                  41
#define IDC_CFG_CONTROLHELP                1149
#define IDC_CFG_VIDHELP                    1147
#define IDC_CFG_AUDIOHELP                  1148
#define IDC_CFG_RETURNTOMAIN               1150
#define IDC_CONFIGURE_GORE                 1151
#define IDC_CONFIGURE_AUTOPATCHHELP        1152
#define IDC_CONFIGURE_CUSTOMIZE            1198
#define IDC_MULTI_CUSTOMIZE                1199

// - IDD_VIDEO (161) ---
#define IDD_VIDEO                          161
#define IDC_VIDEO_GAMMAIMAGE               1056
#define IDC_VIDEO_GAMMA                    1099
#define IDC_VIDEO_GLARE                    1100
#define IDC_VIDEO_SCREENSIZE               1101
#define IDC_VIDEO_SCREENSIZESLIDER               1098
#define IDC_VIDEO_GAMMASLIDER               1103
#define IDC_VIDEO_GLARESLIDER               1102
#define IDC_VIDEO_GAMMAHELP                1168
#define IDC_VIDEO_GLAREHELP                1169
#define IDC_SKIP_SPRITE                    1043

// - IDD_AUDIO (162) ---
#define IDD_AUDIO                          162
#define IDC_AUDIO_VOLUME                   1090
#define IDC_AUDIO_SUITVOL                  1079
#define IDC_AUDIO_USECD                    1026
#define IDC_AUDIO_HIGHQUALITY              1025
#define IDC_AUDIO_A3D                      1027
#define IDC_AUDIO_CDHINT                   1212
#define IDC_AUDIO_EAX                      1028

// - IDD_KEYBOARD (163) ---
#define IDD_KEYBOARD                         163
#define IDC_KEYBOARD_USEDEFAULTS            21
#define IDC_KEYBOARD_ADVANCED        34
#define IDC_KEYBOARD_CANCEL                  25
// Not on template 163 -- it has only 21, 34, 1, 25 and 1149; these two
// are created in code by CKeyboardDlg::OnInitDialog.
#define IDC_KEYBOARD_KEYSEARCH               113
#define IDC_KEYBOARD_BINDLIST                1030
#define IDC_CONTROLS_KEYHELP               1149

// - IDD_CREATESERVER (166) ---
#define IDD_CREATESERVER                   166
#define IDC_CREATESERVER_ADVANCED_MULTIPLAYER 29
#define IDC_CREATESERVER_NAME              1110
#define IDC_CREATESERVER_MAXPLAYERS        1111
#define IDC_CREATESERVER_MAP               1112
#define IDC_NEWPROFILE_PASSWORD            1115
// the map list and its notify sibling are created in code
#define IDC_CREATESERVER_MAPLIST           1006
#define IDC_CREATESERVER_MAPNOTIFY         1042
#define IDC_CREATESERVER_DEDICATED         1043

// - IDD_MODDOWNLOAD (237) -- FTP mod-download progress dialog ---
// Shared by CModDownloadDlg and CModHttpDownloadDlg.  Note 1218 here is NOT the
// same control as IDC_MODREQ_STATUS, which is 1218 on template 233.
#define IDD_MODDOWNLOAD                    237
#define IDC_MODDOWNLOAD_TITLE              1217
#define IDC_MODDOWNLOAD_STATUS             1218
#define IDC_MODDOWNLOAD_TIME               1219

// - IDD_LOGIN (173) ---
#define IDD_LOGIN                         173
#define IDC_LOGIN_TITLE               1119
#define IDC_LOGIN_LINE_UPPER              1120
#define IDC_LOGIN_LINE_LOWER              1121

// - IDD_PROFILE (174) ---
#define IDD_PROFILE                        174
#define IDC_PROFILE_MODEL                  1077
#define IDC_PROFILE_COLOR                  1079
#define IDC_PROFILE_NICKNAME               1082
#define IDC_PROFILE_LOGO                   1183
#define IDC_PROFILE_LOGOCOLOR              1206
#define IDC_OPTS_HIMODELS                  1043
#define IDC_PROFILE_LEFT                   22
#define IDC_PROFILE_RIGHT                  30
#define IDC_PROFILE_LOGO_PREV                 35
#define IDC_PROFILE_LOGO_NEXT                38
// the name field and the colour picker are not on the template; OnInitDialog
// creates them
#define IDC_PROFILE_NAMEEDIT               1005
#define IDC_PROFILE_COLORCOMBO             135
#define IDC_BTN_SETINFO                    1220
#define IDC_AUDIO_MICVOL                   32789
#define IDC_AUDIO_MILES                    1224
#define IDC_AUDIO_SPEAKVOL                 32790
#define IDC_OPTS_VOCENABLE                 32791

// - IDD_OPTS (175) ---
#define IDD_OPTS                           175
#define IDC_OPTS_CROSSHAIR                 1064
#define IDC_OPTS_REVERSE                   1065
#define IDC_OPTS_MLOOK                     1061
#define IDC_OPTS_LOOKSPRING                1027
#define IDC_OPTS_LOOKSTRAFE                1028
#define IDC_OPTS_MFILTER                   1062
#define IDC_OPTS_JOYSTICK                  1035
#define IDC_OPTS_AUTOAIM                   1063
#define IDC_OPTS_CROSSHAIRHELP             1165
#define IDC_OPTS_REVERSEHELP               1166
#define IDC_OPTS_JOYSTICKHELP              1167
#define IDC_OPTS_MLOOKHELP                 1168
#define IDC_OPTS_LOOKSPRINGHELP            1169
#define IDC_OPTS_LOOKSTRAFEHELP            1170
#define IDC_OPTS_MFILTERHELP               1171
#define IDC_OPTS_SENSITIVITYHELP           1096
#define IDC_OPTS_AUTOAIMHELP               1172
#define IDC_OPTS_JLOOK                     1066
#define IDC_OPTS_JLOOKHELP                 1173
#define IDC_OPTS_CONSOLE                   1067
#define IDC_OPTS_CONSOLEHELP               1174

// - IDD_NEWPROFILE (177) ---
#define IDD_NEWPROFILE                     177
#define IDC_NEWPROFILE_OK                  1151
#define IDC_NEWPROFILE_REMEMBER            1155
#define IDC_NEWPROFILE_NAME                1136
#define IDC_NEWPROFILE_PASSWORD_2          1137
#define IDC_NEWPROFILE_PANEL          1135
#define IDC_NEWPROFILE_TITLE               1138

// - IDD_PROMPT (183) ---
#define IDD_PROMPT                         183
#define IDC_PROMPT_DONTASK              1043

// - IDD_README (190) ---
#define IDD_README                         190
#define IDC_README_STATIC                  1123

// - IDD_INPUT (200) ---
#define IDD_INPUT                          200
#define IDC_INPUT_STATIC                   1149

// - IDD_LOGO (202) ---
#define IDD_LOGO                           202

// - IDD_SAVE (203) ---
#define IDD_SAVE                           203
#define IDC_SAVE_SAVE_CURRENT_GAME         1019
#define IDC_SAVE_DELETE_GAME               1021

// - IDD_NEWGAME (204) ---
#define IDD_NEWGAME                        204
#define IDC_NEWGAME_EASY                   26
#define IDC_NEWGAME_MEDIUM                 32
#define IDC_NEWGAME_DIFFICULT              1157
#define IDC_NEWGAME_EASYHELP               1158
#define IDC_NEWGAME_MEDIUMHELP             1159
#define IDC_NEWGAME_DIFFICULTHELP          1160
#define IDC_NEWGAME_RETURNHELP             1161

// - IDD_LOADSAVE (205) ---
#define IDD_LOADSAVE                       205
#define IDC_LOADSAVE_LOAD_GAME             27
#define IDC_LOADSAVE_SAVE_GAME             33
#define IDC_LOADSAVE_LOADHELP              1162
#define IDC_LOADSAVE_SAVEHELP              1163
#define IDC_LOADSAVE_HINT                  1080
#define IDC_LOADSAVE_RETURN                1150

// - IDD_MODE_LIST (206) "Mode List" ---
#define IDD_MODE_LIST                      206
#define IDC_MODE_LIST_LISTBOX1207          1207

// - IDD_TEST_VIDEO_MODE (207) "Test video mode" ---
#define IDD_TEST_VIDEO_MODE                207

// - IDD_VIDMODE (208) ---
#define IDD_VIDMODE                        208
#define IDC_VIDMODE_CANCEL                 1170
#define IDC_VIDMODE_ADVANCED               1172
#define IDC_VIDMODE_WINDOWED               1043
#define IDC_VIDMODE_MOUSE                  1044
#define IDC_VIDMODE_HINT                   1173
#define IDC_VIDMODE_3D_INFO_SITE           1216

// - IDD_VIDSELECT (209) ---
#define IDD_VIDSELECT                      209
#define IDC_VIDSELECT_VIDEO_OPTIONS        1164
#define IDC_VIDSELECT_VIDEO_MODES          1165
#define IDC_VIDSELECT_OPTIONSHELP          1146
#define IDC_VIDSELECT_MODESHELP            1147
#define IDC_VIDSELECT_RETURNHELP           1150

// - IDD_DLG217 (217) ---
#define IDD_DLG217                         217	// skip/quickstart popup -- no code constructs it
#define IDC_DLG217_SKIP                    28
#define IDC_DLG217_BODY                  1177
#define IDC_DLG217_FOOTER                1178
#define IDC_QUICKSTART_TITLE               1179

// - IDD_REFRESH (218) ---
#define IDD_REFRESH                         218
#define IDC_REFRESH_TITLE  1180
#define IDC_REFRESH_BODY                  1181
#define IDC_REFRESH_PERCENT              1182
#define IDC_REFRESH_STATUS              1183

// - IDD_CHATLOGON (219) ---
#define IDD_CHATLOGON                         219
#define IDC_CHATLOGON_STATUS  1184

// - IDD_MULTISELECT (220) ---
#define IDD_MULTISELECT                    220
#define IDC_MULTI_RESUME                   136
#define IDC_MULTI_DISCONNECT               137
#define IDC_BTN_QUICK                      1185
#define IDC_BTN_BROWSE                     1187
#define IDC_BTN_CHAT                       1191
#define IDC_BTN_LAN                        1195
#define IDC_BTN_CUSTOMIZE                  1198
#define IDC_BTN_WON                        23
#define IDC_MAIN_QUICKHELP                 1080
#define IDC_MULTI_BROWSE                   1189
#define IDC_MULTI_CHAT                     1193
#define IDC_MULTI_LAN                      1197
#define IDC_MULTI_RESUMEHELP               1081
#define IDC_MULT_DISCONNECTHELP            1082
#define IDC_MULTI_DONEHELP                 1200
#define IDC_MULTI_WONHELP                  1201
#define IDC_BTN_CONTROLS                   1202
#define IDC_BTN_SPECTATE                   1190
#define IDC_MULTI_SPECTATE                 1192

// - IDD_LAN (223) ---
#define IDD_LAN                         223
#define IDC_LAN_JOINGAME        1200
#define IDC_LAN_STARTGAME       1205
#define IDC_LAN_INFO                     1204
#define IDC_LAN_REFRESH         1203
#define IDC_DLG223_IDC_BTN_DISCONNECT      137

// - IDD_ADVANCEDMP (231) ---
#define IDD_ADVANCEDMP                         231
#define IDC_ADVANCEDMP_PAGE                    1210

// - IDD_GORE (232) ---
#define IDD_GORE                           232
#define IDC_GORE_CHECKBOX                  30
#define IDC_GORE_HELP                      1209

// - IDD_MODREQ (233) ---
#define IDD_MODREQ                         233
#define IDC_MODREQ_STATUS                  1218
#define IDC_MODREQ_CANCEL                  1170

// - IDD_CUSTOMGAME (234) ---
#define IDD_CUSTOMGAME                     234
#define IDC_CUSTOMGAME_ACTIVATE            1213
#define IDC_CUSTOMGAME_IINSTALL            1214
#define IDC_CUSTOMGAME_REFRESH_LIST        1215
#define IDC_CUSTOMGAME_DEACTIVATE          1216
#define IDC_CUSTOMGAME_VIST_MOD_SITE       1218
// the mod list is not on the template; OnInitDialog creates it
#define IDC_CUSTOMGAME_MODLIST             129

// - IDD_SETINFO (239) ---
#define IDD_SETINFO                         239
#define IDC_SETINFO_PAGE                    1210
#define IDC_SETINFO_SENSHELP         32792

// - IDD_DLG243 (243) "Dialog" ---
#define IDD_DLG243                         243	// ClassWizard default template -- unreferenced

// - IDD_DLG244 (244) ---
#define IDD_DLG244                         244	// no controls at all -- unreferenced

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif


// Internet Games page (IDD_DLG156) -- controls created in OnCreate, plus the
// context-menu / accelerator commands its message map handles.
#define IDC_NET_SERVER_LIST                101
#define IDC_NET_DELETE_SELECTED            112
#define IDC_NET_SPEED_COMBO                125
#define IDC_NET_SORT_LIST                  127
#define IDC_NET_REFRESH_SELECTED           131
#define IDC_NET_CHAT_TEXT                  1004
#define IDC_NET_CHAT_INPUT                 1005
#define IDC_NET_USER_LIST                  1006
#define IDC_NET_CHAT_SEND                  1111
#define IDC_NET_USER_INSERT                1113
#define IDC_NET_REFRESH_ALL                32773
#define IDC_NET_REBUILD_LIST               32775
#define IDC_NET_FAVORITE_ON                32786
#define IDC_NET_FAVORITE_OFF               32787

#endif // RESOURCE_DLG_H
