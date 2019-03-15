#ifndef __LEDUART_DEBUG_H__
#define __LEDUART_DEBUG_H__

#ifdef __cplusplus
extern "C"
{
#endif

//要使用的时候将这个定义打开
//#define __DEBUG

#ifdef __DEBUG
#define DEBUG(format,...) printf("File: %s, Line: %05d: \n"format"\n", __FILE__, __LINE__, ##__VA_ARGS__)
//##的意思是，如果可变参数被忽略为空，就让预处理器preprocessor去除前面那个逗号
#else
#define DEBUG(info)
#endif



#ifdef __cplusplus
}
#endif

#endif
