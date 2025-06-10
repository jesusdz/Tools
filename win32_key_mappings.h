// Window virtual key codes taken from: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
internal Key Win32KeyMappings[] = {
KEY_NULL, // -						0x00	Undefined
KEY_NULL, // VK_LBUTTON				0x01	Left mouse button
KEY_NULL, // VK_RBUTTON				0x02	Right mouse button
KEY_NULL, // VK_CANCEL				0x03	Control-break processing
KEY_NULL, // VK_MBUTTON				0x04	Middle mouse button (three-button mouse)
KEY_NULL, // VK_XBUTTON1			0x05	X1 mouse button
KEY_NULL, // VK_XBUTTON2			0x06	X2 mouse button
KEY_NULL, // -						0x07	Undefined
KEY_BACKSPACE, // VK_BACK			0x08	BACKSPACE key
KEY_TAB, // VK_TAB					0x09	TAB key
KEY_NULL, // -						0x0A
KEY_NULL, // -						0x0B	Reserved
KEY_NULL, // VK_CLEAR				0x0C	CLEAR key
KEY_RETURN, // VK_RETURN			0x0D	ENTER key
KEY_NULL, // -						0x0E
KEY_NULL, // -						0x0F	Undefined
KEY_SHIFT, // VK_SHIFT				0x10	SHIFT key
KEY_CONTROL, // VK_CONTROL			0x11	CTRL key
KEY_ALT, // VK_MENU					0x12	ALT key
KEY_NULL, // VK_PAUSE				0x13	PAUSE key
KEY_NULL, // VK_CAPITAL				0x14	CAPSLOCK key
KEY_NULL, // VK_KANA				0x15	IME Kana,Hanguel,Hangul modes
KEY_NULL, // VK_IME_ON				0x16	IME On
KEY_NULL, // VK_JUNJA				0x17	IME Junja mode
KEY_NULL, // VK_FINAL				0x18	IME final mode
KEY_NULL, // VK_HANJA				0x19	IME Hanja,Kanji modes
KEY_NULL, // VK_IME_OFF				0x1A	IME Off
KEY_ESCAPE, // VK_ESCAPE			0x1B	ESC key
KEY_NULL, // VK_CONVERT				0x1C	IME convert
KEY_NULL, // VK_NONCONVERT			0x1D	IME non convert
KEY_NULL, // VK_ACCEPT				0x1E	IME accept
KEY_NULL, // VK_MODECHANGE			0x1F	IME mode change request
KEY_SPACE, // VK_SPACE				0x20	SPACEBAR
KEY_NULL, // VK_PRIOR				0x21	PAGEUP key
KEY_NULL, // VK_NEXT				0x22	PAGEDOWN key
KEY_NULL, // VK_END					0x23	END key
KEY_NULL, // VK_HOME				0x24	HOME key
KEY_LEFT, // VK_LEFT				0x25	LEFTARROW key
KEY_UP,   // VK_UP					0x26	UPARROW key
KEY_RIGHT,// VK_RIGHT				0x27	RIGHTARROW key
KEY_DOWN, // VK_DOWN				0x28	DOWNARROW key
KEY_NULL, // VK_SELECT				0x29	SELECT key
KEY_NULL, // VK_PRINT				0x2A	PRINT key
KEY_NULL, // VK_EXECUTE				0x2B	EXECUTE key
KEY_NULL, // VK_SNAPSHOT			0x2C	PRINTSCREEN key
KEY_NULL, // VK_INSERT				0x2D	INS key
KEY_DELETE, // VK_DELETE			0x2E	DEL key
KEY_NULL, // VK_HELP				0x2F	HELP key
KEY_0, // 							0x30	0 key
KEY_1, // 							0x31	1 key
KEY_2, // 							0x32	2 key
KEY_3, // 							0x33	3 key
KEY_4, // 							0x34	4 key
KEY_5, // 							0x35	5 key
KEY_6, // 							0x36	6 key
KEY_7, // 							0x37	7 key
KEY_8, // 							0x38	8 key
KEY_9, // 							0x39	9 key
KEY_NULL, // -						0x3A	Undefined
KEY_NULL, // -						0x3B	Undefined
KEY_NULL, // -						0x3C	Undefined
KEY_NULL, // -						0x3D	Undefined
KEY_NULL, // -						0x3E	Undefined
KEY_NULL, // -						0x3F	Undefined
KEY_NULL, // -						0x40	Undefined
KEY_A, // 							0x41	A key
KEY_B, // 							0x42	B key
KEY_C, // 							0x43	C key
KEY_D, // 							0x44	D key
KEY_E, // 							0x45	E key
KEY_F, // 							0x46	F key
KEY_G, // 							0x47	G key
KEY_H, // 							0x48	H key
KEY_Y, // 							0x49	I key
KEY_J, // 							0x4A	J key
KEY_K, // 							0x4B	K key
KEY_L, // 							0x4C	L key
KEY_M, // 							0x4D	M key
KEY_N, // 							0x4E	N key
KEY_O, // 							0x4F	O key
KEY_P, // 							0x50	P key
KEY_Q, // 							0x51	Q key
KEY_R, // 							0x52	R key
KEY_S, // 							0x53	S key
KEY_T, // 							0x54	T key
KEY_U, // 							0x55	U key
KEY_V, // 							0x56	V key
KEY_W, // 							0x57	W key
KEY_X, // 							0x58	X key
KEY_Y, // 							0x59	Y key
KEY_Z, // 							0x5A	Z key
KEY_NULL, // VK_LWIN				0x5B	Left Windows key(Natural keyboard)
KEY_NULL, // VK_RWIN				0x5C	Right Windows key(Natural keyboard)
KEY_NULL, // VK_APPS				0x5D	Applications key(Natural keyboard)
KEY_NULL, // -						0x5E	Reserved
KEY_NULL, // VK_SLEEP				0x5F	Computer Sleep key
KEY_0, // VK_NUMPAD0				0x60	Numeric keypad0 key
KEY_1, // VK_NUMPAD1				0x61	Numeric keypad1 key
KEY_2, // VK_NUMPAD2				0x62	Numeric keypad2 key
KEY_3, // VK_NUMPAD3				0x63	Numeric keypad3 key
KEY_4, // VK_NUMPAD4				0x64	Numeric keypad4 key
KEY_5, // VK_NUMPAD5				0x65	Numeric keypad5 key
KEY_6, // VK_NUMPAD6				0x66	Numeric keypad6 key
KEY_7, // VK_NUMPAD7				0x67	Numeric keypad7 key
KEY_8, // VK_NUMPAD8				0x68	Numeric keypad8 key
KEY_9, // VK_NUMPAD9				0x69	Numeric keypad9 key
KEY_NULL, // VK_MULTIPLY			0x6A	Multiply key
KEY_NULL, // VK_ADD					0x6B	Add key
KEY_NULL, // VK_SEPARATOR			0x6C	Separator key
KEY_NULL, // VK_SUBTRACT			0x6D	Subtract key
KEY_NULL, // VK_DECIMAL				0x6E	Decimal key
KEY_NULL, // VK_DIVIDE				0x6F	Divide key
KEY_NULL, // VK_F1					0x70	F1 key
KEY_NULL, // VK_F2					0x71	F2 key
KEY_NULL, // VK_F3					0x72	F3 key
KEY_NULL, // VK_F4					0x73	F4 key
KEY_NULL, // VK_F5					0x74	F5 key
KEY_NULL, // VK_F6					0x75	F6 key
KEY_NULL, // VK_F7					0x76	F7 key
KEY_NULL, // VK_F8					0x77	F8 key
KEY_NULL, // VK_F9					0x78	F9 key
KEY_NULL, // VK_F10					0x79	F10 key
KEY_NULL, // VK_F11					0x7A	F11 key
KEY_NULL, // VK_F12					0x7B	F12 key
KEY_NULL, // VK_F13					0x7C	F13 key
KEY_NULL, // VK_F14					0x7D	F14 key
KEY_NULL, // VK_F15					0x7E	F15 key
KEY_NULL, // VK_F16					0x7F	F16 key
KEY_NULL, // VK_F17					0x80	F17 key
KEY_NULL, // VK_F18					0x81	F18 key
KEY_NULL, // VK_F19					0x82	F19 key
KEY_NULL, // VK_F20					0x83	F20 key
KEY_NULL, // VK_F21					0x84	F21 key
KEY_NULL, // VK_F22					0x85	F22 key
KEY_NULL, // VK_F23					0x86	F23 key
KEY_NULL, // VK_F24					0x87	F24 key
KEY_NULL, // -						0x88	Unassigned
KEY_NULL, // -						0x89	Unassigned
KEY_NULL, // -						0x8A	Unassigned
KEY_NULL, // -						0x8B	Unassigned
KEY_NULL, // -						0x8C	Unassigned
KEY_NULL, // -						0x8D	Unassigned
KEY_NULL, // -						0x8E	Unassigned
KEY_NULL, // -						0x8F	Unassigned
KEY_NULL, // VK_NUMLOCK				0x90	NUMLOCK key
KEY_NULL, // VK_SCROLL				0x91	SCROLLLOCK key
KEY_NULL, // -						0x92	OEM specific
KEY_NULL, // -						0x93	OEM specific
KEY_NULL, // -						0x94	OEM specific
KEY_NULL, // -						0x95	OEM specific
KEY_NULL, // -						0x96	OEM specific
KEY_NULL, // -						0x97	Unassigned
KEY_NULL, // -						0x98	Unassigned
KEY_NULL, // -						0x99	Unassigned
KEY_NULL, // -						0x9A	Unassigned
KEY_NULL, // -						0x9B	Unassigned
KEY_NULL, // -						0x9C	Unassigned
KEY_NULL, // -						0x9D	Unassigned
KEY_NULL, // -						0x9E	Unassigned
KEY_NULL, // -						0x9F	Unassigned
KEY_NULL, // VK_LSHIFT				0xA0	Left SHIFT key
KEY_NULL, // VK_RSHIFT				0xA1	Right SHIFT key
KEY_NULL, // VK_LCONTROL			0xA2	Left CONTROL key
KEY_NULL, // VK_RCONTROL			0xA3	Right CONTROL key
KEY_NULL, // VK_LMENU				0xA4	Left ALT key
KEY_NULL, // VK_RMENU				0xA5	Right ALT key
KEY_NULL, // VK_BROWSER_BACK		0xA6	BrowserBack key
KEY_NULL, // VK_BROWSER_FORWARD		0xA7	BrowserForward key
KEY_NULL, // VK_BROWSER_REFRESH		0xA8	BrowserRefresh key
KEY_NULL, // VK_BROWSER_STOP		0xA9	BrowserStop key
KEY_NULL, // VK_BROWSER_SEARCH		0xAA	BrowserSearch key
KEY_NULL, // VK_BROWSER_FAVORITES	0xAB	BrowserFavorites key
KEY_NULL, // VK_BROWSER_HOME		0xAC	BrowserStart and Home key
KEY_NULL, // VK_VOLUME_MUTE			0xAD	VolumeMute key
KEY_NULL, // VK_VOLUME_DOWN			0xAE	VolumeDown key
KEY_NULL, // VK_VOLUME_UP			0xAF	VolumeUp key
KEY_NULL, // VK_MEDIA_NEXT_TRACK	0xB0	NextTrack key
KEY_NULL, // VK_MEDIA_PREV_TRACK	0xB1	PreviousTrack key
KEY_NULL, // VK_MEDIA_STOP			0xB2	StopMedia key
KEY_NULL, // VK_MEDIA_PLAY_PAUSE	0xB3	Play/PauseMedia key
KEY_NULL, // VK_LAUNCH_MAIL			0xB4	StartMail key
KEY_NULL, // VK_LAUNCH_MEDIA_SELECT	0xB5	SelectMedia key
KEY_NULL, // VK_LAUNCH_APP1			0xB6	StartApplication1 key
KEY_NULL, // VK_LAUNCH_APP2			0xB7	StartApplication2 key
KEY_NULL, // -						0xB8	Reserved
KEY_NULL, // -						0xB9	Reserved
KEY_NULL, // VK_OEM_1				0xBA	Miscellaneous
KEY_NULL, // VK_OEM_PLUS			0xBB	For any country/region, the '+' key
KEY_NULL, // VK_OEM_COMMA			0xBC	For any country/region, the ',' key
KEY_NULL, // VK_OEM_MINUS			0xBD	For any country/region, the '-' key
KEY_NULL, // VK_OEM_PERIOD			0xBE	For any country/region, the '.' key
KEY_NULL, // VK_OEM_2				0xBF	Miscellaneous
KEY_NULL, // VK_OEM_3				0xC0	Miscellaneous
KEY_NULL, // -						0xC1	Reserved
KEY_NULL, // -						0xC2	Reserved
KEY_NULL, // -						0xC3	Reserved
KEY_NULL, // -						0xC4	Reserved
KEY_NULL, // -						0xC5	Reserved
KEY_NULL, // -						0xC6	Reserved
KEY_NULL, // -						0xC7	Reserved
KEY_NULL, // -						0xC8	Reserved
KEY_NULL, // -						0xC9	Reserved
KEY_NULL, // -						0xCA	Reserved
KEY_NULL, // -						0xCB	Reserved
KEY_NULL, // -						0xCC	Reserved
KEY_NULL, // -						0xCD	Reserved
KEY_NULL, // -						0xCE	Reserved
KEY_NULL, // -						0xCF	Reserved
KEY_NULL, // -						0xD0	Reserved
KEY_NULL, // -						0xD1	Reserved
KEY_NULL, // -						0xD2	Reserved
KEY_NULL, // -						0xD3	Reserved
KEY_NULL, // -						0xD4	Reserved
KEY_NULL, // -						0xD5	Reserved
KEY_NULL, // -						0xD6	Reserved
KEY_NULL, // -						0xD7	Reserved
KEY_NULL, // -						0xD8	Unassigned
KEY_NULL, // -						0xD9	Unassigned
KEY_NULL, // -						0xDA	Unassigned
KEY_NULL, // VK_OEM_4				0xDB	Miscellaneous
KEY_NULL, // VK_OEM_5				0xDC	Miscellaneous
KEY_NULL, // VK_OEM_6				0xDD	Miscellaneous
KEY_NULL, // VK_OEM_7				0xDE	Miscellaneous
KEY_NULL, // VK_OEM_8				0xDF	Miscellaneous
KEY_NULL, // -						0xE0	Reserved
KEY_NULL, // 						0xE1	OEM specific
KEY_NULL, // VK_OEM_102				0xE2	The <> keys on the US standard keyboard, or the \\| key on the non-US102-key keyboard
KEY_NULL, // 						0xE3
KEY_NULL, // 						0xE4	OEM specific
KEY_NULL, // VK_PROCESSKEY			0xE5	IMEPROCESS key
KEY_NULL, // 						0xE6	OEM specific
KEY_NULL, // VK_PACKET				0xE7	Used to pass Unicode characters...
KEY_NULL, // -						0xE8	Unassigned
KEY_NULL, // 						0xE9	OEM specific
KEY_NULL, // 						0xEA	OEM specific
KEY_NULL, // 						0xEB	OEM specific
KEY_NULL, // 						0xEC	OEM specific
KEY_NULL, // 						0xED	OEM specific
KEY_NULL, // 						0xEE	OEM specific
KEY_NULL, // 						0xEF	OEM specific
KEY_NULL, // -						0xF0	OEM specific
KEY_NULL, // -						0xF1	OEM specific
KEY_NULL, // -						0xF2	OEM specific
KEY_NULL, // -						0xF3	OEM specific
KEY_NULL, // -						0xF4	OEM specific
KEY_NULL, // -						0xF5	OEM specific
KEY_NULL, // VK_ATTN				0xF6	Attn key
KEY_NULL, // VK_CRSEL				0xF7	CrSel key
KEY_NULL, // VK_EXSEL				0xF8	ExSel key
KEY_NULL, // VK_EREOF				0xF9	EraseEOF key
KEY_NULL, // VK_PLAY				0xFA	Play key
KEY_NULL, // VK_ZOOM				0xFB	Zoom key
KEY_NULL, // VK_NONAME				0xFC	Reserved
KEY_NULL, // VK_PA1					0xFD	PA1 key
KEY_NULL, // VK_OEM_CLEAR			0xFE	Clear key
KEY_NULL, // -						0xFF	Unassigned
};
