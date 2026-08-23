//DLL State Flags

#define DLL_INACTIVE 0		// no dll
#define DLL_ACTIVE   1		// dll is running
#define DLL_PAUSED   2		// dll is paused
#define DLL_CLOSE    3		// closing down dll
#define DLL_TRANS    4 		// Level Transition

// DLL Pause reasons

#define DLL_NORMAL        0   // User hit Esc or something.
#define DLL_QUIT          4   // Quit now

// DLL Substate info (for playing valve logo and supressing console/keyboard input.
#define ENG_NORMAL         (1<<0)
#define ENG_RESET          (1<<1)   // Force state to latch back to ENG_NORMAL
#define ENG_NOLOADCONSOLE  (1<<2)
#define ENG_NOINPUTCONTROL (1<<3)
#define ENG_ESCAPEEXITS    (1<<4)   // Hitting escape exits the level.  Valve.bsp