// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemCognitoTypes.h"
#include "NboSerializer.h"

/**
 * Serializes data in network byte order form into a buffer
 */
class FNboSerializeToBufferCognito : public FNboSerializeToBuffer
{
public:
	/** Default constructor zeros num bytes*/
	FNboSerializeToBufferCognito() :
		FNboSerializeToBuffer(512)
	{
	}

	/** Constructor specifying the size to use */
	FNboSerializeToBufferCognito(uint32 Size) :
		FNboSerializeToBuffer(Size)
	{
	}

	/**
	 * Adds Cognito session info to the buffer
	 */
 	friend inline FNboSerializeToBufferCognito& operator<<(FNboSerializeToBufferCognito& Ar, const FOnlineSessionInfoCognito& SessionInfo)
 	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar << SessionInfo.SessionId;
		Ar << *SessionInfo.HostAddr;
		return Ar;
 	}

	/**
	 * Adds Cognito Unique Id to the buffer
	 */
	friend inline FNboSerializeToBufferCognito& operator<<(FNboSerializeToBufferCognito& Ar, const FUniqueNetIdCognito& UniqueId)
	{
		Ar << UniqueId.UniqueNetIdStr;
		return Ar;
	}
};

/**
 * Class used to write data into packets for sending via system link
 */
class FNboSerializeFromBufferCognito : public FNboSerializeFromBuffer
{
public:
	/**
	 * Initializes the buffer, size, and zeros the read offset
	 */
	FNboSerializeFromBufferCognito(uint8* Packet,int32 Length) :
		FNboSerializeFromBuffer(Packet,Length)
	{
	}

	/**
	 * Reads Cognito session info from the buffer
	 */
 	friend inline FNboSerializeFromBufferCognito& operator>>(FNboSerializeFromBufferCognito& Ar, FOnlineSessionInfoCognito& SessionInfo)
 	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar >> SessionInfo.SessionId; 
		Ar >> *SessionInfo.HostAddr;
		return Ar;
 	}

	/**
	 * Reads Cognito Unique Id from the buffer
	 */
	friend inline FNboSerializeFromBufferCognito& operator>>(FNboSerializeFromBufferCognito& Ar, FUniqueNetIdCognito& UniqueId)
	{
		Ar >> UniqueId.UniqueNetIdStr;
		return Ar;
	}
};
