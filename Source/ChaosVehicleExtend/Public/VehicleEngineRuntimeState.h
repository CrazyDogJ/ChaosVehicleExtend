#pragma once

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

// 修复：仅在游戏线程读取 GAS；物理模块共享数值快照，不持有组件或 ASC。
struct FVehicleEngineRuntimeState
{
	TAtomic<float> Fuel{0.0f};
	TAtomic<bool> bActive{false};

	void Refresh(AActor* Owner, const FGameplayAttribute& Attribute, const FGameplayTag& ActiveTag)
	{
		auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
		bool bFound = false;
		const float Value = ASC && Attribute.IsValid() ? ASC->GetGameplayAttributeValue(Attribute, bFound) : 0.0f;
		Fuel.Store(bFound ? Value : 0.0f);
		bActive.Store(ASC && ASC->HasMatchingGameplayTag(ActiveTag));
	}
};
