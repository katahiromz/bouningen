#ifndef PNGRES_H_
#define PNGRES_H_

#define PNG 256
#define RT_PNG MAKEINTRESOURCE(PNG)

#ifndef RC_INVOKED
    HBITMAP LoadPngAsBitmapFromResource(HINSTANCE hInstance, LPCTSTR pszName);

    class PngRes
    {
    protected:
        HBITMAP m_hbm;

    public:
        PngRes() : m_hbm(NULL) { }

        PngRes(HINSTANCE hInstance, LPCTSTR pszName)
        {
            m_hbm = NULL;
            Load(hInstance, pszName);
        }

        ~PngRes() { Free(); }

        VOID Free()
        {
            if (m_hbm != NULL)
            {
                DeleteObject(m_hbm);
                m_hbm = NULL;
            }
        }

        BOOL Load(HINSTANCE hInstance, LPCTSTR pszName)
        {
            Free();
            m_hbm = LoadPngAsBitmapFromResource(hInstance, pszName);
            return m_hbm != NULL;
        }

        HBITMAP GetHandle() const { return m_hbm; }

        BOOL GetBitmap(LPBITMAP pbm) const
        {
            return GetObject(m_hbm, sizeof(BITMAP), pbm) == sizeof(BITMAP);
        }

        VOID Premultiply();
    };
#endif  /* ndef RC_INVOKED */

#endif  // ndef PNGRES_H_
