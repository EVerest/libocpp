
target_sources(ocpp
    PRIVATE
        Authorize.cpp
        BootNotification.cpp
        CancelReservation.cpp
        CertificateSigned.cpp
        ChangeAvailability.cpp
        ChangeConfiguration.cpp
        ClearCache.cpp
        ClearChargingProfile.cpp
        DataTransfer.cpp
        DeleteCertificate.cpp
        DiagnosticsStatusNotification.cpp
        ExtendedTriggerMessage.cpp
        FirmwareStatusNotification.cpp
        GetCompositeSchedule.cpp
        GetConfiguration.cpp
        GetDiagnostics.cpp
        GetInstalledCertificateIds.cpp
        GetLocalListVersion.cpp
        GetLog.cpp
        Heartbeat.cpp
        InstallCertificate.cpp
        LogStatusNotification.cpp
        MeterValues.cpp
        RemoteStartTransaction.cpp
        RemoteStopTransaction.cpp
        ReserveNow.cpp
        Reset.cpp
        SecurityEventNotification.cpp
        SendLocalList.cpp
        SetChargingProfile.cpp
        SignCertificate.cpp
        SignedFirmwareStatusNotification.cpp
        SignedUpdateFirmware.cpp
        StartTransaction.cpp
        StatusNotification.cpp
        StopTransaction.cpp
        TriggerMessage.cpp
        UnlockConnector.cpp
        UpdateFirmware.cpp
    )
