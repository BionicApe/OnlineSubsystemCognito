// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemImpl.h"
#include "OnlineSubsystemCognitoPackage.h"
#include "HAL/ThreadSafeCounter.h"

class FOnlineAchievementsCognito;
class FOnlineIdentityCognito;
class FOnlineLeaderboardsCognito;
class FOnlineSessionCognito;
class FOnlineVoiceImpl;

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineSessionCognito, ESPMode::ThreadSafe> FOnlineSessionCognitoPtr;
typedef TSharedPtr<class FOnlineProfileCognito, ESPMode::ThreadSafe> FOnlineProfileCognitoPtr;
typedef TSharedPtr<class FOnlineFriendsCognito, ESPMode::ThreadSafe> FOnlineFriendsCognitoPtr;
typedef TSharedPtr<class FOnlineUserCloudCognito, ESPMode::ThreadSafe> FOnlineUserCloudCognitoPtr;
typedef TSharedPtr<class FOnlineLeaderboardsCognito, ESPMode::ThreadSafe> FOnlineLeaderboardsCognitoPtr;
typedef TSharedPtr<class FOnlineVoiceImpl, ESPMode::ThreadSafe> FOnlineVoiceImplPtr;
typedef TSharedPtr<class FOnlineExternalUICognito, ESPMode::ThreadSafe> FOnlineExternalUICognitoPtr;
typedef TSharedPtr<class FOnlineIdentityCognito, ESPMode::ThreadSafe> FOnlineIdentityCognitoPtr;
typedef TSharedPtr<class FOnlineAchievementsCognito, ESPMode::ThreadSafe> FOnlineAchievementsCognitoPtr;
typedef TSharedPtr<class FOnlineStoreV2Cognito, ESPMode::ThreadSafe> FOnlineStoreV2CognitoPtr;
typedef TSharedPtr<class FOnlinePurchaseCognito, ESPMode::ThreadSafe> FOnlinePurchaseCognitoPtr;

/**
 *	OnlineSubsystemCognito - Implementation of the online subsystem for Cognito services
 */
class ONLINESUBSYSTEMCOGNITO_API FOnlineSubsystemCognito : 
	public FOnlineSubsystemImpl
{

public:

	virtual ~FOnlineSubsystemCognito() = default;

	// IOnlineSubsystem

	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;	
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override;
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override;
	virtual IOnlineEventsPtr GetEventsInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineSharingPtr GetSharingInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineMessagePtr GetMessageInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IOnlineChatPtr GetChatInterface() const override;
	virtual IOnlineStatsPtr GetStatsInterface() const override;
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;
	virtual IOnlineTournamentPtr GetTournamentInterface() const override;

	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual FText GetOnlineServiceName() const override;

	// FTickerObjectBase
	
	virtual bool Tick(float DeltaTime) override;

	// FOnlineSubsystemCognito

PACKAGE_SCOPE:

	/** Only the factory makes instances */
	FOnlineSubsystemCognito() = delete;
	explicit FOnlineSubsystemCognito(FName InInstanceName) :
		FOnlineSubsystemImpl(TEXT("Cognito"), InInstanceName),
		SessionInterface(nullptr),
		VoiceInterface(nullptr),
		bVoiceInterfaceInitialized(false),
		LeaderboardsInterface(nullptr),
		IdentityInterface(nullptr),
		AchievementsInterface(nullptr),
		StoreV2Interface(nullptr),
		OnlineAsyncTaskThreadRunnable(nullptr),
		OnlineAsyncTaskThread(nullptr)
	{}

private:

	/** Interface to the session services */
	FOnlineSessionCognitoPtr SessionInterface;

	/** Interface for voice communication */
	mutable IOnlineVoicePtr VoiceInterface;

	/** Interface for voice communication */
	mutable bool bVoiceInterfaceInitialized;

	/** Interface to the leaderboard services */
	FOnlineLeaderboardsCognitoPtr LeaderboardsInterface;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityCognitoPtr IdentityInterface;

	/** Interface for achievements */
	FOnlineAchievementsCognitoPtr AchievementsInterface;

	/** Interface for store */
	FOnlineStoreV2CognitoPtr StoreV2Interface;

	/** Interface for purchases */
	FOnlinePurchaseCognitoPtr PurchaseInterface;

	/** Online async task runnable */
	class FOnlineAsyncTaskManagerCognito* OnlineAsyncTaskThreadRunnable;

	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;

	// task counter, used to generate unique thread names for each task
	static FThreadSafeCounter TaskCounter;
};

typedef TSharedPtr<FOnlineSubsystemCognito, ESPMode::ThreadSafe> FOnlineSubsystemCognitoPtr;

