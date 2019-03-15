
#ifdef __HEX_BYTE_H__
#define __HEX_BYTE_H__

extern void ByteToHexStr(const unsigned char* source, char* dest, int sourceLen);
extern     void Hex2Str( const char *sSrc,  char *sDest, int nSrcLen);
extern void HexStrToByte(const char* source, unsigned char* dest, int sourceLen) ;


#endif

