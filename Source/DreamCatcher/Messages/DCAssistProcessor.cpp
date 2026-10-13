// Copyright Epic Games, Inc. All Rights Reserved.

#include "Messages/DCAssistProcessor.h"

#include "GameFramework/PlayerState.h"
#include "Messages/DCVerbMessage.h"
#include "Messages/DCVerbMessageHelpers.h"
#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DCAssistProcessor)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DC_UIProcessor_Elimination_Message, "Lyra.Elimination.Message");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DC_UIProcessor_Damage_Message, "Lyra.Damage.Message");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DC_UIProcessor_Assist_Message, "Lyra.Assist.Message");

void UDCAssistProcessor::StartListening()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	AddListenerHandle(MessageSubsystem.RegisterListener(TAG_DC_UIProcessor_Elimination_Message, this, &ThisClass::OnEliminationMessage));
	AddListenerHandle(MessageSubsystem.RegisterListener(TAG_DC_UIProcessor_Damage_Message, this, &ThisClass::OnDamageMessage));
}

void UDCAssistProcessor::OnDamageMessage(FGameplayTag Channel, const FDCVerbMessage& Payload)
{
	if (Payload.Instigator != Payload.Target)
	{
		if (APlayerState* InstigatorPS = UDCVerbMessageHelpers::GetPlayerStateFromObject(Payload.Instigator))
		{
			if (APlayerState* TargetPS = UDCVerbMessageHelpers::GetPlayerStateFromObject(Payload.Target))
			{
				FDCPlayerAssistDamageTracking& Damage = DamageHistory.FindOrAdd(TargetPS);
				float& DamageTotalFromTarget = Damage.AccumulatedDamageByPlayer.FindOrAdd(InstigatorPS);
				DamageTotalFromTarget += Payload.Magnitude;
			}
		}
	}
}


void UDCAssistProcessor::OnEliminationMessage(FGameplayTag Channel, const FDCVerbMessage& Payload)
{
	if (APlayerState* TargetPS = Cast<APlayerState>(Payload.Target))
	{
		// Grant an assist to each player who damaged the target but wasn't the instigator
		if (FDCPlayerAssistDamageTracking* DamageOnTarget = DamageHistory.Find(TargetPS))
		{
			for (const auto& KVP : DamageOnTarget->AccumulatedDamageByPlayer)
			{
				if (APlayerState* AssistPS = KVP.Key)
				{
					if (AssistPS != Payload.Instigator)
					{
						FDCVerbMessage AssistMessage;
						AssistMessage.Verb = TAG_DC_UIProcessor_Assist_Message;
						AssistMessage.Instigator = AssistPS;
						//@TODO: Get default tags from a player state or save off most recent tags during assist damage?
						//AssistMessage.InstigatorTags = ;
						AssistMessage.Target = TargetPS;
						AssistMessage.TargetTags = Payload.TargetTags;
						AssistMessage.ContextTags = Payload.ContextTags;
						AssistMessage.Magnitude = KVP.Value;

						UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
						MessageSubsystem.BroadcastMessage(AssistMessage.Verb, AssistMessage);
					}
				}
			}

			// Clear the damage log for the eliminated player
			DamageHistory.Remove(TargetPS);
		}
	}
}

