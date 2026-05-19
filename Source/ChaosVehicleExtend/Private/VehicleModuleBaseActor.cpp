// Fill out your copyright notice in the Description page of Project Settings.

#include "VehicleModuleBaseActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Net/UnrealNetwork.h"

AVehicleModuleBaseActor::AVehicleModuleBaseActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;

	RootMeshComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("RootMesh"));
	RootMeshComponent->SetSimulatePhysics(false);
}

void AVehicleModuleBaseActor::SetAttachmentData(const FVehicleModuleAttachment& InAttachmentData)
{
	AttachmentData = InAttachmentData;
	OnRep_AttachmentData();
}

void AVehicleModuleBaseActor::OnRep_AttachmentData()
{
	if (AttachmentData.PrimitiveComponent)
	{
		const FAttachmentTransformRules AttachRules(
			AttachmentData.AttachmentRule,
			AttachmentData.AttachmentRule,
			AttachmentData.AttachmentRule,
			true);
		AttachToComponent(AttachmentData.PrimitiveComponent, AttachRules, AttachmentData.Socket);
		RootMeshComponent->SetSimulatePhysics(true);
	}
}

UAbilitySystemComponent* AVehicleModuleBaseActor::GetAbilitySystemComponent() const
{
	if (const auto AttachedActor = GetAttachParentActor())
	{
		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AttachedActor);
	}

	return nullptr;
}

void AVehicleModuleBaseActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, AttachmentData);
}
