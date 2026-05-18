// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "VehicleAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

#define ATTRIBUTE_MAX_ADJUST(Attribute, NewValue, AttributeName, MaxAttributeName) \
if (Attribute == Get##MaxAttributeName##Attribute()) AdjustAttributeForMaxChange(AttributeName, MaxAttributeName, NewValue, Get##AttributeName##Attribute());

#define ATTRIBUTE_MAX_SET(Data, AttributeName, MaxAttributeName) \
float Clamped##AttributeName = FMath::Clamp(Get##AttributeName(), 0.0f, Get##MaxAttributeName()); \
if (Data.EvaluatedData.Attribute == Get##AttributeName##Attribute() && Clamped##AttributeName != Get##AttributeName()) \
{Set##AttributeName(FMath::Clamp(Get##AttributeName(), 0.0f, Get##MaxAttributeName()));}

UCLASS()
class CHAOSVEHICLEEXTEND_API UVehicleAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UVehicleAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Fuel", ReplicatedUsing = OnRep_Fuel)
	FGameplayAttributeData Fuel;
	ATTRIBUTE_ACCESSORS(ThisClass, Fuel)

	UPROPERTY(BlueprintReadOnly, Category = "Fuel", ReplicatedUsing = OnRep_MaxFuel)
	FGameplayAttributeData MaxFuel;
	ATTRIBUTE_ACCESSORS(ThisClass, MaxFuel)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
	
	UFUNCTION()
	void OnRep_Fuel(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Fuel, OldValue); }
	UFUNCTION()
	void OnRep_MaxFuel(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxFuel, OldValue); }
};
