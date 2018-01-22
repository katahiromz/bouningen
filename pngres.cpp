#include <windows.h>
#include <png.h>

#include "pngres.h"

#define WIDTHBYTES(i) (((i) + 31) / 32 * 4)

#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "msimg32.lib")

struct PngMemory
{
    LPBYTE m_pb;
    INT m_i;
    INT m_cb;
    PngMemory(LPVOID pv, INT i, INT cb)
    : m_pb((LPBYTE)pv), m_i(i), m_cb(cb)
    {
    }
};

static void PngReadProc(png_structp png, png_bytep data, png_size_t length)
{
    PngMemory *pngmem = (PngMemory *)png_get_io_ptr(png);
    CopyMemory(data, pngmem->m_pb + pngmem->m_i, length);
    pngmem->m_i += length;
}

static HBITMAP LoadPngAsBitmapFromMemory(LPVOID pv, INT cb)
{
    HBITMAP         hbm;
    png_structp     png;
    png_infop       info;
    png_uint_32     y, width, height, rowbytes;
    int             color_type, depth, widthbytes;
    double          gamma;
    BITMAPINFO      bi;
    LPBYTE          pbBits;
    PngMemory       pngmem(pv, 0, cb);
    png_bytepp      row_pointers;

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == NULL)
    {
        return NULL;
    }

    info = png_create_info_struct(png);
    if (info == NULL || setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, NULL, NULL);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

    png_set_read_fn(png, &pngmem, PngReadProc);
    png_read_info(png, info);

    png_get_IHDR(png, info, &width, &height, &depth, &color_type,
                 NULL, NULL, NULL);
    png_set_expand(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);
    png_set_strip_16(png);
    png_set_gray_to_rgb(png);
    png_set_palette_to_rgb(png);
    png_set_bgr(png);
    png_set_packing(png);
    if (png_get_gAMA(png, info, &gamma))
        png_set_gamma(png, 2.2, gamma);
    else
        png_set_gamma(png, 2.2, 0.45455);

    png_read_update_info(png, info);
    png_get_IHDR(png, info, &width, &height, &depth, &color_type,
                 NULL, NULL, NULL);

    rowbytes = png_get_rowbytes(png, info);
    row_pointers = (png_bytepp)malloc(height * sizeof(png_bytep));
    for (y = 0; y < height; y++)
    {
        row_pointers[y] = (png_bytep)png_malloc(png, rowbytes);
    }

    png_read_image(png, row_pointers);
    png_read_end(png, NULL);

    ZeroMemory(&bi.bmiHeader, sizeof(BITMAPINFOHEADER));
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = width;
    bi.bmiHeader.biHeight      = height;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = (WORD)(depth * png_get_channels(png, info));

    hbm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (VOID **)&pbBits,
                           NULL, 0);
    if (hbm == NULL)
    {
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

    widthbytes = WIDTHBYTES(width * bi.bmiHeader.biBitCount);
    for(y = 0; y < height; y++)
    {
        CopyMemory(pbBits + y * widthbytes,
                   row_pointers[height - 1 - y], rowbytes);
    }

    png_destroy_read_struct(&png, &info, NULL);
    free(row_pointers);
    return hbm;
}

HBITMAP LoadPngAsBitmapFromResource(HINSTANCE hInstance, LPCTSTR pszName)
{
    HRSRC hRsrc;
    HGLOBAL hGlobal;
    DWORD Size;
    HBITMAP hbm;
    
    LPVOID lpData;

    hRsrc = FindResource(hInstance, pszName, RT_PNG);
    if (hRsrc == NULL)
        return NULL;

    Size = SizeofResource(hInstance, hRsrc);
    hGlobal = LoadResource(hInstance, hRsrc);
    if (hGlobal == NULL)
        return NULL;

    lpData = LockResource(hGlobal);
    hbm = LoadPngAsBitmapFromMemory(lpData, Size);
    UnlockResource(hGlobal);
    FreeResource(hGlobal);

    return hbm;
}

VOID PngRes::Premultiply()
{
    BITMAP bm;
    if (GetBitmap(&bm) && bm.bmBitsPixel == 32)
    {
        LONG cx = bm.bmWidth;
        LONG cy = bm.bmHeight;
        DWORD cdw = cx * cy;
        LPBYTE pb = LPBYTE(bm.bmBits);
        BYTE alpha;
        while (cdw--)
        {
            alpha = pb[3];
            pb[0] = (BYTE) ((DWORD) pb[0] * alpha / 255);
            pb[1] = (BYTE) ((DWORD) pb[1] * alpha / 255);
            pb[2] = (BYTE) ((DWORD) pb[2] * alpha / 255);
            pb += 4;
        }
    }
}
