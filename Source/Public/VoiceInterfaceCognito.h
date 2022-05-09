// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VoiceInterfaceImpl.h"
#include "Online/CoreOnline.h"
#include "OnlineSubsystemTypes.h"
#include "Interfaces/VoiceInterface.h"
#include "Net/VoiceDataCommon.h"
#include "OnlineSubsystemUtilsPackage.h"

class ONLINESUBSYSTEMCOGNITO_API FOnlineVoiceImplCognito : public FOnlineVoiceImpl {
PACKAGE_SCOPE:
	FOnlineVoiceImplCognito() : FOnlineVoiceImpl()
	{};

public:
	/** Constructor */
	FOnlineVoiceImplCognito(class IOnlineSubsystem* InOnlineSubsystem) :
		FOnlineVoiceImpl(InOnlineSubsystem)
	{
	};

	virtual ~FOnlineVoiceImplCognito() override {}
};

typedef TSharedPtr<FOnlineVoiceImpl, ESPMode::ThreadSafe> FOnlineVoiceImplPtr;
