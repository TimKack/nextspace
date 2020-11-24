/*
 *	CFFileDescriptor.h
 *
 *  http://www.puredarwin.org/ 2009, 2018
 */

#if !defined(__COREFOUNDATION_CFFILEDESCRIPTOR__)
#define __COREFOUNDATION_CFFILEDESCRIPTOR__ 1

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFRunLoop.h>

#if __HAS_DISPATCH__
#include <dispatch/dispatch.h>
#endif

#if TARGET_OS_OSX || TARGET_OS_LINUX

CF_EXTERN_C_BEGIN

typedef int CFFileDescriptorNativeDescriptor;

typedef struct __CFFileDescriptor *CFFileDescriptorRef;

/* Callback Reason Types */
enum CFOptionFlags {
  kCFFileDescriptorReadCallBack = 1 << 0,
  kCFFileDescriptorWriteCallBack = 1 << 1
};

typedef void (*CFFileDescriptorCallBack)(CFFileDescriptorRef f, CFOptionFlags callBackTypes, void *info);

typedef struct {
  CFIndex version;
  void *info;
  void *(*retain)(void *info);
  void (*release)(void *info);
  CFStringRef (*copyDescription)(void *info);
} CFFileDescriptorContext;

CF_EXPORT CFTypeID CFFileDescriptorGetTypeID(void);

CF_EXPORT CFFileDescriptorRef CFFileDescriptorCreate(CFAllocatorRef allocator, CFFileDescriptorNativeDescriptor fd, Boolean closeOnInvalidate, CFFileDescriptorCallBack callout, const CFFileDescriptorContext *context);

CF_EXPORT CFFileDescriptorNativeDescriptor CFFileDescriptorGetNativeDescriptor(CFFileDescriptorRef f);

CF_EXPORT void CFFileDescriptorGetContext(CFFileDescriptorRef f, CFFileDescriptorContext *context);

CF_EXPORT void CFFileDescriptorEnableCallBacks(CFFileDescriptorRef f, CFOptionFlags callBackTypes);
CF_EXPORT void CFFileDescriptorDisableCallBacks(CFFileDescriptorRef f, CFOptionFlags callBackTypes);

CF_EXPORT void CFFileDescriptorInvalidate(CFFileDescriptorRef f);
CF_EXPORT Boolean CFFileDescriptorIsValid(CFFileDescriptorRef f);

CF_EXPORT CFRunLoopSourceRef CFFileDescriptorCreateRunLoopSource(CFAllocatorRef allocator, CFFileDescriptorRef f, CFIndex order);

CF_EXTERN_C_END

#endif /* DEPLOYMENT_TARGET */

#endif /* ! __COREFOUNDATION_CFFILEDESCRIPTOR__ */

