// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineAsyncTaskManager.h"

/**
 *	Cognito version of the async task manager to register the various Cognito callbacks with the engine
 */
class FOnlineAsyncTaskManagerCognito : public FOnlineAsyncTaskManager
{
protected:

	/** Cached reference to the main online subsystem */
	class FOnlineSubsystemCognito* CognitoSubsystem;

public:

	FOnlineAsyncTaskManagerCognito(class FOnlineSubsystemCognito* InOnlineSubsystem)
		: CognitoSubsystem(InOnlineSubsystem)
	{
	}

	~FOnlineAsyncTaskManagerCognito() 
	{
	}

	// FOnlineAsyncTaskManager
	virtual void OnlineTick() override;
};
