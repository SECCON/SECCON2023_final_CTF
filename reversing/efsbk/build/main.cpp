#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")

HANDLE hOutput = NULL;
BOOL efsReadDone = FALSE;

DWORD EFSReadCallback(PBYTE pbData, PVOID pvCallbackContext, ULONG ulLength) {
	DWORD dwBytesWritten;
	if (ulLength == 0) {
		efsReadDone = TRUE;
		return ERROR_SUCCESS;
	}

	WriteFile(hOutput, pbData, ulLength, &dwBytesWritten, NULL);
	return ERROR_SUCCESS;
}

int main() {
	PVOID pvContext = NULL;
	HCERTSTORE hStore = NULL;
	PCCERT_CONTEXT pCertContext = NULL;
	CRYPT_DATA_BLOB PFXBlob = { 0 };
	LPCWSTR password = L"SECCON CTF 2023 Finals";
	WCHAR certSubject[0x40];
	DWORD certSujectLen = sizeof(certSubject) / sizeof(WCHAR);
	DWORD dwBytesWritten;

	hOutput = CreateFile(L"flag.bin", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOutput == INVALID_HANDLE_VALUE) goto cleanup;

	if (OpenEncryptedFileRaw(L"flag.txt", 0, &pvContext) != ERROR_SUCCESS)
		goto cleanup;
	if (ReadEncryptedFileRaw(EFSReadCallback, NULL, pvContext) != ERROR_SUCCESS)
		goto cleanup;

	while (!efsReadDone) {
		Sleep(1);
	}

	GetUserName(certSubject, &certSujectLen);
	hStore = CertOpenSystemStore(NULL, L"MY");
	if (!hStore) goto cleanup;
	pCertContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, certSubject, NULL);
	if (!pCertContext) goto cleanup;

	if (!PFXExportCertStoreEx(hStore, &PFXBlob, password, NULL,
		                      EXPORT_PRIVATE_KEYS | REPORT_NO_PRIVATE_KEY | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY))
		goto cleanup;
	PFXBlob.pbData = (PBYTE)HeapAlloc(GetProcessHeap(), 0, PFXBlob.cbData);
	if (!PFXExportCertStoreEx(hStore, &PFXBlob, password, NULL,
		                      EXPORT_PRIVATE_KEYS | REPORT_NO_PRIVATE_KEY | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY))
		goto cleanup;

	WriteFile(hOutput, PFXBlob.pbData, PFXBlob.cbData, &dwBytesWritten, NULL);

cleanup:
	if (hOutput) CloseHandle(hOutput);
	if (pvContext) CloseEncryptedFileRaw(pvContext);
	if (PFXBlob.pbData) HeapFree(GetProcessHeap(), 0, PFXBlob.pbData);
	if (pCertContext) CertFreeCertificateContext(pCertContext);
	if (hStore) CertCloseStore(hStore, 0);
	return 0;
}
