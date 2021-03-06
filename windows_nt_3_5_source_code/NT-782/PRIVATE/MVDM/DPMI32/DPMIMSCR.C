/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmimscr.c

Abstract:

    This module contains misc dpmi functions for risc.

Author:

    Dave Hart (davehart) creation-date 11-Apr-1993

Revision History:


--*/

#include "dpmi32p.h"

VOID
DpmiGetFastBopEntry(
    VOID
    )
/*++

Routine Description:

    This routine fails the get fast bop entry bop.  It is used by
    krnl286 on risc.

Arguments:

    None.

Return Value:

    None.

--*/
{
        //
        // krnl286 does a DPMIBOP GetFastBopAddress even on
        // risc, so just fail the call since fast-bopping
        // will only ever work on x86.
        //

        setBX(0);
        setDX(0);
        setES(0);
}

VOID
DpmiDpmiInUse(
    VOID
    )
/*++

Routine Description:

    This routine notifies the CPU that the NT dpmi server is in use,
    so that IRET hooks can be used in protected mode.  If the NT DPMI
    server is not used, no protected mode iret hooks can be used, because
    we don't have a protected mode address that points to the correct bop.
    Apps that run in protected mode without using DPMI will likely change
    the LDT and GDT in unpredictable ways.

Arguments:

    None.

Return Value:

    None.

--*/
{
    EnableEmulatorIretHooks();
}

VOID
DpmiDpmiNoLongerInUse(
    VOID
    )
/*++

Routine Description:

    This routine notifies the CPU that the NT dpmi server is no longer in use.
    This will cause the CPU to stop using PM iret hooks.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DisableEmulatorIretHooks();
}

VOID
DpmiIllegalRiscFunction(
    VOID
    )
/*++

Routine Description:

    This routine ignores any Dpmi bops that are not implemented on risc

Arguments:

    None.

Return Value:

    None.

--*/
{
   char szFormat[] = "NtVdm: Invalid DPMI BOP 0x%x from CS:IP %4.4x:%4.4x (%s mode), could be i386 dosx.exe.\n";
   char szMsg[sizeof(szFormat)+64];

   sprintf(
       szMsg,
       szFormat,
       Index,
       (int)getCS(),
       (int)getIP(),
       (getMSW() & MSW_PE) ? "prot" : "real"
       );

   OutputDebugString(szMsg);
}
