// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/StreamableManager.h"

DECLARE_DELEGATE_OneParam(FDCAssetManagerStartupJobSubstepProgress, float /*NewProgress*/);

/** Handles reporting progress from streamable handles */
struct FDCAssetManagerStartupJob
{
	FDCAssetManagerStartupJobSubstepProgress SubstepProgressDelegate;

	TFunction<void(const FDCAssetManagerStartupJob&, TSharedPtr<FStreamableHandle>&)> JobFunc;

	FString JobName;
	float JobWeight;
	mutable double LastUpdate = 0;

	/** Simple job that is all synchronous */
	FDCAssetManagerStartupJob(const FString& InJobName,
	                          const TFunction<void(const FDCAssetManagerStartupJob&, TSharedPtr<FStreamableHandle>&)>&
	                          InJobFunc, float InJobWeight)
		: JobFunc(InJobFunc)
		  , JobName(InJobName)
		  , JobWeight(InJobWeight)
	{
	}

	/** Perform actual loading, will return a handle if it created one */
	TSharedPtr<FStreamableHandle> DoJob() const;

	void UpdateSubstepProgress(float NewProgress) const
	{
		SubstepProgressDelegate.ExecuteIfBound(NewProgress);
	}

	void UpdateSubstepProgressFromStreamable(TSharedRef<FStreamableHandle> StreamableHandle) const
	{
		if (SubstepProgressDelegate.IsBound())
		{
			// StreamableHandle::GetProgress traverses a large graphand is quite expensive.
			double Now = FPlatformTime::Seconds();

			// Lyra port fix: elapsed time is Now - LastUpdate.
			if (Now - LastUpdate > 1.0 / 60)
			{
				SubstepProgressDelegate.Execute(StreamableHandle->GetProgress());

				LastUpdate = Now;
			}
		}
	}
};
