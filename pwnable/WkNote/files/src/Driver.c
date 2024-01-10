#include <ntifs.h>
#include <ntddk.h>

#define NT_DEVICE_NAME	L"\\Device\\WkNote"
#define SYM_LINK_NAME	L"\\??\\WkNote"

void WkNoteUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS WkNoteCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS WkNoteClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS WkNoteRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS WkNoteWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS WkNoteDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS        ntStatus;
	UNICODE_STRING  devName;
	PDEVICE_OBJECT  deviceObject;

	RtlInitUnicodeString(&devName, NT_DEVICE_NAME);

	ntStatus = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject);
	if (!NT_SUCCESS(ntStatus)) {
		KdPrint(("Failed to create device (0x%08X)\n", ntStatus));
		return ntStatus;
	}

	DriverObject->DriverUnload = WkNoteUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE]         = WkNoteCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]          = WkNoteClose;
	DriverObject->MajorFunction[IRP_MJ_READ]           = WkNoteRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE]          = WkNoteWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WkNoteDeviceControl;

	UNICODE_STRING symlinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);
	ntStatus = IoCreateSymbolicLink(&symlinkName, &devName);
	if (!NT_SUCCESS(ntStatus)) {
		KdPrint(("Failed to create symbolic link (0x%08X)\n", ntStatus));
		IoDeleteDevice(deviceObject);
		return ntStatus;
	}

	return STATUS_SUCCESS;
}

void WkNoteUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING symlinkName = RTL_CONSTANT_STRING(SYM_LINK_NAME);
	// delete symbolic link
	IoDeleteSymbolicLink(&symlinkName);

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);
}
