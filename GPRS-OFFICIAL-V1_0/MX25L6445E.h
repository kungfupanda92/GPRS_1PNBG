#ifndef _MX25L6445E_h
#define _MX25L6445E_h

#include "main.h"
typedef int16u uint16_t
typedef int8u uint8_t

void DF_DelayMs(int8u ucTime);
void DF_WriteState(int8u State);
int8u DF_ReadState(void)
void DF_ReadID(int8u *buf)
void _DF_ReadData(int8u *ucBuf,int32u ulAdr, int16u uiLen)
void DF_WriteCmd(int8u Cmd)
void DF_WaiteBusy(void)
void DF_EraseSec(int32u Addr)
void _DF_Write(int32u ulAdr,int8u *ucBuf,int16u uiLen)
void _DF_WritePage(int32u ulPage,int8u *ucBuf,int16u uiLen)
void _DF_ReadPage(int8u *ucBuf,int32u ulPage,int16u uiLen)
void DF_Write(int32u ulAdr,int8u *ucBuf,int16u uiLen)
void _DF_ReadRand(int8u *ucBuf,int32u ulPage,int16u uiOfs,int16u uiLen)
void DF_Read(int8u *ucBuf,int32u ulAdr,int16u uiLen)
void DF_Check(void)
void DF_Test(void)




#endif