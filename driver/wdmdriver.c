/*++

Module Name:

    wdmdriver.c

Abstract:

    A minimal Windows Driver Model (WDM) kernel-mode driver.

    The driver registers the standard WDM entry points (DriverEntry,
    AddDevice, an IRP dispatch routine and Unload) and creates a device
    object when added to a device stack. It does no real hardware work --
    it simply succeeds the IRPs it understands so it can serve as a
    starting point / build-verification sample.

Environment:

    Kernel mode only.

--*/

#include <wdm.h>

//
// Tag used for pool allocations made by this driver ("WdmD").
//
#define WDMDRIVER_POOL_TAG 'DmdW'

//
// Per-device extension. WDM drivers store their private state here rather
// than in globals so that multiple device instances can coexist.
//
typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT  DeviceObject;       // Back pointer to our FDO
    PDEVICE_OBJECT  NextLowerDriver;    // Device we attached on top of
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Forward declarations.
//
DRIVER_INITIALIZE   DriverEntry;
DRIVER_ADD_DEVICE   WdmAddDevice;
DRIVER_UNLOAD       WdmUnload;
DRIVER_DISPATCH     WdmDispatchPnp;
DRIVER_DISPATCH     WdmDispatchPassThrough;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, WdmAddDevice)
#pragma alloc_text(PAGE, WdmUnload)
#endif

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Called by the I/O manager when the driver is loaded. It wires up the
    driver-wide entry points that the system invokes later.

--*/
{
    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrint("WdmDriver: DriverEntry\n");

    //
    // Register the AddDevice and Unload callbacks.
    //
    DriverObject->DriverExtension->AddDevice = WdmAddDevice;
    DriverObject->DriverUnload                = WdmUnload;

    //
    // Provide dispatch routines for every major function code. Most are
    // simply passed down the stack; PnP is handled so the device can be
    // started and removed cleanly.
    //
    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = WdmDispatchPassThrough;
    }
    DriverObject->MajorFunction[IRP_MJ_PNP] = WdmDispatchPnp;

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
WdmAddDevice(
    PDRIVER_OBJECT  DriverObject,
    PDEVICE_OBJECT  PhysicalDeviceObject
    )
/*++

Routine Description:

    Called by the PnP manager for every device this driver is responsible
    for. It creates a functional device object (FDO) and attaches it to the
    device stack rooted at PhysicalDeviceObject.

--*/
{
    NTSTATUS          status;
    PDEVICE_OBJECT    deviceObject = NULL;
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_EXTENSION),
        NULL,                       // Unnamed FDO
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,                      // Not exclusive
        &deviceObject);

    if (!NT_SUCCESS(status)) {
        DbgPrint("WdmDriver: IoCreateDevice failed 0x%08X\n", status);
        return status;
    }

    deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
    deviceExtension->DeviceObject = deviceObject;

    //
    // Attach on top of the existing device stack.
    //
    deviceExtension->NextLowerDriver = IoAttachDeviceToDeviceStack(
        deviceObject,
        PhysicalDeviceObject);

    if (deviceExtension->NextLowerDriver == NULL) {
        DbgPrint("WdmDriver: IoAttachDeviceToDeviceStack failed\n");
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Inherit relevant flags from the device below us and announce that we
    // are done initializing.
    //
    deviceObject->Flags |= deviceExtension->NextLowerDriver->Flags &
                           (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DbgPrint("WdmDriver: AddDevice succeeded\n");
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
WdmDispatchPnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    )
/*++

Routine Description:

    Handles IRP_MJ_PNP. We let the lower driver do the real work and just
    pass the IRP down, handling REMOVE_DEVICE so we can tear down our FDO.

--*/
{
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION stack;
    NTSTATUS           status;

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    stack           = IoGetCurrentIrpStackLocation(Irp);

    switch (stack->MinorFunction) {
    case IRP_MN_REMOVE_DEVICE:

        //
        // Forward the remove down the stack first, then detach and delete.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(deviceExtension->NextLowerDriver, Irp);

        IoDetachDevice(deviceExtension->NextLowerDriver);
        IoDeleteDevice(DeviceObject);

        DbgPrint("WdmDriver: device removed\n");
        return status;

    default:

        //
        // Everything else flows straight through to the lower driver.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(deviceExtension->NextLowerDriver, Irp);
    }
}

_Use_decl_annotations_
NTSTATUS
WdmDispatchPassThrough(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    )
/*++

Routine Description:

    Default dispatch routine. Forwards the IRP to the next lower driver
    without modification.

--*/
{
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->NextLowerDriver, Irp);
}

_Use_decl_annotations_
VOID
WdmUnload(
    PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Called when the driver is being unloaded. By the time we get here the
    PnP manager has already removed every device, so there is nothing to
    free here.

--*/
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DriverObject);

    DbgPrint("WdmDriver: Unload\n");
}
