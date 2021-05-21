// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlinePurchaseInterfaceCognito.h"
#include "OnlineStoreV2InterfaceCognito.h"
#include "OnlineSubsystemCognito.h"
#include "OnlineError.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	static FPurchaseReceipt::FReceiptOfferEntry MakeReceiptOfferEntry(const FUniqueNetIdCognito& CognitoUserId, const FString& Id, const FString& Name)
	{
		FPurchaseReceipt::FReceiptOfferEntry OfferEntry(FString(), Id, 1);
		{
			FPurchaseReceipt::FLineItemInfo LineItem;
			LineItem.ItemName = Name;
			LineItem.UniqueId = Id;
			OfferEntry.LineItems.Emplace(MoveTemp(LineItem));
		}

		return OfferEntry;
	}
}

FOnlinePurchaseCognito::FOnlinePurchaseCognito(FOnlineSubsystemCognito& InCognitoSubsystem)
	: CognitoSubsystem(InCognitoSubsystem)
{
}

FOnlinePurchaseCognito::~FOnlinePurchaseCognito()
{
}

void FOnlinePurchaseCognito::Tick()
{
	if (PendingPurchaseFailTime.IsSet() && PendingPurchaseDelegate.IsSet())
	{
		if (FPlatformTime::Seconds() > PendingPurchaseFailTime.GetValue())
		{
			FOnPurchaseCheckoutComplete Delegate = MoveTemp(PendingPurchaseDelegate.GetValue());
			PendingPurchaseDelegate.Reset();
			PendingPurchaseFailTime.Reset();

			Delegate.ExecuteIfBound(FOnlineError(TEXT("Checkout was cancelled or timed out")), MakeShared<FPurchaseReceipt>());
		}
	}
}

bool FOnlinePurchaseCognito::IsAllowedToPurchase(const FUniqueNetId& UserId)
{
	return true;
}

void FOnlinePurchaseCognito::Checkout(const FUniqueNetId& UserId, const FPurchaseCheckoutRequest& CheckoutRequest, const FOnPurchaseCheckoutComplete& Delegate)
{
	// Lambda to wrap calling our delegate with an error and logging the message
	auto CallDelegateError = [this, &Delegate](FString&& ErrorMessage)
	{
		CognitoSubsystem.ExecuteNextTick([Delegate, MovedErrorMessage = MoveTemp(ErrorMessage)]() mutable
		{
			UE_LOG_ONLINE(Error, TEXT("%s"), *MovedErrorMessage);

			const TSharedRef<FPurchaseReceipt> PurchaseReceipt = MakeShared<FPurchaseReceipt>();
			PurchaseReceipt->TransactionState = EPurchaseTransactionState::Failed;

			Delegate.ExecuteIfBound(FOnlineError(MoveTemp(MovedErrorMessage)), PurchaseReceipt);
		});
	};

	if (CheckoutRequest.PurchaseOffers.Num() == 0)
	{
		CallDelegateError(TEXT("FOnlinePurchaseCognito::Checkout failed, there were no entries passed to purchase"));
		return;
	}
	else if (CheckoutRequest.PurchaseOffers.Num() != 1)
	{
		CallDelegateError(TEXT("FOnlinePurchaseCognito::Checkout failed, there were more than one entry passed to purchase. We currently only support one."));
		return;
	}

	check(CheckoutRequest.PurchaseOffers.IsValidIndex(0));
	const FPurchaseCheckoutRequest::FPurchaseOfferEntry& Entry = CheckoutRequest.PurchaseOffers[0];

	if (Entry.Quantity != 1)
	{
		CallDelegateError(TEXT("FOnlinePurchaseCognito::Checkout failed, purchase quantity not set to one. We currently only support one."));
		return;
	}

	if (Entry.OfferId.IsEmpty())
	{
		CallDelegateError(TEXT("FOnlinePurchaseCognito::Checkout failed, OfferId is blank."));
		return;
	}

	const IOnlineStoreV2Ptr CognitoStoreInt = CognitoSubsystem.GetStoreV2Interface();

	TSharedPtr<FOnlineStoreOffer> CognitoOffer = CognitoStoreInt->GetOffer(Entry.OfferId);
	if (!CognitoOffer.IsValid())
	{
		CallDelegateError(TEXT("FOnlinePurchaseCognito::Checkout failed, Could not find corresponding offer."));
		return;
	}

	if (PendingPurchaseDelegate.IsSet())
	{
		CallDelegateError(TEXT("FOnlinePurchaseCognito::Checkout failed, there was another purchase in progress."));
		return;
	}

	PendingPurchaseDelegate = Delegate;

	TWeakPtr<FOnlinePurchaseCognito, ESPMode::ThreadSafe> WeakMe = AsShared();
	const FUniqueNetIdCognito& CognitoUserId = static_cast<const FUniqueNetIdCognito&>(UserId);

	CognitoSubsystem.ExecuteNextTick([CognitoUserId, CognitoOffer, WeakMe]
	{
		FOnlinePurchaseCognitoPtr StrongThis = WeakMe.Pin();
		if (StrongThis.IsValid())
		{
			StrongThis->CheckoutSuccessfully(CognitoUserId, CognitoOffer);
		}
	});
}

void FOnlinePurchaseCognito::CheckoutSuccessfully(const FUniqueNetIdCognito& UserId, TSharedPtr<FOnlineStoreOffer> Offer)
{
	// Cache this receipt
	TArray<FPurchaseReceipt>& UserReceipts = UserFakeReceipts.FindOrAdd(UserId);
	FPurchaseReceipt& PurchaseReceipt = UserReceipts.Emplace_GetRef();
	PurchaseReceipt.AddReceiptOffer(MakeReceiptOfferEntry(UserId, Offer->OfferId, Offer->Title.ToString()));

	check(PendingPurchaseDelegate.IsSet());

	// Have a pending purchase
	FOnPurchaseCheckoutComplete Delegate = MoveTemp(PendingPurchaseDelegate.GetValue());
	PendingPurchaseDelegate.Reset();
	PendingPurchaseFailTime.Reset();

	// Finish pending purchase
	Delegate.ExecuteIfBound(FOnlineError(true), MakeShared<FPurchaseReceipt>(PurchaseReceipt));
}

void FOnlinePurchaseCognito::FinalizePurchase(const FUniqueNetId& UserId, const FString& ReceiptId)
{
	const FUniqueNetIdCognito& CognitoUserId = static_cast<const FUniqueNetIdCognito&>(UserId);
	TArray<FPurchaseReceipt>* UserReceipts = UserFakeReceipts.Find(CognitoUserId);
	if (UserReceipts)
	{
		for (const FPurchaseReceipt& UserReceipt : *UserReceipts)
		{
			for (const FPurchaseReceipt::FReceiptOfferEntry& ReceiptOffer : UserReceipt.ReceiptOffers)
			{
				if (ReceiptOffer.OfferId == ReceiptId)
				{
					UE_LOG_ONLINE(Log, TEXT("Consumption of Entitlement %s completed was successful"), *ReceiptId);
					return;
				}
			}
		}
	}

	UE_LOG_ONLINE(Error, TEXT("Didn't find receipt with id %s"), *ReceiptId);
}

void FOnlinePurchaseCognito::RedeemCode(const FUniqueNetId& UserId, const FRedeemCodeRequest& RedeemCodeRequest, const FOnPurchaseRedeemCodeComplete& Delegate)
{
	TWeakPtr<FOnlinePurchaseCognito, ESPMode::ThreadSafe> WeakMe = AsShared();
	const FUniqueNetIdCognito& CognitoUserId = static_cast<const FUniqueNetIdCognito&>(UserId);

	CognitoSubsystem.ExecuteNextTick([CognitoUserId, WeakMe, RedeemCodeRequest, Delegate]
	{
		FOnlinePurchaseCognitoPtr StrongThis = WeakMe.Pin();
		if (StrongThis.IsValid())
		{
			UE_LOG_ONLINE(Log, TEXT("FOnlinePurchaseCognito::RedeemCode redeemed successfully"));

			// Cache this receipt
			TArray<FPurchaseReceipt>& UserReceipts = StrongThis->UserFakeReceipts.FindOrAdd(CognitoUserId);
			FPurchaseReceipt& PurchaseReceipt = UserReceipts.Emplace_GetRef();
			PurchaseReceipt.AddReceiptOffer(MakeReceiptOfferEntry(CognitoUserId, RedeemCodeRequest.Code, RedeemCodeRequest.Code));

			Delegate.ExecuteIfBound(FOnlineError(true), MakeShared<FPurchaseReceipt>(PurchaseReceipt));
		}
	});
}

void FOnlinePurchaseCognito::QueryReceipts(const FUniqueNetId& UserId, bool bRestoreReceipts, const FOnQueryReceiptsComplete& Delegate)
{
	const FUniqueNetIdCognito& CognitoUserId = static_cast<const FUniqueNetIdCognito&>(UserId);
	if (!CognitoUserId.IsValid())
	{
		CognitoSubsystem.ExecuteNextTick([Delegate]
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlinePurchaseCognito::QueryReceipts user is invalid"));

			Delegate.ExecuteIfBound(FOnlineError(TEXT("User is invalid")));
		});
		return;
	}

	CognitoSubsystem.ExecuteNextTick([Delegate]
	{
		Delegate.ExecuteIfBound(FOnlineError(true));
	});
}

void FOnlinePurchaseCognito::GetReceipts(const FUniqueNetId& UserId, TArray<FPurchaseReceipt>& OutReceipts) const
{
	const FUniqueNetIdCognito& CognitoUserId = static_cast<const FUniqueNetIdCognito&>(UserId);

	const TArray<FPurchaseReceipt>* FoundReceipts = UserFakeReceipts.Find(CognitoUserId);
	if (FoundReceipts == nullptr)
	{
		OutReceipts.Empty();
	}
	else
	{
		OutReceipts = *FoundReceipts;
	}
}

void FOnlinePurchaseCognito::FinalizeReceiptValidationInfo(const FUniqueNetId& UserId, FString& InReceiptValidationInfo, const FOnFinalizeReceiptValidationInfoComplete& Delegate)
{

}
