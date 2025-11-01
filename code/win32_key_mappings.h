// Window virtual key codes taken from: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
internal Key Win32KeyMappings[] = {
K_NULL, // -						0x00	Undefined
K_NULL, // VK_LBUTTON				0x01	Left mouse button
K_NULL, // VK_RBUTTON				0x02	Right mouse button
K_NULL, // VK_CANCEL				0x03	Control-break processing
K_NULL, // VK_MBUTTON				0x04	Middle mouse button (three-button mouse)
K_NULL, // VK_XBUTTON1			0x05	X1 mouse button
K_NULL, // VK_XBUTTON2			0x06	X2 mouse button
K_NULL, // -						0x07	Undefined
K_BACKSPACE, // VK_BACK			0x08	BACKSPACE key
K_TAB, // VK_TAB					0x09	TAB key
K_NULL, // -						0x0A
K_NULL, // -						0x0B	Reserved
K_NULL, // VK_CLEAR				0x0C	CLEAR key
K_RETURN, // VK_RETURN			0x0D	ENTER key
K_NULL, // -						0x0E
K_NULL, // -						0x0F	Undefined
K_SHIFT, // VK_SHIFT				0x10	SHIFT key
K_CONTROL, // VK_CONTROL			0x11	CTRL key
K_ALT, // VK_MENU					0x12	ALT key
K_NULL, // VK_PAUSE				0x13	PAUSE key
K_NULL, // VK_CAPITAL				0x14	CAPSLOCK key
K_NULL, // VK_KANA				0x15	IME Kana,Hanguel,Hangul modes
K_NULL, // VK_IME_ON				0x16	IME On
K_NULL, // VK_JUNJA				0x17	IME Junja mode
K_NULL, // VK_FINAL				0x18	IME final mode
K_NULL, // VK_HANJA				0x19	IME Hanja,Kanji modes
K_NULL, // VK_IME_OFF				0x1A	IME Off
K_ESCAPE, // VK_ESCAPE			0x1B	ESC key
K_NULL, // VK_CONVERT				0x1C	IME convert
K_NULL, // VK_NONCONVERT			0x1D	IME non convert
K_NULL, // VK_ACCEPT				0x1E	IME accept
K_NULL, // VK_MODECHANGE			0x1F	IME mode change request
K_SPACE, // VK_SPACE				0x20	SPACEBAR
K_NULL, // VK_PRIOR				0x21	PAGEUP key
K_NULL, // VK_NEXT				0x22	PAGEDOWN key
K_NULL, // VK_END					0x23	END key
K_NULL, // VK_HOME				0x24	HOME key
K_LEFT, // VK_LEFT				0x25	LEFTARROW key
K_UP,   // VK_UP					0x26	UPARROW key
K_RIGHT,// VK_RIGHT				0x27	RIGHTARROW key
K_DOWN, // VK_DOWN				0x28	DOWNARROW key
K_NULL, // VK_SELECT				0x29	SELECT key
K_NULL, // VK_PRINT				0x2A	PRINT key
K_NULL, // VK_EXECUTE				0x2B	EXECUTE key
K_NULL, // VK_SNAPSHOT			0x2C	PRINTSCREEN key
K_NULL, // VK_INSERT				0x2D	INS key
K_DELETE, // VK_DELETE			0x2E	DEL key
K_NULL, // VK_HELP				0x2F	HELP key
K_0, // 							0x30	0 key
K_1, // 							0x31	1 key
K_2, // 							0x32	2 key
K_3, // 							0x33	3 key
K_4, // 							0x34	4 key
K_5, // 							0x35	5 key
K_6, // 							0x36	6 key
K_7, // 							0x37	7 key
K_8, // 							0x38	8 key
K_9, // 							0x39	9 key
K_NULL, // -						0x3A	Undefined
K_NULL, // -						0x3B	Undefined
K_NULL, // -						0x3C	Undefined
K_NULL, // -						0x3D	Undefined
K_NULL, // -						0x3E	Undefined
K_NULL, // -						0x3F	Undefined
K_NULL, // -						0x40	Undefined
K_A, // 							0x41	A key
K_B, // 							0x42	B key
K_C, // 							0x43	C key
K_D, // 							0x44	D key
K_E, // 							0x45	E key
K_F, // 							0x46	F key
K_G, // 							0x47	G key
K_H, // 							0x48	H key
K_Y, // 							0x49	I key
K_J, // 							0x4A	J key
K_K, // 							0x4B	K key
K_L, // 							0x4C	L key
K_M, // 							0x4D	M key
K_N, // 							0x4E	N key
K_O, // 							0x4F	O key
K_P, // 							0x50	P key
K_Q, // 							0x51	Q key
K_R, // 							0x52	R key
K_S, // 							0x53	S key
K_T, // 							0x54	T key
K_U, // 							0x55	U key
K_V, // 							0x56	V key
K_W, // 							0x57	W key
K_X, // 							0x58	X key
K_Y, // 							0x59	Y key
K_Z, // 							0x5A	Z key
K_NULL, // VK_LWIN				0x5B	Left Windows key(Natural keyboard)
K_NULL, // VK_RWIN				0x5C	Right Windows key(Natural keyboard)
K_NULL, // VK_APPS				0x5D	Applications key(Natural keyboard)
K_NULL, // -						0x5E	Reserved
K_NULL, // VK_SLEEP				0x5F	Computer Sleep key
K_0, // VK_NUMPAD0				0x60	Numeric keypad0 key
K_1, // VK_NUMPAD1				0x61	Numeric keypad1 key
K_2, // VK_NUMPAD2				0x62	Numeric keypad2 key
K_3, // VK_NUMPAD3				0x63	Numeric keypad3 key
K_4, // VK_NUMPAD4				0x64	Numeric keypad4 key
K_5, // VK_NUMPAD5				0x65	Numeric keypad5 key
K_6, // VK_NUMPAD6				0x66	Numeric keypad6 key
K_7, // VK_NUMPAD7				0x67	Numeric keypad7 key
K_8, // VK_NUMPAD8				0x68	Numeric keypad8 key
K_9, // VK_NUMPAD9				0x69	Numeric keypad9 key
K_NULL, // VK_MULTIPLY			0x6A	Multiply key
K_NULL, // VK_ADD					0x6B	Add key
K_NULL, // VK_SEPARATOR			0x6C	Separator key
K_NULL, // VK_SUBTRACT			0x6D	Subtract key
K_NULL, // VK_DECIMAL				0x6E	Decimal key
K_NULL, // VK_DIVIDE				0x6F	Divide key
K_NULL, // VK_F1					0x70	F1 key
K_NULL, // VK_F2					0x71	F2 key
K_NULL, // VK_F3					0x72	F3 key
K_NULL, // VK_F4					0x73	F4 key
K_NULL, // VK_F5					0x74	F5 key
K_NULL, // VK_F6					0x75	F6 key
K_NULL, // VK_F7					0x76	F7 key
K_NULL, // VK_F8					0x77	F8 key
K_NULL, // VK_F9					0x78	F9 key
K_NULL, // VK_F10					0x79	F10 key
K_NULL, // VK_F11					0x7A	F11 key
K_NULL, // VK_F12					0x7B	F12 key
K_NULL, // VK_F13					0x7C	F13 key
K_NULL, // VK_F14					0x7D	F14 key
K_NULL, // VK_F15					0x7E	F15 key
K_NULL, // VK_F16					0x7F	F16 key
K_NULL, // VK_F17					0x80	F17 key
K_NULL, // VK_F18					0x81	F18 key
K_NULL, // VK_F19					0x82	F19 key
K_NULL, // VK_F20					0x83	F20 key
K_NULL, // VK_F21					0x84	F21 key
K_NULL, // VK_F22					0x85	F22 key
K_NULL, // VK_F23					0x86	F23 key
K_NULL, // VK_F24					0x87	F24 key
K_NULL, // -						0x88	Unassigned
K_NULL, // -						0x89	Unassigned
K_NULL, // -						0x8A	Unassigned
K_NULL, // -						0x8B	Unassigned
K_NULL, // -						0x8C	Unassigned
K_NULL, // -						0x8D	Unassigned
K_NULL, // -						0x8E	Unassigned
K_NULL, // -						0x8F	Unassigned
K_NULL, // VK_NUMLOCK				0x90	NUMLOCK key
K_NULL, // VK_SCROLL				0x91	SCROLLLOCK key
K_NULL, // -						0x92	OEM specific
K_NULL, // -						0x93	OEM specific
K_NULL, // -						0x94	OEM specific
K_NULL, // -						0x95	OEM specific
K_NULL, // -						0x96	OEM specific
K_NULL, // -						0x97	Unassigned
K_NULL, // -						0x98	Unassigned
K_NULL, // -						0x99	Unassigned
K_NULL, // -						0x9A	Unassigned
K_NULL, // -						0x9B	Unassigned
K_NULL, // -						0x9C	Unassigned
K_NULL, // -						0x9D	Unassigned
K_NULL, // -						0x9E	Unassigned
K_NULL, // -						0x9F	Unassigned
K_NULL, // VK_LSHIFT				0xA0	Left SHIFT key
K_NULL, // VK_RSHIFT				0xA1	Right SHIFT key
K_NULL, // VK_LCONTROL			0xA2	Left CONTROL key
K_NULL, // VK_RCONTROL			0xA3	Right CONTROL key
K_NULL, // VK_LMENU				0xA4	Left ALT key
K_NULL, // VK_RMENU				0xA5	Right ALT key
K_NULL, // VK_BROWSER_BACK		0xA6	BrowserBack key
K_NULL, // VK_BROWSER_FORWARD		0xA7	BrowserForward key
K_NULL, // VK_BROWSER_REFRESH		0xA8	BrowserRefresh key
K_NULL, // VK_BROWSER_STOP		0xA9	BrowserStop key
K_NULL, // VK_BROWSER_SEARCH		0xAA	BrowserSearch key
K_NULL, // VK_BROWSER_FAVORITES	0xAB	BrowserFavorites key
K_NULL, // VK_BROWSER_HOME		0xAC	BrowserStart and Home key
K_NULL, // VK_VOLUME_MUTE			0xAD	VolumeMute key
K_NULL, // VK_VOLUME_DOWN			0xAE	VolumeDown key
K_NULL, // VK_VOLUME_UP			0xAF	VolumeUp key
K_NULL, // VK_MEDIA_NEXT_TRACK	0xB0	NextTrack key
K_NULL, // VK_MEDIA_PREV_TRACK	0xB1	PreviousTrack key
K_NULL, // VK_MEDIA_STOP			0xB2	StopMedia key
K_NULL, // VK_MEDIA_PLAY_PAUSE	0xB3	Play/PauseMedia key
K_NULL, // VK_LAUNCH_MAIL			0xB4	StartMail key
K_NULL, // VK_LAUNCH_MEDIA_SELECT	0xB5	SelectMedia key
K_NULL, // VK_LAUNCH_APP1			0xB6	StartApplication1 key
K_NULL, // VK_LAUNCH_APP2			0xB7	StartApplication2 key
K_NULL, // -						0xB8	Reserved
K_NULL, // -						0xB9	Reserved
K_NULL, // VK_OEM_1				0xBA	Miscellaneous
K_NULL, // VK_OEM_PLUS			0xBB	For any country/region, the '+' key
K_NULL, // VK_OEM_COMMA			0xBC	For any country/region, the ',' key
K_NULL, // VK_OEM_MINUS			0xBD	For any country/region, the '-' key
K_NULL, // VK_OEM_PERIOD			0xBE	For any country/region, the '.' key
K_NULL, // VK_OEM_2				0xBF	Miscellaneous
K_NULL, // VK_OEM_3				0xC0	Miscellaneous
K_NULL, // -						0xC1	Reserved
K_NULL, // -						0xC2	Reserved
K_NULL, // -						0xC3	Reserved
K_NULL, // -						0xC4	Reserved
K_NULL, // -						0xC5	Reserved
K_NULL, // -						0xC6	Reserved
K_NULL, // -						0xC7	Reserved
K_NULL, // -						0xC8	Reserved
K_NULL, // -						0xC9	Reserved
K_NULL, // -						0xCA	Reserved
K_NULL, // -						0xCB	Reserved
K_NULL, // -						0xCC	Reserved
K_NULL, // -						0xCD	Reserved
K_NULL, // -						0xCE	Reserved
K_NULL, // -						0xCF	Reserved
K_NULL, // -						0xD0	Reserved
K_NULL, // -						0xD1	Reserved
K_NULL, // -						0xD2	Reserved
K_NULL, // -						0xD3	Reserved
K_NULL, // -						0xD4	Reserved
K_NULL, // -						0xD5	Reserved
K_NULL, // -						0xD6	Reserved
K_NULL, // -						0xD7	Reserved
K_NULL, // -						0xD8	Unassigned
K_NULL, // -						0xD9	Unassigned
K_NULL, // -						0xDA	Unassigned
K_NULL, // VK_OEM_4				0xDB	Miscellaneous
K_NULL, // VK_OEM_5				0xDC	Miscellaneous
K_NULL, // VK_OEM_6				0xDD	Miscellaneous
K_NULL, // VK_OEM_7				0xDE	Miscellaneous
K_NULL, // VK_OEM_8				0xDF	Miscellaneous
K_NULL, // -						0xE0	Reserved
K_NULL, // 						0xE1	OEM specific
K_NULL, // VK_OEM_102				0xE2	The <> keys on the US standard keyboard, or the \\| key on the non-US102-key keyboard
K_NULL, // 						0xE3
K_NULL, // 						0xE4	OEM specific
K_NULL, // VK_PROCESSKEY			0xE5	IMEPROCESS key
K_NULL, // 						0xE6	OEM specific
K_NULL, // VK_PACKET				0xE7	Used to pass Unicode characters...
K_NULL, // -						0xE8	Unassigned
K_NULL, // 						0xE9	OEM specific
K_NULL, // 						0xEA	OEM specific
K_NULL, // 						0xEB	OEM specific
K_NULL, // 						0xEC	OEM specific
K_NULL, // 						0xED	OEM specific
K_NULL, // 						0xEE	OEM specific
K_NULL, // 						0xEF	OEM specific
K_NULL, // -						0xF0	OEM specific
K_NULL, // -						0xF1	OEM specific
K_NULL, // -						0xF2	OEM specific
K_NULL, // -						0xF3	OEM specific
K_NULL, // -						0xF4	OEM specific
K_NULL, // -						0xF5	OEM specific
K_NULL, // VK_ATTN				0xF6	Attn key
K_NULL, // VK_CRSEL				0xF7	CrSel key
K_NULL, // VK_EXSEL				0xF8	ExSel key
K_NULL, // VK_EREOF				0xF9	EraseEOF key
K_NULL, // VK_PLAY				0xFA	Play key
K_NULL, // VK_ZOOM				0xFB	Zoom key
K_NULL, // VK_NONAME				0xFC	Reserved
K_NULL, // VK_PA1					0xFD	PA1 key
K_NULL, // VK_OEM_CLEAR			0xFE	Clear key
K_NULL, // -						0xFF	Unassigned
};
