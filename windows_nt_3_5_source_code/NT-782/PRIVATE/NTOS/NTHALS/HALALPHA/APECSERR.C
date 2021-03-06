/*++

Copyright (c) 1994  Digital Equipment Corporation

Module Name:

    apecserr.c

Abstract:

    This module implements error handling (machine checks and error
    interrupts) for machines based on the APECS chip-set.

Author:

    Joe Notarangelo 14-Feb-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "apecs.h"

VOID
HalpSetMachineCheckEnables( 
    IN BOOLEAN DisableMachineChecks,
    IN BOOLEAN DisableProcessorCorrectables,
    IN BOOLEAN DisableSystemCorrectables
    );

VOID
HalpApecsReportFatalError(
    VOID
    );

ULONG
HalpSimm(
    ULONGLONG Address
    );

// jnfix - temp count
ULONG CorrectedMemoryReads = 0;
#define MAX_ERROR_STRING 128

extern ULONG HalDisablePCIParityChecking;


VOID
HalpInitializeMachineChecks(
    IN BOOLEAN ReportCorrectableErrors
    )
/*++

Routine Description:

    This routine initializes machine check handling for an APECS-based
    system by clearing all pending errors in the COMANCHE and EPIC and
    enabling correctable errors according to the callers specification.

Arguments:

    ReportCorrectableErrors - Supplies a boolean value which specifies
                              if correctable error reporting should be
                              enabled.

Return Value:

    None.

--*/
{
    ULONG Dummy;
    EPIC_ECSR Ecsr;
    COMANCHE_EDSR Edsr;

    //
    // Clear the lost error bit in the Comanche EDSR.
    //

    Edsr.all = 0;
    Edsr.Losterr = 1;
    WRITE_COMANCHE_REGISTER( 
        &((PCOMANCHE_CSRS)(APECS_COMANCHE_BASE_QVA))->ErrorAndDiagnosticStatusRegister,
        Edsr.all );

    //
    // Unlock the other error bits of the EDSR by reading the error address
    // registers.
    //

    Dummy = READ_COMANCHE_REGISTER(
        &((PCOMANCHE_CSRS)(APECS_COMANCHE_BASE_QVA))->ErrorHighAddressRegister 
        ); 

    Dummy = READ_COMANCHE_REGISTER(
        &((PCOMANCHE_CSRS)(APECS_COMANCHE_BASE_QVA))->ErrorLowAddressRegister 
        ); 

    //
    // Clear all of the error bits in the Epic ECSR.  Set correctable error
    // reporting as requested. 
    //

    Ecsr.all = READ_EPIC_REGISTER( 
        &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->EpicControlAndStatusRegister );

    //
    // For the common registers, simply set them - common in that the P1
    // definition matches the P2 definition (same structure offset).
    //

    Ecsr.Merr = 1;
    Ecsr.Iptl = 1;
    Ecsr.Umrd = 1;
    Ecsr.Cmrd = 1;
    Ecsr.Ndev = 1;
    Ecsr.Tabt = 1;
    Ecsr.Iope = 1;
    Ecsr.Ddpe = 1;
    Ecsr.Lost = 1;
    Ecsr.Iort = 1;

    if( ReportCorrectableErrors == TRUE ){
        Ecsr.Dcei = 0;
    } else {
        Ecsr.Dcei = 1;
    }

    if (HalDisablePCIParityChecking == 0xffffffff) {
        Ecsr.Dpec = 1;
    } else {
        Ecsr.Dpec = HalDisablePCIParityChecking;
    }
#if HALDBG
    if (HalDisablePCIParityChecking == 0) {
        DbgPrint("apecserr: PCI Parity Checking ON\n");
    } else if (HalDisablePCIParityChecking == 1) {
        DbgPrint("apecserr: PCI Parity Checking OFF\n");
    } else {
        DbgPrint("apecserr: PCI Parity Checking OFF - not set by ARC yet\n");
    }
#endif

    WRITE_EPIC_REGISTER( 
        &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->EpicControlAndStatusRegister,
        Ecsr.all );

    //
    // Set the machine check enables with the EV4.
    //

    if( ReportCorrectableErrors == TRUE ){
        HalpSetMachineCheckEnables( FALSE, FALSE, FALSE );
    } else {
        HalpSetMachineCheckEnables( FALSE, TRUE, TRUE );
    }

    return;

}

BOOLEAN
HalpPlatformMachineCheck(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    This routine is given control when an hard error is acknowledged
    by the APECS chipset.  The routine is given the chance to
    correct and dismiss the error.

Arguments:

    ExceptionRecord - Supplies a pointer to the exception record generated
                      at the point of the exception.

    ExceptionFrame - Supplies a pointer to the exception frame generated
                     at the point of the exception.

    TrapFrame - Supplies a pointer to the trap frame generated
                at the point of the exception.

Return Value:

    TRUE is returned if the machine check has been handled and dismissed -
    indicating that execution can continue.  FALSE is return otherwise.

--*/
{
    COMANCHE_EDSR Edsr;
    EPIC_ECSR Ecsr;

    //
    // Check if there are any memory errors pending in the Comanche.
    //
    // A lost error, tag parity, tag control or non-existent memory
    // error indicates a non-dismissable error.
    //

    Edsr.all = READ_COMANCHE_REGISTER( 
                   &((PCOMANCHE_CSRS)(APECS_COMANCHE_BASE_QVA))->ErrorAndDiagnosticStatusRegister );

    if( (Edsr.Losterr == 1) ||
        (Edsr.Bctaperr == 1) ||
        (Edsr.Bctcperr == 1) ||
        (Edsr.Nxmerr == 1) ){

        goto FatalError;

    }

    //
    // Check if there are any non-recoverable I/O errors.
    //
    // Memory errors, invalid page table lookups, uncorrectable
    // memory errors, target aborts, io parity errors, dma parity
    // errors, lost errors, and retry timeouts are all considered
    // non-dismissable errors.
    //

    Ecsr.all = READ_EPIC_REGISTER( 
        &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->EpicControlAndStatusRegister );

    if( (Ecsr.Merr == 1) ||
        (Ecsr.Iptl == 1) ||
        (Ecsr.Umrd == 1) ||
        (Ecsr.Tabt == 1) ||
        (Ecsr.Iope == 1) ||
        (Ecsr.Ddpe == 1) ||
        (Ecsr.Iort == 1) ){

        goto FatalError;

    }

    //
    // A Pass1 APECS chip bug will erroneously report a lost operation,
    // so, if a Pass1 chip, then ignore the error.
    //

    if ( (Ecsr.Lost == 1) && (Ecsr.Pass2 == 1)) {
        goto FatalError;
    }

    //
    // Check for a PCI configuration read error.  An error is a 
    // candidate if the I/O controller has signalled a NoDevice error.
    //

    if( Ecsr.Ndev == 1 ){

        //
        // So far, the error looks like a PCI configuration space read
        // that accessed a device that does not exist.  In order to fix
        // this up we expect that the faulting instruction or the instruction 
        // previous to the faulting instruction must be a load with v0 as
        // the destination register.  If this condition is met then simply
        // update v0 in the register set and return.  However, be careful
        // not to re-execute the load.
        //
        // jnfix - add condition to check if Rb contains the superpage
        //         address for config space?

        ALPHA_INSTRUCTION FaultingInstruction;
        BOOLEAN PreviousInstruction = FALSE;
        BOOLEAN WasLost = (Ecsr.Lost == 1 ? TRUE : FALSE);

        FaultingInstruction.Long = *(PULONG)((ULONG)TrapFrame->Fir); 
        if( (FaultingInstruction.Memory.Ra != V0_REG) ||
            (FaultingInstruction.Memory.Opcode != LDL_OP) ){

            //
            // Faulting instruction did not match, try the previous
            // instruction.
            //

            PreviousInstruction = TRUE;

            FaultingInstruction.Long = *(PULONG)((ULONG)TrapFrame->Fir - 4); 
            if( (FaultingInstruction.Memory.Ra != V0_REG) ||
                (FaultingInstruction.Memory.Opcode != LDL_OP) ){

                //
                // No match, we can't fix this up.
                //

                goto FatalError;
            }
        }

        //
        // The error has matched all of our conditions.  Fix it up by
        // writing the value 0xffffffff into the destination of the load.
        // 

        TrapFrame->IntV0 = (ULONGLONG)0xffffffffffffffff;

        //
        // If the faulting instruction was the load the restart execution
        // at the instruction after the load.
        //

        if( PreviousInstruction == FALSE ){
            TrapFrame->Fir += 4;
        }

        //
        // Clear the error condition in the ECSR.
        //

        Ecsr.all = READ_EPIC_REGISTER( 
            &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->EpicControlAndStatusRegister);

        Ecsr.Ndev = 1;
        Ecsr.Dcei = 1;

        if (WasLost) {
            Ecsr.Lost = 1;
        }

        WRITE_EPIC_REGISTER( 
            &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->EpicControlAndStatusRegister,
            Ecsr.all );

        return TRUE;

    } //endif Ecsr.Ndev == 1

//
// The system is not well and cannot continue reliable execution.
// Print some useful messages and return FALSE to indicate that the error
// was not handled.
//

FatalError:

    HalpApecsReportFatalError();

    return FALSE;

}


VOID
HalpApecsErrorInterrupt(
    VOID
    )
/*++

Routine Description:

    This routine is entered as a result of an error interrupt on the
    APECS chipset.  This function determines if the error is fatal
    or recoverable and if recoverable performs the recovery and
    error logging.

Arguments:

    None.

Return Value:

    None.

--*/
{
    COMANCHE_EDSR Edsr;
    EPIC_ECSR Ecsr;

    Edsr.all = READ_COMANCHE_REGISTER( 
                   &((PCOMANCHE_CSRS)(APECS_COMANCHE_BASE_QVA))->ErrorAndDiagnosticStatusRegister );

    Ecsr.all = READ_EPIC_REGISTER( 
        &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->EpicControlAndStatusRegister );

    //
    // Any error from the COMANCHE represents a fatal condition.
    //

    if( (Edsr.Losterr == 1) ||
        (Edsr.Bctaperr == 1) ||
        (Edsr.Bctcperr == 1) ||
        (Edsr.Nxmerr == 1) ){

        goto FatalErrorInterrupt;

    }

    //
    // Almost any error from the EPIC is also fatal.  The only exception
    // is the correctable memory read error.  Any other error is fatal.
    //

    if( (Ecsr.Merr == 1) ||
        (Ecsr.Iptl == 1) ||
        (Ecsr.Umrd == 1) ||
        (Ecsr.Tabt == 1) ||
        (Ecsr.Ndev == 1) ||
        (Ecsr.Iope == 1) ||
        (Ecsr.Ddpe == 1) ||
        (Ecsr.Iort == 1) ){

        goto FatalErrorInterrupt;

    }

    //
    // A Pass1 APECS chip bug will erroneously report a lost operation,
    // so, if a Pass1 chip, then ignore the error.
    //

    if ( (Ecsr.Lost == 1) && (Ecsr.Pass2 == 1)) {
        goto FatalErrorInterrupt;
    }

    //
    // The error may be correctable.  If the correctable error bit is
    // set in the ECSR then log the error and clear the condition.
    //

    if( Ecsr.Cmrd == 1 ){

        ULONG Sear;
        ULONGLONG SystemErrorAddress;

        CorrectedMemoryReads += 1;

        Sear = READ_EPIC_REGISTER(
            &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->SysbusErrorAddressRegister );

        SystemErrorAddress = (ULONGLONG)(Sear) << 2;

	//jnfix - temporary debug logging only
#if (DBG) || (HALDBG)

        if( (CorrectedMemoryReads % 32) == 0 ){
            DbgPrint( "APECS: CorrectedMemoryReads = %d, Address = 0x%Lx\n",
                      CorrectedMemoryReads,
                      SystemErrorAddress );
        }

#endif //DBG || HALDBG

        //
        // Clear the error condition.
        //

        WRITE_EPIC_REGISTER(
            &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->EpicControlAndStatusRegister,
            Ecsr.all
            );

        return;

    } //endif Ecsr.Cmrd == 1

//
// The interrupt indicates a fatal system error.
// Display information about the error and shutdown the machine.
//

FatalErrorInterrupt:

    HalpApecsReportFatalError();

//jnfix - add some of the address registers?
    KeBugCheckEx( DATA_BUS_ERROR,
                  Edsr.all,
                  Ecsr.all,
                  0,
                  0 );

                     
}


VOID
HalpApecsReportFatalError(
    VOID
    )
/*++

Routine Description:

   This function reports and interprets a fatal hardware error on
   the APECS chipset.  The COMANCHE Error and Diagnostic Status Register
   and the EPIC Error and Status Register are read to determine how
   to interpret the error.

Arguments:

   None.

Return Value:

   None.

--*/
{
    COMANCHE_EDSR Edsr;
    EPIC_ECSR Ecsr;
    ULONGLONG ErrorAddress;
    ULONG ErrorHighAddress;
    ULONG ErrorLowAddress;
    ULONG Pear;
    ULONG Sear;
    ULONGLONG SystemErrorAddress;
    UCHAR OutBuffer[MAX_ERROR_STRING];

    //
    // Begin the error output by acquiring ownership of the display
    // and printing the dreaded banner.
    //

    HalAcquireDisplayOwnership(NULL);

    HalDisplayString( "\nFatal system hardware error.\n\n" );

    //
    // Read both of the error registers.  It is possible that more
    // than one error was reported simulataneously.
    //

    Edsr.all = READ_COMANCHE_REGISTER( 
                   &((PCOMANCHE_CSRS)(APECS_COMANCHE_BASE_QVA))->ErrorAndDiagnosticStatusRegister );

    Ecsr.all = READ_EPIC_REGISTER( 
        &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->EpicControlAndStatusRegister );

    //
    // Read all of the relevant error address registers.
    //

    ErrorLowAddress = READ_COMANCHE_REGISTER(
        &((PCOMANCHE_CSRS)(APECS_COMANCHE_BASE_QVA))->ErrorLowAddressRegister );

    ErrorHighAddress = READ_COMANCHE_REGISTER(
        &((PCOMANCHE_CSRS)(APECS_COMANCHE_BASE_QVA))->ErrorHighAddressRegister);

    ErrorAddress = ((ULONGLONG)(ErrorHighAddress) << 21) +
                   ((ULONGLONG)(ErrorLowAddress) << 5);

    Sear = READ_EPIC_REGISTER(
        &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->SysbusErrorAddressRegister );

    SystemErrorAddress = (ULONGLONG)(Sear) << 2;

    Pear = READ_EPIC_REGISTER(
        &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->PciErrorAddressRegister );

    //
    // Interpret any errors from the COMANCHE EDSR.
    //

    sprintf( OutBuffer, "Comanche EDSR = 0x%x\n", Edsr.all );
    HalDisplayString( OutBuffer );

    if( Edsr.Bctaperr == 1 ){

        sprintf( OutBuffer,
                 "Bcache Tag Address Parity Error, Address: %Lx, SIMM = %d\n",
                 ErrorAddress, HalpSimm(ErrorAddress) );

        HalDisplayString( OutBuffer );

    }

    if( Edsr.Bctcperr == 1 ){

        sprintf( OutBuffer,
                 "Bcache Tag Control Parity Error, Address: %Lx, SIMM = %d\n",
                 ErrorAddress, HalpSimm(ErrorAddress) );

        HalDisplayString( OutBuffer );

    }

    if( Edsr.Nxmerr == 1 ){

        sprintf( OutBuffer,
                 "Non-existent memory error, Address: %Lx\n",
                 ErrorAddress );

        HalDisplayString( OutBuffer );

    }

    if( (Edsr.Bctaperr == 1) || (Edsr.Bctcperr == 1) || 
        (Edsr.Nxmerr == 1) ){

        if( Edsr.Dmacause == 1 ){
            HalDisplayString( "Error caused on DMA.\n" );
        }

        if( Edsr.Viccause == 1 ){
            HalDisplayString( "Victim write caused error.\n" );
        }

    }

    if( Edsr.Losterr == 1 ){
        HalDisplayString( "Multiple Cache/Memory errors.\n" );
    }

    //
    // Interpret any errors from the EPIC Ecsr.
    //

    sprintf( OutBuffer, "EPIC ECSR = 0x%lx\n", Ecsr.all );
    HalDisplayString( OutBuffer );

    if( Ecsr.Iort == 1 ){

        sprintf( OutBuffer,
                 "PCI Retry timeout error, PCI Address: 0x%x\n",
                 Pear );
        HalDisplayString( OutBuffer ); 

    }

    if( Ecsr.Ddpe == 1 ){

        sprintf( OutBuffer,
                 "DMA Data Parity Error at PCI Address: 0x%x\n",
                 Pear );
        HalDisplayString( OutBuffer ); 

    }

    if( Ecsr.Iope == 1 ){

        sprintf( OutBuffer,
                 "I/O Data Parity Error at PCI Address: 0x%x\n",
                 Pear );
        HalDisplayString( OutBuffer ); 

    }

    if( Ecsr.Tabt == 1 ){

        sprintf( OutBuffer,
                 "Target Abort Error at PCI Address: 0x%x\n",
                 Pear );
        HalDisplayString( OutBuffer ); 

    }

    if( Ecsr.Ndev == 1 ){

        sprintf( OutBuffer,
                 "No Device Error at PCI Address: 0x%x\n",
                 Pear );
        HalDisplayString( OutBuffer ); 

    }

    if( Ecsr.Iptl == 1 ){

        sprintf( OutBuffer,
                 "Invalid Page Table Lookup at PCI Address: 0x%x\n",
                 Pear );
        HalDisplayString( OutBuffer ); 

    }

    if( Ecsr.Umrd == 1 ){

        sprintf( OutBuffer,
                 "Uncorrectable memory read error, System Address: 0x%Lx\n",
                 SystemErrorAddress );
        HalDisplayString( OutBuffer ); 

    }

                  
    if( Ecsr.Merr == 1 ){

        sprintf( OutBuffer,
                 "Memory error, System Address: 0x%Lx\n",
                 SystemErrorAddress );
        HalDisplayString( OutBuffer ); 

    }

    if( Ecsr.Lost == 1 ){
        HalDisplayString( "Multiple I/O errors detected.\n" );
    }

    return;

} 

ULONG
HalpSimm(
    ULONGLONG Address
    )
/*++

Routine Description:

    Return the number of the SIMM corresponding to the supplied physical
    address.

Arguments:

    Address - Supplies the physical address of a byte in memory.

Return Value:

    The number of the SIMM that contains the byte of physical memory is
    returned.  If the Address does not fit within a SIMM or the SIMM number
    cannot be determined 0xffffffff is returned.

--*/
{

//jnfix - see if this can be determined exclusively from memory registers
//jnfix - or is this platform-dependent

    return( 0xffffffff );

}
