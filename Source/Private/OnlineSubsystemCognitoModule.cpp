// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemCognitoModule.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystemCognitoModule.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystem.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/MessageDialog.h"

IMPLEMENT_MODULE(FOnlineSubsystemCognitoModule, OnlineSubsystemCognito);


#define LOCTEXT_NAMESPACE "OnlineSubsystemCognito"

#if PLATFORM_WINDOWS
TArray<LibraryHelperOSSCognito> FOnlineSubsystemCognitoModule::Libraries =
{
	{
		TEXT("zlib1"),
		TEXT("Source/ThirdParty/BaAwsSdkCoreLibrary/bin/zlib1.dll"),
		nullptr
	},
	{
		TEXT("aws-c-common"),
		TEXT("Source/ThirdParty/BaAwsSdkCoreLibrary/bin/aws-c-common.dll"),
		nullptr
	},
	{
		TEXT("aws-checksums"),
		TEXT("Source/ThirdParty/BaAwsSdkCoreLibrary/bin/aws-checksums.dll"),
		nullptr
	},
	{
		TEXT("aws-c-event-stream"),
		TEXT("Source/ThirdParty/BaAwsSdkCoreLibrary/bin/aws-c-event-stream.dll"),
		nullptr
	},
	{
		TEXT("aws-cpp-sdk-core"),
		TEXT("Source/ThirdParty/BaAwsSdkCoreLibrary/bin/aws-cpp-sdk-core.dll"),
		nullptr
	},
	{
		TEXT("aws-cpp-sdk-cognito-idp"),
		TEXT("Source/ThirdParty/BaAwsSdkCoreLibrary/bin/aws-cpp-sdk-cognito-idp.dll"),
		nullptr
	}
};
#elif PLATFORM_MAC

#endif // PLATFORM_WINDOWS


/**
 * Class responsible for creating instance(s) of the subsystem
 */
class FOnlineFactoryCognito : public IOnlineFactory
{
public:

	FOnlineFactoryCognito() {}
	virtual ~FOnlineFactoryCognito() {}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		FOnlineSubsystemCognitoPtr OnlineSub = MakeShared<FOnlineSubsystemCognito, ESPMode::ThreadSafe>(InstanceName);
		if (OnlineSub->IsEnabled())
		{
			if(!OnlineSub->Init())
			{
				UE_LOG_ONLINE(Warning, TEXT("Cognito API failed to initialize!"));
				OnlineSub->Shutdown();
				OnlineSub = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Cognito API disabled!"));
			OnlineSub->Shutdown();
			OnlineSub = NULL;
		}

		return OnlineSub;
	}
};

void FOnlineSubsystemCognitoModule::StartupModule()
{
	
	FString BaseDir = IPluginManager::Get().FindPlugin("BaAwsSdk")->GetBaseDir();

	// Add on the relative location of the third party dll and load it

	for (LibraryHelperOSSCognito Library : Libraries)
	{
		FString const FullPath = FPaths::Combine(*BaseDir, *Library.LibraryPath);
		Library.Handle = !Library.LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*FullPath) : nullptr;
		if (!Library.Handle)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Name"), FText::FromString(Library.LibraryName));
			Arguments.Add(TEXT("Path"), FText::FromString(FullPath));
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LoadDependencyError", "Failed to load {Name} with Path: {Path} Plugin will not be functional"), Arguments));
		}
	}
	
	CognitoFactory = new FOnlineFactoryCognito();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(TEXT("Cognito"), CognitoFactory);
}

void FOnlineSubsystemCognitoModule::ShutdownModule()
{
	
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(TEXT("Cognito"));
	
	delete CognitoFactory;
	CognitoFactory = NULL;
	
	for (LibraryHelperOSSCognito Library : Libraries)
	{
		FPlatformProcess::FreeDllHandle(Library.Handle);
		Library.Handle = nullptr;
	}
}
#undef LOCTEXT_NAMESPACE
