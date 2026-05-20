// Fill out your copyright notice in the Description page of Project Settings.


#include "GeometryCollectionRootProxyOverlayRenderer.h"

#include "GeometryCollection/GeometryCollectionComponent.h"

void UGeometryCollectionRootProxyOverlayRenderer::OnRegisterGeometryCollection(UGeometryCollectionComponent& InComponent)
{
	CreateRootProxyComponents(InComponent);
}

void UGeometryCollectionRootProxyOverlayRenderer::OnUnregisterGeometryCollection()
{
	ClearRootProxyComponents();
}

void UGeometryCollectionRootProxyOverlayRenderer::UpdateState(UGeometryCollection const& InGeometryCollection, FTransform const& InComponentTransform, uint32 InStateFlags)
{
	const bool bIsStateVisible = (InStateFlags & EState_Visible) != 0;
	const bool bIsStateBroken = (InStateFlags & EState_Broken) != 0;
	const bool bSetVisible = !bIsStateBroken && bIsStateVisible;
	
	if (bIsVisible != bSetVisible)
	{
		bIsVisible = bSetVisible;

		for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
		{
			if (StaticMeshComponent)
			{
				StaticMeshComponent->SetVisibility(bIsVisible);
			}
		}
	}
}

void UGeometryCollectionRootProxyOverlayRenderer::UpdateRootTransform(UGeometryCollection const& InGeometryCollection, FTransform const& InRootTransform)
{
	UpdateRootProxyTransforms(InGeometryCollection, InRootTransform, {});
}

void UGeometryCollectionRootProxyOverlayRenderer::UpdateRootTransforms(UGeometryCollection const& InGeometryCollection, FTransform const& InRootTransform, TArrayView<const FTransform3f> InRootTransforms)
{
	UpdateRootProxyTransforms(InGeometryCollection, InRootTransform, InRootTransforms);
}

void UGeometryCollectionRootProxyOverlayRenderer::UpdateTransforms(UGeometryCollection const& InGeometryCollection, TArrayView<const FTransform3f> InTransforms)
{
	// Don't support non root proxy mesh.
}

void UGeometryCollectionRootProxyOverlayRenderer::CreateRootProxyComponents(UGeometryCollectionComponent& InComponent)
{
	UGeometryCollection const* GeometryCollection = InComponent.GetRestCollection();
	if (GeometryCollection == nullptr)
	{
		return;
	}
	ClearRootProxyComponents();
	StaticMeshComponents.Reserve(GeometryCollection->RootProxyData.ProxyMeshes.Num());

	for (UStaticMesh* ProxyMesh : GeometryCollection->RootProxyData.ProxyMeshes)
	{
		UStaticMeshComponent* MeshComponent = nullptr;
		
		if (ProxyMesh != nullptr)
		{
			MeshComponent = NewObject<UStaticMeshComponent>(this, NAME_None, RF_DuplicateTransient | RF_Transient);

			MeshComponent->SetStaticMesh(ProxyMesh);
			MeshComponent->SetCanEverAffectNavigation(false);
			MeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
			MeshComponent->SetMobility(InComponent.Mobility);
			MeshComponent->SetupAttachment(&InComponent);
			MeshComponent->RegisterComponent();
			MeshComponent->SetOverlayMaterial(InComponent.OverlayMaterial);
			MeshComponent->SetOverlayMaterialMaxDrawDistance(InComponent.OverlayMaterialMaxDrawDistance);
			MeshComponent->MaterialSlotsOverlayMaterial = InComponent.MaterialSlotsOverlayMaterial;
		}

		StaticMeshComponents.Add(MeshComponent);
	}
}

void UGeometryCollectionRootProxyOverlayRenderer::UpdateRootProxyTransforms(UGeometryCollection const& InGeometryCollection, FTransform const& InRootTransform, TArrayView<const FTransform3f> InLocalRootTransforms)
{
	for (int32 MeshIndex = 0; MeshIndex < StaticMeshComponents.Num(); MeshIndex++)
	{
		if (UStaticMeshComponent* MeshComponent = StaticMeshComponents[MeshIndex])
		{
			if (InLocalRootTransforms.IsValidIndex(MeshIndex))
			{
				MeshComponent->SetRelativeTransform(FTransform(InLocalRootTransforms[MeshIndex]) * InRootTransform);
			}
			else
			{
				MeshComponent->SetRelativeTransform(InRootTransform);
			}
		}
	}
}

void UGeometryCollectionRootProxyOverlayRenderer::ClearRootProxyComponents()
{
	for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		if (StaticMeshComponent)
		{
			StaticMeshComponent->DestroyComponent();
		}
	}
	StaticMeshComponents.Reset();
}