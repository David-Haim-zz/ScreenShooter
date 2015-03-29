#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <fstream>

PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp);
void CreateBMPFile(LPTSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC);

int main(int argc, _TCHAR* argv[]){
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	HWND desktopHandle = GetDesktopWindow();
	HDC desktopSurfaceHandle = GetWindowDC(desktopHandle);
	HDC destinationData = CreateCompatibleDC(desktopSurfaceHandle);
	HBITMAP bitmapHandle = CreateCompatibleBitmap(desktopSurfaceHandle, screenWidth, screenHeight);
	HGDIOBJ oldBitmapHandle = SelectObject(destinationData, bitmapHandle);
	bool copyingResult = BitBlt(destinationData, 0, 0, screenWidth, screenHeight, desktopSurfaceHandle, 0, 0, SRCCOPY | CAPTUREBLT);
	PBITMAPINFO info = CreateBitmapInfoStruct(bitmapHandle);
	LPTSTR defaultPath = _T("ScreenShot.bmp");
	CreateBMPFile(argc>1 ? argv[1] : defaultPath, info, bitmapHandle, destinationData);
	
	//cleanups
	delete[] info;
	DeleteObject(bitmapHandle);
	DeleteDC(destinationData);
	ReleaseDC(desktopHandle, desktopSurfaceHandle);
	return 0;
}

PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp){
	BITMAP bitmap;
	WORD    cClrBits;
	GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bitmap);
	cClrBits = (WORD)(bitmap.bmPlanes * bitmap.bmBitsPixel);

	if (cClrBits == 1){
		cClrBits = 1;
	} else if (cClrBits <= 4){
		cClrBits = 4;
	} else if (cClrBits <= 8){
		cClrBits = 8;
	} else if (cClrBits <= 16){
		cClrBits = 16;
	} else if (cClrBits <= 24){
		cClrBits = 24;
	} else {
		cClrBits = 32;
	}

	PBITMAPINFO bitmapInfo;
	if (cClrBits < 24){
		bitmapInfo = (PBITMAPINFO) new char [sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << cClrBits)];
	} else {
		bitmapInfo = (PBITMAPINFO) new char [sizeof(BITMAPINFOHEADER)];
	}

	bitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo->bmiHeader.biWidth = bitmap.bmWidth;
	bitmapInfo->bmiHeader.biHeight = bitmap.bmHeight;
	bitmapInfo->bmiHeader.biPlanes = bitmap.bmPlanes;
	bitmapInfo->bmiHeader.biBitCount = bitmap.bmBitsPixel;

	if (cClrBits < 24){
		bitmapInfo->bmiHeader.biClrUsed = (1 << cClrBits);
	}

	bitmapInfo->bmiHeader.biCompression = BI_RGB;
	bitmapInfo->bmiHeader.biSizeImage = ((bitmapInfo->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8 * bitmapInfo->bmiHeader.biHeight;
	bitmapInfo->bmiHeader.biClrImportant = 0;

	return bitmapInfo;
}

void CreateBMPFile(LPTSTR filePath, PBITMAPINFO bitmapInfo,HBITMAP bitmapHandle, HDC sourceContext){          
	BITMAPINFOHEADER* pointerToHeader = (BITMAPINFOHEADER*)bitmapInfo;
	char* buffer = new char[pointerToHeader->biSizeImage];
	GetDIBits(sourceContext, bitmapHandle, 0, (WORD)pointerToHeader->biHeight, buffer, bitmapInfo, DIB_RGB_COLORS);

	BITMAPFILEHEADER header;
	header.bfType = 0x4d42; 
	header.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pointerToHeader->biSize + pointerToHeader->biClrUsed * sizeof(RGBQUAD) + pointerToHeader->biSizeImage);
	header.bfReserved1 = 0;
	header.bfReserved2 = 0;
	header.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pointerToHeader->biSize + pointerToHeader->biClrUsed * sizeof(RGBQUAD);
	
	std::ofstream imageFile(filePath, std::ofstream::binary);
	if (!imageFile.good()){
		std::cout << "couldn't open file..\n";
		getchar();
		return;
	}

	imageFile.write(reinterpret_cast<const char*>(&header), sizeof(BITMAPFILEHEADER));
	imageFile.write(reinterpret_cast<const char*>(pointerToHeader), sizeof(BITMAPINFOHEADER) + pointerToHeader->biClrUsed * sizeof(RGBQUAD));
	imageFile.write(buffer, pointerToHeader->biSizeImage);
	
	imageFile.close();
	delete[] buffer;
}
