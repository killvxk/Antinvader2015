///////////////////////////////////////////////////////////////////////////////
//
// 版权所有 (c) 2011 - 2012
//
// 原始文件名称     : ConfidentialFile.h
// 工程名称         : AntinvaderDriver
// 创建时间         : 2011-07-28
//
//
// 描述             : 关于文件上下文等头文件
//
// 更新维护:
//  0001 [2011-07-28] 最初版本.废弃上一版本全部内容
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ntdef.h>
#include <ntifs.h>
#include <fltKernel.h>

#include "Common.h"
#include "AntinvaderDef.h"

////////////////////////
//      宏定义
////////////////////////

// 文件内存标志
#define MEM_TAG_FILE_TABLE          'cftm'

// 文件流上下文大小
#define FILE_STREAM_CONTEXT_SIZE    sizeof(_CUST_FILE_STREAM_CONTEXT)

#if 0

// 锁保护
#define FILE_STREAM_CONTEXT_LOCK_ON(_file_data)  ((void)0)
#define FILE_STREAM_CONTEXT_LOCK_OFF(_file_data) ((void)0)

#elif 0

// 锁保护
#define FILE_STREAM_CONTEXT_LOCK_ON(_file_data)  \
    if ((_file_data)->prResource != NULL) { \
        ExAcquireResourceExclusiveLite((_file_data)->prResource, TRUE); \
    }

#define FILE_STREAM_CONTEXT_LOCK_OFF(_file_data) \
    if ((_file_data)->prResource != NULL) { \
        ExReleaseResourceLite((_file_data)->prResource); \
    }

#elif 0

// 锁保护
#define FILE_STREAM_CONTEXT_LOCK_ON(_file_data)  \
    KeEnterCriticalRegion(); \
    if ((_file_data)->prResource != NULL) { \
        ExAcquireResourceExclusiveLite((_file_data)->prResource, TRUE); \
    }

#define FILE_STREAM_CONTEXT_LOCK_OFF(_file_data) \
    if ((_file_data)->prResource != NULL) { \
        ExReleaseResourceLite((_file_data)->prResource); \
    } \
    KeLeaveCriticalRegion()

#else

// 锁保护
#define FILE_STREAM_CONTEXT_LOCK_ON(_file_data)  \
    KeEnterCriticalRegion(); \
    if ((_file_data)->prResource != NULL) { \
        ExAcquireResourceExclusiveLite((_file_data)->prResource, TRUE); \
    } \
    KeLeaveCriticalRegion()

#define FILE_STREAM_CONTEXT_LOCK_OFF(_file_data) \
    KeEnterCriticalRegion(); \
    if ((_file_data)->prResource != NULL) { \
        ExReleaseResourceLite((_file_data)->prResource); \
    } \
    KeLeaveCriticalRegion()

#endif

//
// FILE_CLEAR_CACHE_USE_ORIGINAL_LOCK, 注意: 该宏定义在 AntinvaderDef.h 头文件里
//
#if defined(FILE_CLEAR_CACHE_USE_ORIGINAL_LOCK) && (FILE_CLEAR_CACHE_USE_ORIGINAL_LOCK != 0)

// 锁保护 for FileClearCache()
#define FILE_STREAM_CONTEXT_LOCK_ON_FOR_CACHE(_file_data)       ((void)0)
#define FILE_STREAM_CONTEXT_LOCK_OFF_FOR_CACHE(_file_data)      ((void)0)

#else

// 锁保护 for FileClearCache()
#define FILE_STREAM_CONTEXT_LOCK_ON_FOR_CACHE(_file_data)       FILE_STREAM_CONTEXT_LOCK_ON(_file_data)
#define FILE_STREAM_CONTEXT_LOCK_OFF_FOR_CACHE(_file_data)      FILE_STREAM_CONTEXT_LOCK_OFF(_file_data)

#endif

////////////////////////
//      常量定义
////////////////////////

// 机密文件头大小 为分页方便初始设置为4k
#define CONFIDENTIAL_FILE_HEAD_SIZE         1024

// 加密标识长度
#define ENCRYPTION_HEAD_LOGO_SIZE           12

// 加密标识, 注意修改加密标识后要将ENCRYPTION_HEAD_LOGO_SIZE修改为相应的数值.
#define ENCRYPTION_HEADER_BEGIN             { L'E',L'N',L'C',L'R',L'Y',L'P',L'T',L' ',L'F',L'L',L'A',L'G' }
#define ENCRYPTION_HEADER_END               { L'G',L'A',L'L',L'F',L' ',L'T',L'P',L'Y',L'R',L'C',L'N',L'E' }

////////////////////////
//      结构定义
////////////////////////

// 声明当前文件缓存中是密文还是明文, 机密进程还是非机密进程正在访问.
typedef enum _FILE_OPEN_STATUS {
    OPEN_STATUS_UNKNOWN = 0,        // 可以切换,需要刷掉缓存
    OPEN_STATUS_CONFIDENTIAL,       // 当前是机密进程正在访问
    OPEN_STATUS_NOT_CONFIDENTIAL    // 当前是非机密进程正在访问
} FILE_OPEN_STATUS;

// 声明当前文件是机密文件还是非机密文件.
typedef enum _FILE_ENCRYPTED_TYPE {
    ENCRYPTED_TYPE_UNKNOWN = 0,      // 未知状态
    ENCRYPTED_TYPE_ENCRYPTED,        // 文件是机密的
    ENCRYPTED_TYPE_NOT_ENCRYPTED     // 文件是非机密的
} FILE_ENCRYPTED_TYPE;

// 文件流上下文结构体
typedef struct _CUST_FILE_STREAM_CONTEXT
{
    PERESOURCE prResource;                  // 取锁资源
    FILE_ENCRYPTED_TYPE fctEncrypted;       // 是否被加密
    ULONG ulReferenceTimes;                 // 引用计数
    BOOLEAN bNeedUpdateHeadWhenClose;       // 是否需要在关闭文件时重写加密头
    BOOLEAN bCached;                        // 是否缓冲
    LARGE_INTEGER nFileValidLength ;        // 文件有效大小
    LARGE_INTEGER nFileSize ;               // 文件实际大小 包括了加密头等
    FILE_OPEN_STATUS fosOpenStatus;         // 当前缓存有授信/不可信进程打开
    PVOID pfcbFCB;                          // 缓冲FCB地址
//  PVOID pfcbNonCachedFCB;                 // 非缓冲FCB地址
    PVOID pvSwappedBuffer;                  // 新建的Buffer地址(用于替换的)
    PVOID pvOriginalBuffer;                 // 原始的Buffer地址(被替换的)
    UNICODE_STRING usName;                  // 文件名称
    UNICODE_STRING usPostFix;               // 文件后缀名
//  UNICODE_STRING usPath;                  // 文件路径
    WCHAR wszVolumeName[NORMAL_NAME_LENGTH];   // 卷名称

} CUST_FILE_STREAM_CONTEXT, * PCUST_FILE_STREAM_CONTEXT;

// 加密头结构体
typedef struct _CUST_FILE_ENCRYPTION_HEAD
{
    WCHAR wEncryptionLogo_begin[ENCRYPTION_HEAD_LOGO_SIZE];   // 24
    //WCHAR wSeperate0[4];        // 8
    //ULONG ulVersion;            // 4
    //WCHAR wSeperate1[4];        // 8
    LONGLONG nFileValidLength;  // 8
    //WCHAR wSeperate2[4];        // 8
    LONGLONG nFileRealSize;     // 8
    //WCHAR wSeperate3[4];        // 8
    //WCHAR wMD5Check[32];        // 64
    //WCHAR wSeperate4[4];        // 8
    //WCHAR wCRC32Check[32];      // 64
    //WCHAR wSeperate5[4];        // 8
    //WCHAR wKeyEncrypted[32];    // 64
	WCHAR wEncryptionLogo_end[ENCRYPTION_HEAD_LOGO_SIZE];   // 24
} CUST_FILE_ENCRYPTION_HEAD, * PCUST_FILE_ENCRYPTION_HEAD;

///////////////////////
//      函数声明
///////////////////////

NTSTATUS
FctCreateCustFileStreamContextForFileObject(
    __in PFLT_INSTANCE pfiInstance,
    __in PFILE_OBJECT pfoFileObject,
	__in PFLT_CALLBACK_DATA pfcdCBD,
	__in PFLT_FILE_NAME_INFORMATION pfniFileNameInformation,
    __inout PCUST_FILE_STREAM_CONTEXT * dpscFileStreamContext
);

NTSTATUS
FctUpdateCustFileStreamContextFileName(
    __in PUNICODE_STRING pusName,
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext
);

NTSTATUS
FctFreeCustFileStreamContext(
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext
);

NTSTATUS
FctInitializeCustFileStreamContext(
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext,
    __in PFLT_CALLBACK_DATA pfcdCBD,
    __in PFLT_FILE_NAME_INFORMATION pfniFileNameInformation
);

NTSTATUS
FctGetCustFileStreamContextByFileObject(
    __in PFLT_INSTANCE pfiInstance,
    __in PFILE_OBJECT pfoFileObject,
    __inout PCUST_FILE_STREAM_CONTEXT * dpscFileStreamContext
);

FILE_ENCRYPTED_TYPE
FctGetCustFileStreamContextEncryptedType(
    __in    PCUST_FILE_STREAM_CONTEXT pscFileStreamContext
);

VOID
FctDecCustFileStreamContextReferenceCount(
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext
);

//VOID
//FctIncCustFileStreamContextReferenceCount(
//    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext
//);

VOID
FctSetCustFileStreamContextEncryptedType(
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext,
    __in    FILE_ENCRYPTED_TYPE  fetFileEncryptedType
);

VOID FctReleaseCustFileStreamContext(
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext
);
BOOLEAN
FctStreamContextNeedRelease(
    __in    PCUST_FILE_STREAM_CONTEXT  pscFileStreamContext
);

BOOLEAN
FctNeedUpdateHeadWhenClose(
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext
);

VOID
FctSetNeedUpdateHeadWhenClose(
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext,
    __in    BOOLEAN              bSet
);

VOID
FctGetCustFileStreamContextValidSize(
    __in    PCUST_FILE_STREAM_CONTEXT pscFileStreamContext,
    __inout PLARGE_INTEGER       pnFileValidSize
);

VOID
FctUpdateCustFileStreamContextValidSize(
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext,
    __in    PLARGE_INTEGER       pnFileValidSize,
    __in    BOOLEAN              bSetUpdateWhenClose
);

BOOLEAN
FctUpdateCustFileStreamContextValidSizeIfLonger(
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext,
    __in    PLARGE_INTEGER       pnFileValidSize,
    __in    BOOLEAN              bSetUpdateWhenClose
);

NTSTATUS
FctEncodeCustFileStreamContextEncrytedHead(
    __in    PCUST_FILE_STREAM_CONTEXT pscFileStreamContext,
    __inout PVOID pFileHead
);

NTSTATUS
FctDecodeCustFileStreamContextEncrytedHead(
    __inout PCUST_FILE_STREAM_CONTEXT pscFileStreamContext,
    __in PVOID  pFileHead
);
