#ifndef MYERROR
#define MYERROR


// 发生错误时打印错误信息、代码行号并退出程序
void errorExit(const bool errorFlag, 
                const char* errMsg, 
                const char* fileName, 
                const int errLine);


// 发生错误时打印错误信息、代码行号
void errorPrint(bool errorFLag, 
                const char* errMsg, 
                const char* fileName,
                const int errLine);

            
#endif