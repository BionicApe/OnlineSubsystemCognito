// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineAsyncTaskManagerCognito.h"

void FOnlineAsyncTaskManagerCognito::OnlineTick()
{
	check(CognitoSubsystem);
	check(FPlatformTLS::GetCurrentThreadId() == OnlineThreadId || !FPlatformProcess::SupportsMultithreading());
}

