#include <ntifs.h>
#include <ntddk.h>
#include "WkNote.h"

#pragma warning(disable : 4996)
#pragma warning(disable : 4305)

#define POOL_TAG     'nccs'
#define MAX_ENT      4
#define SPACE_FREED 0xdeadbeef

typedef struct NoteEnt {
	PCHAR Buf;
	ULONG nSpace, nWritten;
} NOTE_ENT, *PNOTE_ENT;

typedef struct NoteRoot {
	NOTE_ENT Entry[MAX_ENT];
	KMUTEX   Lock;
} NOTE_ROOT, *PNOTE_ROOT;

NTSTATUS WkNoteCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	NTSTATUS ntStatus = STATUS_NO_MEMORY;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

	PNOTE_ROOT pNoteRoot = (PNOTE_ROOT)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(NOTE_ROOT), (ULONG)POOL_TAG);
	if (pNoteRoot) {
		KeInitializeMutex(&pNoteRoot->Lock, 0);
		for (int i = 0; i < MAX_ENT; i++)
			pNoteRoot->Entry[i].nSpace = SPACE_FREED;
		irpSp->FileObject->FsContext  = pNoteRoot;
		irpSp->FileObject->FsContext2 = 0;
		ntStatus = STATUS_SUCCESS;
	}

	Irp->IoStatus.Status = ntStatus;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return ntStatus;
}

NTSTATUS WkNoteClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT pFileObject = irpSp->FileObject;
	PNOTE_ROOT pNoteRoot     = pFileObject->FsContext;
	PNOTE_ENT pNoteArr       = pNoteRoot->Entry;

	NTSTATUS ntStatus = KeWaitForSingleObject(&pNoteRoot->Lock, UserRequest, KernelMode, TRUE, NULL);
	if (!NT_SUCCESS(ntStatus))
		return ntStatus;

	for (int i = 0; i < MAX_ENT; i++) {
		if(pNoteArr[i].Buf && pNoteArr[i].nSpace != SPACE_FREED)
			ExFreePoolWithTag(pNoteArr[i].Buf, (ULONG)POOL_TAG);
	}
	ExFreePoolWithTag(pNoteRoot, (ULONG)POOL_TAG);
	pFileObject->FsContext  = NULL;
	pFileObject->FsContext2 = 0;

	KeReleaseMutex(&pNoteRoot->Lock, FALSE);
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS WkNoteDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	ULONG inBufLen           = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	PWKNOTE_IOREQ req        = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
	PFILE_OBJECT pFileObject = irpSp->FileObject;
	PNOTE_ROOT pNoteRoot     = pFileObject->FsContext;
	PNOTE_ENT pNoteArr       = pNoteRoot->Entry;

	NTSTATUS ntStatus = KeWaitForSingleObject(&pNoteRoot->Lock, UserRequest, KernelMode, TRUE, NULL);
	if (!NT_SUCCESS(ntStatus))
		return ntStatus;

	ntStatus = STATUS_INVALID_PARAMETER;

	__try {
		ProbeForRead(req, inBufLen, __alignof(NOTE_ENT));

		if (inBufLen != sizeof(WKNOTE_IOREQ) || req->Id >= MAX_ENT)
			goto END;

		switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
			case IOCTL_WKNOTE_SELECT:
				pFileObject->FsContext2 = (PVOID)req->Id;
				ntStatus = STATUS_SUCCESS;
				break;
			case IOCTL_WKNOTE_ALLOCATE:
			{
				PNOTE_ENT pNote = &pNoteArr[req->Id];
				ULONG sz = req->Size;

				if (pNote->nSpace != SPACE_FREED || !sz || sz >= 0x1000)
					break;

				pNote->Buf = ExAllocatePoolWithTag(NonPagedPoolNx, sz, (ULONG)POOL_TAG);
				if (pNote->Buf) {
					pNote->nSpace = sz;
					pNote->nWritten = 0;
					ntStatus = STATUS_SUCCESS;
				}
				break;
			}
			case IOCTL_WKNOTE_REMOVE:
			{
				PNOTE_ENT pNote = &pNoteArr[req->Id];

				if (pNote->Buf) {
					ExFreePoolWithTag(pNote->Buf, (ULONG)POOL_TAG);
					pNote->Buf = NULL;
					pNote->nSpace = SPACE_FREED;
					pNote->nWritten = 0;
					ntStatus = STATUS_SUCCESS;
				}
				break;
			}
		}
	}
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ntStatus = GetExceptionCode();
        DbgPrint("[-] Exception Code: 0x%X\n", ntStatus);
    }

END:
	KeReleaseMutex(&pNoteRoot->Lock, FALSE);

	Irp->IoStatus.Status = ntStatus;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return ntStatus;
}

NTSTATUS WkNoteRead(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	ULONG readLen            = irpSp->Parameters.Read.Length;
	PCHAR UserBuffer         = Irp->UserBuffer;
	PFILE_OBJECT pFileObject = irpSp->FileObject;
	PNOTE_ROOT pNoteRoot     = pFileObject->FsContext;
	PNOTE_ENT pNote          = &pNoteRoot->Entry[(UINT16)pFileObject->FsContext2];

	NTSTATUS ntStatus = KeWaitForSingleObject(&pNoteRoot->Lock, UserRequest, KernelMode, TRUE, NULL);
	if (!NT_SUCCESS(ntStatus))
		return ntStatus;

	ntStatus = STATUS_NO_MEMORY;

	if(!MmIsAddressValid(pNote->Buf) || !pNote->nWritten)
		goto END;

	__try {
		ProbeForWrite(UserBuffer, readLen, __alignof(CHAR));

		if (pNote->nWritten < readLen)
			readLen = pNote->nWritten;
		RtlCopyMemory(UserBuffer, pNote->Buf, readLen);
		ntStatus = STATUS_SUCCESS;
	}
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ntStatus = GetExceptionCode();
        DbgPrint("[-] Exception Code: 0x%X\n", ntStatus);
    }

END:
	KeReleaseMutex(&pNoteRoot->Lock, FALSE);

	Irp->IoStatus.Status = ntStatus;
	Irp->IoStatus.Information = readLen;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return ntStatus;
}

NTSTATUS WkNoteWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	ULONG writeLen           = irpSp->Parameters.Write.Length;
	PCHAR UserBuffer         = Irp->UserBuffer;
	PFILE_OBJECT pFileObject = irpSp->FileObject;
	PNOTE_ROOT pNoteRoot     = pFileObject->FsContext;
	PNOTE_ENT pNote          = &pNoteRoot->Entry[(UINT16)pFileObject->FsContext2];

	NTSTATUS ntStatus = KeWaitForSingleObject(&pNoteRoot->Lock, UserRequest, KernelMode, TRUE, NULL);
	if (!NT_SUCCESS(ntStatus))
		return ntStatus;

	ntStatus = STATUS_NO_MEMORY;

	if(!MmIsAddressValid(pNote->Buf) || pNote->nSpace == SPACE_FREED)
		goto END;

	__try {
		ProbeForRead(UserBuffer, writeLen, __alignof(CHAR));

		if (pNote->nSpace < writeLen)
			writeLen = pNote->nSpace;
		RtlCopyMemory(pNote->Buf, UserBuffer, writeLen);
		pNote->nWritten = writeLen;
		ntStatus = STATUS_SUCCESS;
	}
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ntStatus = GetExceptionCode();
        DbgPrint("[-] Exception Code: 0x%X\n", ntStatus);
    }

END:
	KeReleaseMutex(&pNoteRoot->Lock, FALSE);

	Irp->IoStatus.Status = ntStatus;
	Irp->IoStatus.Information = writeLen;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return ntStatus;
}