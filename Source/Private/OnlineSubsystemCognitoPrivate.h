// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "Modules/ModuleManager.h"

#define INVALID_INDEX -1

/** URL Prefix when using Cognito socket connection */
#define COGNITO_URL_PREFIX TEXT("Cognito.")

/** pre-pended to all COGNITO logging */
#undef ONLINE_LOG_PREFIX
#define ONLINE_LOG_PREFIX TEXT("COGNITO: ")


