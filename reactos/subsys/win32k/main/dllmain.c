/* $Id: dllmain.c,v 1.22 2002/01/13 22:52:08 dwelch Exp $
 * 
 *  Entry Point for win32k.sys
 */

#undef WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <ddk/service.h>

#include <win32k/win32k.h>

#include <include/winsta.h>
#include <include/class.h>
#include <include/window.h>

/*
 * NOTE: the table is actually in the file ./svctab.c,
 * generated by iface/addsys/mktab.c + w32ksvc.db
 */
#include "svctab.c"

/*
 * This definition doesn't work
 */
// WINBOOL STDCALL DllMain(VOID)
NTSTATUS
STDCALL
DllMain (
  IN	PDRIVER_OBJECT	DriverObject,
  IN	PUNICODE_STRING	RegistryPath)
{
  NTSTATUS Status;
  BOOLEAN Result;

  DbgPrint("Win32 kernel mode driver\n");

  /*
   * Register user mode call interface
   * (system service table index = 1)
   */
  Result = KeAddSystemServiceTable (Win32kSSDT, NULL, NUMBER_OF_SYSCALLS, Win32kSSPT, 1);
  if (Result == FALSE)
  {
    DbgPrint("Adding system services failed!\n");
    return STATUS_UNSUCCESSFUL;
  }

  DbgPrint("System services added successfully!\n");

  Status = InitWindowStationImpl();
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Failed to initialize window station implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitClassImpl();
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Failed to initialize window class implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitWindowImpl();
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Failed to initialize window implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

   Status = InitInputImpl();
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to initialize input implementation.\n");
      return(Status);
    }

  Status = MsqInitializeImpl();
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to initialize message queue implementation.\n");
      return(Status);
    }

  return STATUS_SUCCESS;
}


BOOLEAN
STDCALL
W32kInitialize (VOID)
{
  DbgPrint("W32kInitialize\n");

  InitGdiObjectHandleTable ();

  // FIXME: Retrieve name from registry
  EngLoadImage(L"\\SystemRoot\\system32\\drivers\\vidport.sys");

  // Create surface used to draw the internal font onto
  CreateCellCharSurface();

  // Create stock objects, ie. precreated objects commonly used by win32 applications
  CreateStockObjects();

  // Initialize FreeType library
  if(!InitFontSupport()) return FALSE;

  return TRUE;
}

/* EOF */
