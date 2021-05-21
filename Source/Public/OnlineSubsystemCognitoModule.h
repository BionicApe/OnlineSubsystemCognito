// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

struct LibraryHelperOSSCognito
{
	FString LibraryName;
	FString LibraryPath;
	void* Handle;
};


/**
 * Online subsystem module class  (Cognito Implementation)
 * Code related to the loading of the Cognito module
 */
class FOnlineSubsystemCognitoModule : public IModuleInterface
{
private:

	/** Class responsible for creating instance(s) of the subsystem */
	class FOnlineFactoryCognito* CognitoFactory;
	
		
	static TArray<LibraryHelperOSSCognito> Libraries;

public:

	FOnlineSubsystemCognitoModule() : 
		CognitoFactory(NULL)
	{}

	virtual ~FOnlineSubsystemCognitoModule() {}

	// IModuleInterface

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

	virtual bool SupportsAutomaticShutdown() override
	{
		return false;
	}
};
