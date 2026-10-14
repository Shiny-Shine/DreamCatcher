#include "Input/DCInputConfig.h"

#include "DreamCatcher.h"
#include "InputAction.h"

UDCInputConfig::UDCInputConfig(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

const UInputAction* UDCInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FDCInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && Action.InputTag.MatchesTagExact(InputTag))
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("InputConfig [%s] cannot find a native ""InputAction for tag [%s]."),
		       *GetNameSafe(this), *InputTag.ToString());
	}

	return nullptr;
}

const UInputAction* UDCInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FDCInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.MatchesTagExact(InputTag))
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogDreamCatcher, Error, TEXT("InputConfig [%s] cannot find an ability ""InputAction for tag [%s]."),
		       *GetNameSafe(this), *InputTag.ToString());
	}

	return nullptr;
}
