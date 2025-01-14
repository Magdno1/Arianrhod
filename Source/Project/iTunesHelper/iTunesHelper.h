#ifndef _APPLEIDREGISTERHELPER_H_4a22275b_1fae_441a_b825_0027d83753e4_
#define _APPLEIDREGISTERHELPER_H_4a22275b_1fae_441a_b825_0027d83753e4_

#include "iOSDevice.h"
#include "iOSService.h"
#include "iOSAFC.h"
#include "iOSATH.h"
#include "iOSDeviceMonitor.h"
#include "SectionProtector.h"

#define DSID_NOT_AUTHORIZED     ((NTSTATUS)(-42004))
#define AFSYC_SIGN_DATA_FILAED  ((NTSTATUS)(-42553))

typedef struct
{
    ULONG   length;
    BYTE    deviceId[20];

} FAIR_PLAY_HW_INFO, *PFAIR_PLAY_HW_INFO;

typedef struct
{
    HANDLE  session;
    ULONG64 what;

} SAP_SESSION, *PKBSYNC_SESSION;

typedef struct
{
    ULONG deviceType;
    ULONG keyTypeSupportVersion;

} AIR_FAIR_DEVICE_INFO, *PAIR_FAIR_DEVICE_INFO;

typedef struct
{
    ULONG64 LowPart;
    ULONG64 HighPart;

    ULONG64 operator[](int index)
    {
        ULARGE_INTEGER dsid = { (ULONG32)this[index].LowPart, (ULONG32)this[index].HighPart };
        return dsid.QuadPart;
    }

    ULONG64 at(int index)
    {
        return (*this)[index];
    }

} AIR_FAIR_AUTHORIZED_DSID, *PAIR_FAIR_AUTHORIZED_DSID;

typedef struct
{
    NTSTATUS (CDECL*    freeSessionData)(PVOID data);
    BOOLEAN  (FASTCALL* getDeviceId)(PFAIR_PLAY_HW_INFO deviceId);
    BOOLEAN  (FASTCALL* getDeviceId2)(PFAIR_PLAY_HW_INFO deviceId);

    NTSTATUS (CDECL*    loadCoreFP)(PVOID);
    NTSTATUS (FASTCALL* initScInfo)(BOOLEAN _False, PKBSYNC_SESSION *kbsyncSession, ULONG64 Zero);  // CDECL+FASTCALL

    NTSTATUS (CDECL*    kbsyncCreateSession)(PFAIR_PLAY_HW_INFO machineId, PFAIR_PLAY_HW_INFO whatId, PCSTR ScInfoPath, PHANDLE kbsyncSession, ...);
    NTSTATUS (CDECL*    kbsyncInitSomething)(HANDLE kbsyncSession, ULONG_PTR unknown1, ULONG_PTR unknown2);
    NTSTATUS (CDECL*    KbsyncAuthorizeDsid)(HANDLE kbsyncSession, ULONG64 dsid, ULONG_PTR reserved);
    NTSTATUS (CDECL*    KbsyncDsidBindMachine)(HANDLE kbsyncSesstion, PFAIR_PLAY_HW_INFO machineId2, ULONG64 dsid);
    NTSTATUS (CDECL*    kbsyncCloseSession)(HANDLE session);

    NTSTATUS (CDECL*    sapCreateSession)(PHANDLE sapSession, PFAIR_PLAY_HW_INFO deviceId);
    NTSTATUS (CDECL*    sapCloseSession)(HANDLE sapSession);

    NTSTATUS
    (CDECL*
    sapExchangeData)(
        ULONG               keyTypeSupportVersion,
        PFAIR_PLAY_HW_INFO  deviceId,
        HANDLE              sapSession,
        PVOID               certData,
        ULONG_PTR           certSize,
        PVOID*              output,
        PULONG_PTR          outputSize,
        PBOOLEAN            continueSync
    );

    NTSTATUS
    (CDECL*
    sapCreatePrimeSignature)(
        HANDLE      sapSession,
        ULONG       _64,
        ULONG64     zero,
        PVOID*      output,
        PULONG_PTR  outputSize
    );

    NTSTATUS
    (CDECL*
    sapVerifyPrimeSignature)(
        HANDLE      sapSession,
        PVOID       signature,
        ULONG_PTR   signatureSize,
        PVOID,
        PVOID
    );

    NTSTATUS
    (CDECL*
    sapSignData)(
        HANDLE      sapSession,
        PVOID       data,
        ULONG_PTR   dataSize,
        PVOID*      signature,
        PULONG_PTR  signatureSize
    );

    NTSTATUS
    (CDECL*
    airFairVerifyRequest)(
        ULONG       grappaSessionId,
        PVOID       afsyncRequest,
        ULONG_PTR   requestSize,
        PVOID       afsyncRequestSignature,
        ULONG_PTR   signatureSize
    );

    NTSTATUS
    (CDECL*
    airFairSyncCreateSession)(
        HANDLE                  kbsyncSession,
        PVOID                   fairPlayCertificate,
        ULONG_PTR               certificateSize,
        PFAIR_PLAY_HW_INFO              fairPlayGuid,
        PAIR_FAIR_DEVICE_INFO   deviceInfo,
        ULONG                   flags,  // 1 | 2 | 4
        PHANDLE                 afsyncSession
    );

    NTSTATUS
    (CDECL*
    airFairSyncSetRequest)(
        HANDLE      afsyncSession,
        PVOID       afsyncRequest,
        ULONG_PTR   requestSize,
        PVOID       ICInfo,
        ULONG_PTR   ICInfoSize
    );

    NTSTATUS
    (CDECL*
    airFairSyncAddAccount)(
        HANDLE      afsyncSession,
        ULONG64     dsPersonId,
        ULONG       what1,  // 0
        ULONG       what2   // 0
    );

    NTSTATUS
    (CDECL*
    airFairSyncGetAuthorizedAccount)(
        HANDLE      afsyncSession,
        PFAIR_PLAY_HW_INFO  fairPlayGuid,
        USHORT      deviceType,
        PVOID       afsyncRequest,
        ULONG_PTR   requestSize,
        PVOID*      authorizedAccount,
        PULONG      numberOfAccount
    );

    NTSTATUS
    (CDECL
    *airFairSyncGetResponse)(
        HANDLE      afsyncSession,
        PVOID*      afsyncResponse,
        PULONG      responseSize,
        PVOID*      someData,
        PULONG      dataSize
    );

    NTSTATUS
    (CDECL
    *airFairSyncSignData)(
        ULONG       grappaSessionId,
        PVOID       data,
        ULONG_PTR   dataLength,
        PVOID*      signature,
        PULONG      signatureLength
    );

} iTunesRoutines;

class iTunesHelper
{
public:
    typedef VOID (NTAPI *DeviceNotificationCallback)(PIOS_DEVICE Device, ULONG State, PVOID Context);

protected:
    typedef struct
    {
        DeviceNotificationCallback Callback;
        PVOID Context;

    } CALLBACK_ENTRY, *PCALLBACK_ENTRY;

public:
    iTunesHelper();

    NTSTATUS iTunesInitialize();

    /*++

        device methods

    --*/

    NTSTATUS
    DeviceNotificationSubscribe(
        DeviceNotificationCallback Callback,
        PVOID Context
    );

    NTSTATUS WaitForDeviceConnectionChanged(ULONG Timeout = INFINITE)
    {
        return this->monitor.WaitForDeviceConnectionChanged(Timeout);
    }

    /*++

        itunes wrapper

    --*/

    NTSTATUS    FreeSessionData(PVOID data);
    BOOL        GetDeviceId(PFAIR_PLAY_HW_INFO deviceId, PFAIR_PLAY_HW_INFO deviceId2);

    NTSTATUS    KbsyncCreateSession(PHANDLE kbsyncSession, PFAIR_PLAY_HW_INFO machineId, PFAIR_PLAY_HW_INFO machineId2, PCSTR ScInfoPath);
    NTSTATUS    KbsyncCloseSession(HANDLE session);

    /*++

        SAP

    --*/

    NTSTATUS    SapCreateSession(PHANDLE sapSession, PFAIR_PLAY_HW_INFO deviceId);
    NTSTATUS    SapCloseSession(HANDLE sapSession);
    NTSTATUS    SapCreatePrimeSignature(HANDLE sapSession, PVOID* output, PULONG_PTR outputSize);
    NTSTATUS    SapVerifyPrimeSignature(HANDLE sapSession, PVOID signature, ULONG_PTR signatureSize);

    NTSTATUS
    SapExchangeData(
        ULONG_PTR           certType,
        PFAIR_PLAY_HW_INFO  deviceId,
        HANDLE              sapSession,
        PVOID               certData,
        ULONG_PTR           certSize,
        PVOID*              output,
        PULONG_PTR          outputSize
    );

    NTSTATUS
    SapSignData(
        HANDLE      sapSession,
        PVOID       data,
        ULONG_PTR   dataSize,
        PVOID*      signature,
        PULONG_PTR  signatureSize
    );

    /*++

        afsync

    --*/

    NTSTATUS
    AirFairVerifyRequest(
        ULONG_PTR   grappaSessionId,
        PVOID       afsyncRequest,
        ULONG_PTR   requestSize,
        PVOID       afsyncRequestSignature,
        ULONG_PTR   signatureSize
    );

    NTSTATUS
    AirFairSyncCreateSession(
        PHANDLE                 afsyncSession,
        HANDLE                  kbsyncSession,
        PVOID                   fairPlayCertificate,
        ULONG_PTR               certificateSize,
        PFAIR_PLAY_HW_INFO      fairPlayGuid,
        PAIR_FAIR_DEVICE_INFO   deviceInfo,
        ULONG                   flags = 7   // 1 | 2 | 4
    );

    NTSTATUS
    AirFairSyncSetRequest(
        HANDLE      afsyncSession,
        PVOID       afsyncRequest,
        ULONG_PTR   requestSize,
        PVOID       ICInfo,
        ULONG_PTR   ICInfoSize
    );

    NTSTATUS
    AirFairSyncAddAccount(
        HANDLE              kbsyncSession,
        PFAIR_PLAY_HW_INFO  machineId2,
        HANDLE              afsyncSession,
        ULONG64             dsPersonId,
        ULONG               what1 = 0,
        ULONG               what2 = 0
    );

    NTSTATUS
    AirFairSyncGetAuthorizedAccount(
        HANDLE                      afsyncSession,
        PFAIR_PLAY_HW_INFO          fairPlayGuid,
        ULONG_PTR                   deviceType,
        PVOID                       afsyncRequest,
        ULONG_PTR                   requestSize,
        PAIR_FAIR_AUTHORIZED_DSID*  dsid,
        PULONG_PTR                  count
    );

    NTSTATUS
    AirFairSyncGetResponse(
        HANDLE      afsyncSession,
        PVOID*      afsyncResponse,
        PULONG      responseSize
    );

    NTSTATUS
    AirFairSyncReset(
        HANDLE      afsyncSession
    );

    NTSTATUS
    AirFairSyncSignData(
        ULONG       grappaSessionId,
        PVOID       data,
        ULONG_PTR   dataLength,
        PVOID*      signature,
        PULONG      signatureLength
    );

protected:
    NTSTATUS LoadiTunesRoutines();
    NTSTATUS InitScInfo(PKBSYNC_SESSION *kbsync = nullptr);

protected:
    PVOID           iTunesBase;
    BOOLEAN         Initialized;
    iTunesRoutines  iTunes;

    iOSDeviceMonitor monitor;
    RTL_CRITICAL_SECTION DeviceCallbacksLock;
    GrowableArray<CALLBACK_ENTRY> DeviceCallbacks;
};

#endif // _APPLEIDREGISTERHELPER_H_4a22275b_1fae_441a_b825_0027d83753e4_
