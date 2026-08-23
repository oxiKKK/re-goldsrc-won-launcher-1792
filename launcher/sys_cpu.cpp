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
// Purpose: CPU feature and speed detection (CPUID, RDTSC, MMX).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

#define CPUID_FEATURE_TSC	(1 << 4)
#define CPUID_FEATURE_MMX	(1 << 23)

// No CPUID means no way to time the machine; the launcher assumes a Pentium 166.
#define SYS_DEFAULT_CPU_MHZ	166

/*
==================
Sys_CPUIDSupported (0x47B450)
==================
*/
static BOOL Sys_CPUIDSupported( void )
{
	BOOL	supported = FALSE;

	__asm
	{
		push	ebx
		pushfd
		pushfd
		pop		eax
		mov		ebx, eax
		xor		eax, 200000h		; flip EFLAGS.ID (bit 21) -- settable only if CPUID exists
		push	eax
		popfd
		pushfd
		pop		eax
		popfd
		cmp		ebx, eax
		pop		ebx
		jz		short done
		mov		supported, 1
	done:
	}

	return supported;
}

/*
==================
Sys_RDTSCSupported (0x47B480)
==================
*/
static int Sys_RDTSCSupported( void )
{
	int		supported = 0;

	if ( !Sys_CPUIDSupported() )
		return 0;

	__asm
	{
		mov		eax, 0
		cpuid
		cmp		eax, 1
		jl		short done
		mov		eax, 1
		cpuid
		bt		edx, 4
		jnb		short done
		mov		supported, 1
	done:
	}

	return supported;
}

/*
==================
Sys_ReadTSC (0x47B4C0)

The CPUID either side of the RDTSC serialises the pipeline, so the two reads
bracket exactly the gate between them.
==================
*/
static unsigned __int64 Sys_ReadTSC( void )
{
	unsigned __int64	tsc;

	__asm
	{
		push	ebx
		push	ecx
		xor		eax, eax
		cpuid
		rdtsc
		mov		dword ptr tsc, eax
		mov		dword ptr tsc + 4, edx
		xor		eax, eax
		cpuid
		pop		ecx
		pop		ebx
	}

	return tsc;
}

/*
==================
Sys_GetCPUSpeed (0x47B4F0)
==================
*/
int Sys_GetCPUSpeed( void )
{
	unsigned __int64	start;
	DWORD				stop;

	if ( !Sys_RDTSCSupported() )
		return SYS_DEFAULT_CPU_MHZ;

	Sys_ReadTSC();						// warm up / discard
	stop = timeGetTime() + 250;
	while ( timeGetTime() < stop )
		;
	Sys_ReadTSC();						// discard
	start = Sys_ReadTSC();
	stop = timeGetTime() + 1000;
	while ( timeGetTime() < stop )
		;

	// delta-TSC over the 1 s gate / 1e6 -> MHz; +500000 rounds to nearest MHz
	return (int)( ( Sys_ReadTSC() - start + 500000 ) / 1000000ui64 );
}

/*
==================
Sys_CheckMMXTechnology (0x47B570)

Both the feature query and the EMMS are guarded: a CPU without CPUID faults on
the first, and a Win32s-era kernel that does not save MMX state faults on the
second.
==================
*/
int Sys_CheckMMXTechnology( void )
{
	int		features;
	int		ok = 1;

	__try
	{
		__asm
		{
			mov		eax, 1
			cpuid
			mov		features, edx
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		ok = 0;
	}

	if ( !ok )
		return 0;

	if ( ( features & CPUID_FEATURE_MMX ) == 0 )
		return 0;

	__try
	{
		__asm	emms
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		ok = 0;
	}

	return ok;
}
