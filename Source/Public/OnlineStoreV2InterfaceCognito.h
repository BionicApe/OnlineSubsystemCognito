// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "OnlineSubsystemCognitoPackage.h"

class FOnlineSubsystemCognito;
class FUniqueNetIdCognito;

/**
 * Implementation for online store via Cognito interface
 */
class FOnlineStoreV2Cognito : public IOnlineStoreV2, public TSharedFromThis<FOnlineStoreV2Cognito, ESPMode::ThreadSafe>
{
public:
	FOnlineStoreV2Cognito(FOnlineSubsystemCognito& InCognitoSubsystem);
	virtual ~FOnlineStoreV2Cognito() = default;

public:// IOnlineStoreV2
	virtual void QueryCategories(const FUniqueNetId& UserId, const FOnQueryOnlineStoreCategoriesComplete& Delegate) override;
	virtual void GetCategories(TArray<FOnlineStoreCategory>& OutCategories) const override;
	virtual void QueryOffersByFilter(const FUniqueNetId& UserId, const FOnlineStoreFilter& Filter, const FOnQueryOnlineStoreOffersComplete& Delegate) override;
	virtual void QueryOffersById(const FUniqueNetId& UserId, const TArray<FUniqueOfferId>& OfferIds, const FOnQueryOnlineStoreOffersComplete& Delegate) override;
	virtual void GetOffers(TArray<FOnlineStoreOfferRef>& OutOffers) const override;
	virtual TSharedPtr<FOnlineStoreOffer> GetOffer(const FUniqueOfferId& OfferId) const override;

PACKAGE_SCOPE:
	void QueryOffers(const FUniqueNetIdCognito& UserId, const TArray<FUniqueOfferId>& OfferIds, const FOnQueryOnlineStoreOffersComplete& Delegate);

PACKAGE_SCOPE:
	FOnlineSubsystemCognito& CognitoSubsystem;
	TMap<FUniqueOfferId, FOnlineStoreOfferRef> AvailableOffers;

private:
	void CreateFakeOffer(const FString& Id, const FString& Title, const FString& Description, int32 Price);
};

using FOnlineStoreCognitoPtr = TSharedPtr<FOnlineStoreV2Cognito, ESPMode::ThreadSafe>;
using FOnlineStoreCognitoRef = TSharedRef<FOnlineStoreV2Cognito, ESPMode::ThreadSafe>;
