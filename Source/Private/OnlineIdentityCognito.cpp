// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineIdentityCognito.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/OutputDeviceRedirector.h"
#include "OnlineSubsystemCognito.h"
#include "IPAddress.h"
#include "SocketSubsystem.h"
#include "OnlineError.h"
#include "BAUsers.h"
#include "BAUser.h"
#include "Misc/DefaultValueHelper.h"
#include "Engine/Engine.h"
#include "BAMultiplayerCoreTypes.h"
#include "BAIdentitySubsystem.h"
#include "BAMultiplayerBridge.h"
#include "BAMultiplayerCoreTypes.h"
THIRD_PARTY_INCLUDES_START
#include "aws/cognito-idp/model/InitiateAuthRequest.h"
#include "aws/core/utils/memory/stl/AWSMap.h"
#include "aws/core/utils/memory/stl/AWSString.h"
#include "aws/cognito-idp/model/ConfirmSignUpRequest.h"
#include "aws/cognito-idp/CognitoIdentityProviderErrors.h"
#include "aws/core/client/ClientConfiguration.h"
#include "aws/core/utils/memory/stl/AWSAllocator.h"
#include "aws/core/Aws.h"
#include "aws/cognito-idp/model/SignUpRequest.h"
#include "BAProfile.h"
THIRD_PARTY_INCLUDES_END

const char APP_CLIENT_ID_OSS_IDENTITY[] = "7ae646n7b1k6qkpkjk62sdtc5g";
const char* REGION_OSS_IDENTITY = Aws::Region::EU_CENTRAL_1;

FOnlineIdentityCognito::FOnlineIdentityCognito(FOnlineSubsystemCognito* InSubsystem) : CognitoSubsystem(InSubsystem)
{
	Aws::Utils::Logging::LogLevel logLevel{ Aws::Utils::Logging::LogLevel::Error };
	//options.loggingOptions.logger_create_fn = [logLevel] {return std:make_shared<Aws::Utils::Logging::ConsoleLogSystem>(logLevel); };
	Aws::InitAPI(AWSOptions);
	Aws::Client::ClientConfiguration AWSClientConfiguration;
	AWSClientConfiguration.region = REGION_OSS_IDENTITY;    // region must be set for Cognito operations
	s_AmazonCognitoClient = Aws::MakeShared<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient>("CognitoIdentityProviderClient", AWSClientConfiguration);
}

FOnlineIdentityCognito::~FOnlineIdentityCognito()
{
}



bool FUserOnlineAccountCognito::GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundAttr = AdditionalAuthData.Find(AttrName);
	if (FoundAttr != NULL)
	{
		OutAttrValue = *FoundAttr;
		return true;
	}
	return false;
}

bool FUserOnlineAccountCognito::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundAttr = UserAttributes.Find(AttrName);
	if (FoundAttr != NULL)
	{
		OutAttrValue = *FoundAttr;
		return true;
	}
	return false;
}

bool FUserOnlineAccountCognito::SetUserAttribute(const FString& AttrName, const FString& AttrValue)
{
	const FString* FoundAttr = UserAttributes.Find(AttrName);
	if (FoundAttr == NULL || *FoundAttr != AttrValue)
	{
		UserAttributes.Add(AttrName, AttrValue);
		return true;
	}
	return false;
}

bool FOnlineIdentityCognito::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{

	// valid local player index

	if (LocalUserNum < 0 || LocalUserNum >= MAX_LOCAL_PLAYERS)
	{
		FString ErrorStr = FString::Printf(TEXT("Invalid LocalUserNum=%d"), LocalUserNum);
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Login request failed. %s"), *ErrorStr);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdCognito(), ErrorStr);
		return false;
	}

	Aws::CognitoIdentityProvider::Model::InitiateAuthRequest initiateAuthRequest;
	initiateAuthRequest.SetClientId(APP_CLIENT_ID_OSS_IDENTITY);
	initiateAuthRequest.SetAuthFlow(Aws::CognitoIdentityProvider::Model::AuthFlowType::USER_PASSWORD_AUTH);

	const FString Username = AccountCredentials.Id;
	const FString Password = AccountCredentials.Token;

	Aws::Map<Aws::String, Aws::String> authParameters
	{
		{"USERNAME", TCHAR_TO_UTF8(*Username)},
		{"PASSWORD", TCHAR_TO_UTF8(*Password)}
	};

	initiateAuthRequest.SetAuthParameters(authParameters);

	Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome initiateAuthOutcome{ s_AmazonCognitoClient->InitiateAuth(initiateAuthRequest) };

	FBALoginResponse Response;
	Response.bIsSuccess = initiateAuthOutcome.IsSuccess();
	Response.ErrorMsg = UTF8_TO_TCHAR(initiateAuthOutcome.GetError().GetMessage().c_str());

	FUniqueNetIdCognito NewUserId;

	if (initiateAuthOutcome.IsSuccess())
	{
		Aws::CognitoIdentityProvider::Model::InitiateAuthResult initiateAuthResult{ initiateAuthOutcome.GetResult() };
		if (initiateAuthResult.GetChallengeName() == Aws::CognitoIdentityProvider::Model::ChallengeNameType::NOT_SET)
		{
			Aws::CognitoIdentityProvider::Model::AuthenticationResultType authenticationResult = initiateAuthResult.GetAuthenticationResult();

			UE_LOG(LogTemp, Log, TEXT("UBAAwsIdentitySubsystem::Login Results=  TokenType:%s, AccessToken:%s, ExpiresIn:%i, IdToken:%s, RefreshToken:%s "),
				UTF8_TO_TCHAR(authenticationResult.GetTokenType().c_str()),
				UTF8_TO_TCHAR(authenticationResult.GetAccessToken().c_str()),
				authenticationResult.GetExpiresIn(),
				UTF8_TO_TCHAR(authenticationResult.GetIdToken().c_str()),
				UTF8_TO_TCHAR(authenticationResult.GetRefreshToken().c_str())
			);

			UBAUser* User = NewObject<UBAUser>();
			Response.LoggedUser = User;
			User->UserID = 1;//TODO: See if we can have an actual value
			User->Username = Username;
			User->Password = Password;

			UBAProfile* Profile = NewObject<UBAProfile>();
			Profile->BAUser = User;
			Profile->ID = User->UserID;
			Profile->ProfileName = User->Username;

			User->Profiles.Add(Profile);
			User->MainProfile = Profile;

			{
				const FString& UniqueIdString = Response.LoggedUser->GetBAUniqueID();
				NewUserId = FUniqueNetIdCognito(UniqueIdString);
				TSharedPtr<FUserOnlineAccountCognito> UserAccountPtr = MakeShareable(new FUserOnlineAccountCognito(UniqueIdString));
				UserAccountPtr->UserAttributes.Add(USER_ATTR_ID, UniqueIdString);
				UserAccountPtr->BAUser = User;

				// update/add cached entry for user
				UserAccounts.Add(NewUserId, UserAccountPtr.ToSharedRef());

				// keep track of user ids for local users
				UserIds.Add(Response.LocalUserNum, UserAccountPtr->GetUserId());
			}

			//s_IsLoggedIn = true;
			//s_TokenType = authenticationResult.GetTokenType();
			//s_AccessToken = authenticationResult.GetAccessToken();
			//s_IDToken = authenticationResult.GetIdToken();
			//s_RefreshToken = authenticationResult.GetRefreshToken();
		}
	}
	else
	{
		NewUserId = FUniqueNetIdCognito();
		UE_LOG(LogTemp, Log, TEXT("UBAAwsIdentitySubsystem::Login Error: %s"), *Response.ErrorMsg);
	}

	TriggerOnLoginCompleteDelegates(LocalUserNum, Response.bIsSuccess, NewUserId, *Response.ErrorMsg);

	return Response.bIsSuccess;
}

bool FOnlineIdentityCognito::Logout(int32 LocalUserNum)
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		// remove cached user account
		UserAccounts.Remove(FUniqueNetIdCognito(*UserId));
		// remove cached user id
		UserIds.Remove(LocalUserNum);
		// not async but should call completion delegate anyway
		TriggerOnLogoutCompleteDelegates(LocalUserNum, true);

		return true;
	}
	else
	{
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("No logged in user found for LocalUserNum=%d."),
			LocalUserNum);
		TriggerOnLogoutCompleteDelegates(LocalUserNum, false);
	}
	return false;
}

bool FOnlineIdentityCognito::AutoLogin(int32 LocalUserNum)
{
	FString LoginStr;
	FString PasswordStr;
	FString TypeStr;

	FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), LoginStr);
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_PASSWORD="), PasswordStr);
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), TypeStr);

	bool bEnableWarning = LoginStr.Len() > 0 || PasswordStr.Len() > 0 || TypeStr.Len() > 0;

	if (!LoginStr.IsEmpty())
	{
		if (!PasswordStr.IsEmpty())
		{
			if (!TypeStr.IsEmpty())
			{
				return Login(0, FOnlineAccountCredentials(TypeStr, LoginStr, PasswordStr));
			}
			else if (bEnableWarning)
			{
				UE_LOG_ONLINE_IDENTITY(Warning, TEXT("AutoLogin missing AUTH_TYPE=<type>."));
			}
		}
		else if (bEnableWarning)
		{
			UE_LOG_ONLINE_IDENTITY(Warning, TEXT("AutoLogin missing AUTH_PASSWORD=<password>."));
		}
	}
	else if (bEnableWarning)
	{
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("AutoLogin missing AUTH_LOGIN=<login id>."));
	}

	return false;
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityCognito::GetUserAccount(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> Result;

	FUniqueNetIdCognito StringUserId(UserId);
	const TSharedRef<FUserOnlineAccountCognito>* FoundUserAccount = UserAccounts.Find(StringUserId);
	if (FoundUserAccount != NULL)
	{
		Result = *FoundUserAccount;
	}

	return Result;
}

TArray<TSharedPtr<FUserOnlineAccount> > FOnlineIdentityCognito::GetAllUserAccounts() const
{
	TArray<TSharedPtr<FUserOnlineAccount>> Result;

	for (TMap<FUniqueNetIdCognito, TSharedRef<FUserOnlineAccountCognito>>::TConstIterator It(UserAccounts); It; ++It)
	{
		Result.Add(It.Value());
	}

	return Result;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityCognito::GetUniquePlayerId(int32 LocalUserNum) const
{
	const TSharedPtr<const FUniqueNetId>* FoundId = UserIds.Find(LocalUserNum);
	if (FoundId != NULL)
	{
		return *FoundId;
	}
	return NULL;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityCognito::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	if (Bytes != NULL && Size > 0)
	{
		FString StrId(Size, (TCHAR*)Bytes);
		return MakeShareable(new FUniqueNetIdCognito(StrId));
	}
	return NULL;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityCognito::CreateUniquePlayerId(const FString& Str)
{
	return MakeShareable(new FUniqueNetIdCognito(Str));
}

ELoginStatus::Type FOnlineIdentityCognito::GetLoginStatus(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		return GetLoginStatus(*UserId);
	}
	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FOnlineIdentityCognito::GetLoginStatus(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(UserId);
	if (UserAccount.IsValid() &&
		UserAccount->GetUserId()->IsValid())
	{
		return ELoginStatus::LoggedIn;
	}
	return ELoginStatus::NotLoggedIn;
}

FString FOnlineIdentityCognito::GetPlayerNickname(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UniqueId = GetUniquePlayerId(LocalUserNum);
	if (UniqueId.IsValid())
	{
		return UniqueId->ToString();
	}

	return TEXT("CognitoUser");
}

FString FOnlineIdentityCognito::GetPlayerNickname(const FUniqueNetId& UserId) const
{
	return UserId.ToString();
}

FString FOnlineIdentityCognito::GetAuthToken(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(*UserId);
		if (UserAccount.IsValid())
		{
			return UserAccount->GetAccessToken();
		}
	}
	return FString();
}

void FOnlineIdentityCognito::RevokeAuthToken(const FUniqueNetId& UserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate)
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityCognito::RevokeAuthToken not implemented"));
	TSharedRef<const FUniqueNetId> UserIdRef(UserId.AsShared());
	CognitoSubsystem->ExecuteNextTick([UserIdRef, Delegate]()
		{
			Delegate.ExecuteIfBound(*UserIdRef, FOnlineError(FString(TEXT("RevokeAuthToken not implemented"))));
		});
}


void FOnlineIdentityCognito::GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(UserId, Privilege, (uint32)EPrivilegeResults::NoFailures);
}

FPlatformUserId FOnlineIdentityCognito::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId) const
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
	{
		auto CurrentUniqueId = GetUniquePlayerId(i);
		if (CurrentUniqueId.IsValid() && (*CurrentUniqueId == UniqueNetId))
		{
			return i;
		}
	}

	return PLATFORMUSERID_NONE;
}

FString FOnlineIdentityCognito::GetAuthType() const
{
	return TEXT("");
}

void FOnlineIdentityCognito::BAOnloginCompleted(const FBALoginResponse& Response)
{

	if (Response.bIsSuccess && Response.LoggedUser)
	{
		const FString& UniqueId = Response.LoggedUser->GetBAUniqueID();
		TSharedPtr<FUserOnlineAccountCognito> UserAccountPtr;
		FUniqueNetIdCognito NewUserId(UniqueId);

		UserAccountPtr = MakeShareable(new FUserOnlineAccountCognito(UniqueId));
		UserAccountPtr->UserAttributes.Add(USER_ATTR_ID, UniqueId);

		// update/add cached entry for user
		UserAccounts.Add(NewUserId, UserAccountPtr.ToSharedRef());

		// keep track of user ids for local users
		UserIds.Add(Response.LocalUserNum, UserAccountPtr->GetUserId());
		TriggerOnLoginCompleteDelegates(Response.LocalUserNum, Response.bIsSuccess, *UserAccountPtr->GetUserId(), *Response.ErrorMsg);
	}
	else
	{
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Login request failed. %s"), *Response.ErrorMsg);
		TriggerOnLoginCompleteDelegates(Response.LocalUserNum, Response.bIsSuccess, FUniqueNetIdCognito(), *Response.ErrorMsg);
	}
}

TSharedPtr<FUniqueNetId> FOnlineIdentityCognito::BAMakeSharedUniqueID(const FUniqueNetId& PlayerId)
{
	return MakeShareable<FUniqueNetId>(new FUniqueNetIdCognito(PlayerId));
}


void FOnlineIdentityCognito::SignUp(const FString& Email, const FString& Username, const FString& Password, const FString& RepeatPassword)
{

	if (Password != RepeatPassword)
	{
		FBASignedUpResponse Response;
		Response.Errors.Add(TEXT("Password and RepeatPassword mismatch"));
		OnSignUpResponse.Broadcast(Response);
		return;
	}

	Aws::CognitoIdentityProvider::Model::SignUpRequest signUpRequest;

	signUpRequest.SetClientId(APP_CLIENT_ID_OSS_IDENTITY);
	signUpRequest.SetUsername(TCHAR_TO_UTF8(*Username));
	signUpRequest.SetPassword(TCHAR_TO_UTF8(*Password));

	// note that options, like the e-mail address requirement, are stored in an attributes vector
	// not exposed through the request like required fields.
	Aws::Vector<Aws::CognitoIdentityProvider::Model::AttributeType> attributes;
	Aws::CognitoIdentityProvider::Model::AttributeType emailAttribute;

	emailAttribute.SetName("email");
	emailAttribute.SetValue(TCHAR_TO_UTF8(*Email));
	attributes.push_back(emailAttribute);

	Aws::CognitoIdentityProvider::Model::AttributeType nicknameAttribute;
	nicknameAttribute.SetName("nickname");
	nicknameAttribute.SetValue(TCHAR_TO_UTF8(*Username));
	attributes.push_back(nicknameAttribute);

	signUpRequest.SetUserAttributes(attributes);

	Aws::CognitoIdentityProvider::Model::SignUpOutcome signUpOutcome{ s_AmazonCognitoClient->SignUp(signUpRequest) };

	FBASignedUpResponse Response;
	Response.bIsSuccess = signUpOutcome.IsSuccess();

	if (signUpOutcome.IsSuccess())
	{
		Aws::CognitoIdentityProvider::Model::SignUpResult signUpResult{ signUpOutcome.GetResult() };

		UE_LOG(LogTemp, Log, TEXT("UBAAwsIdentitySubsystem::Login Results=  %sIsSuccess: %s%sGetUserConfirmed:%s%sGetUserSub:%s"),
			LINE_TERMINATOR,
			signUpOutcome.IsSuccess() ? TEXT("True") : TEXT("False"),
			LINE_TERMINATOR,
			signUpResult.GetUserConfirmed() ? TEXT("True") : TEXT("False"),
			LINE_TERMINATOR,
			UTF8_TO_TCHAR(signUpResult.GetUserSub().c_str())
		);
	}
	else
	{
		Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = signUpOutcome.GetError();
		Response.ErrorMsg = error.GetMessage().c_str();

		UE_LOG(LogTemp, Log, TEXT("UBAAwsIdentitySubsystem::Login Results=  %sIsSuccess: %s%sErrorMsg:%s"),
			LINE_TERMINATOR,
			signUpOutcome.IsSuccess() ? TEXT("True") : TEXT("False"),
			LINE_TERMINATOR,
			*Response.ErrorMsg
		);
	}

	OnSignUpResponse.Broadcast(Response);
}


void FOnlineIdentityCognito::VerifyCode(const FString& Username, const FString& ConfirmationCode)
{

	Aws::CognitoIdentityProvider::Model::ConfirmSignUpRequest confirmSignupRequest;
	confirmSignupRequest.SetClientId(APP_CLIENT_ID_OSS_IDENTITY);
	confirmSignupRequest.SetUsername(TCHAR_TO_UTF8(*Username));
	confirmSignupRequest.SetConfirmationCode(TCHAR_TO_UTF8(*ConfirmationCode));

	Aws::CognitoIdentityProvider::Model::ConfirmSignUpOutcome confirmSignupOutcome{ s_AmazonCognitoClient->ConfirmSignUp(confirmSignupRequest) };

	FBASignedUpVerificationResponse Response;

	Response.bIsSuccess = confirmSignupOutcome.IsSuccess();

	if (Response.bIsSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("UBAAwsIdentitySubsystem::Login Confirmation succeeded!"));
	}
	else
	{
		Aws::Client::AWSError<Aws::CognitoIdentityProvider::CognitoIdentityProviderErrors> error = confirmSignupOutcome.GetError();

		Response.ErrorMsg = error.GetMessage().c_str();
	}

	OnSignUpVerificationResponse.Broadcast(Response);
}
