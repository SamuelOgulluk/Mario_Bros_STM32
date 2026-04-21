/*------------------------------------------------------------------------*/
/* Unicode Support for FatFs                                               */
/* Provided by ChaN                                                        */
/*------------------------------------------------------------------------*/

#include "../ff.h"

#if _USE_LFN != 0

#if _STRF_ENCODE == 3   /* UTF-8 */
#define XMASK   0x80
#define XBIT    0x40
#define NBIT    0x20
static const BYTE Cdef[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                            10, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

WCHAR ff_convert(WCHAR src, UINT dir)
{
  BYTE cdef;
  static DWORD dw;
  static BYTE s1;
  WCHAR out = 0;

  if (dir) {
    out = src;
  } else {
    if (src < 0x80) {
      out = src;
    } else {
      cdef = Cdef[src >> 4];
      if (cdef & 1) {
        dw = src << 24;
        s1 = 1;
      } else if (s1 && (cdef & 0x80)) {
        dw = (dw >> 8) | (src << 24);
        s1++;
        if (s1 == 4) {
          out = dw >> 8;
          s1 = 0;
          dw = 0;
        }
      }
    }
  }
  return out;
}

WCHAR ff_wtoupper(WCHAR src)
{
  if (src < 0x80) {
    if (src >= 'a' && src <= 'z') src -= 0x20;
  }
  return src;
}

#elif _STRF_ENCODE == 2  /* UTF-16 */

WCHAR ff_convert(WCHAR src, UINT dir)
{
  return src;
}

WCHAR ff_wtoupper(WCHAR src)
{
  if (src < 0x80) {
    if (src >= 'a' && src <= 'z') src -= 0x20;
  }
  return src;
}

#else  /* Single Byte Encoding (SBCS) */

WCHAR ff_convert(WCHAR src, UINT dir)
{
  return src;
}

WCHAR ff_wtoupper(WCHAR src)
{
  if (src < 0x80) {
    if (src >= 'a' && src <= 'z') src -= 0x20;
  }
  return src;
}

#endif

#endif
