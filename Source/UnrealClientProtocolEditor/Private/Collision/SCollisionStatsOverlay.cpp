// MIT License - Copyright (c) 2025 Italink

#include "SCollisionStatsOverlay.h"
#include "Collision/PneumaViewCollision.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Widgets/Layout/SBorder.h"

void SCollisionStatsOverlay::Construct(const FArguments& InArgs)
{
	CollisionViewWeak = InArgs._CollisionView;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(8.0f)
		[
			SAssignNew(StatsText, STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
			.ColorAndOpacity(FLinearColor::White)
			.Text(FText::FromString(TEXT("Waiting...")))
		]
	];
}

void SCollisionStatsOverlay::RefreshStats()
{
	if (!StatsText.IsValid()) return;

	UPneumaViewCollision* View = CollisionViewWeak.Get();
	if (!View)
	{
		StatsText->SetText(FText::FromString(TEXT("No data")));
		return;
	}

	auto GetMeshStats = [](AActor* Actor) -> FString
	{
		if (!Actor) return TEXT("  (none)");
		AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(Actor);
		if (!SMActor) return TEXT("  (none)");
		UStaticMeshComponent* Comp = SMActor->GetStaticMeshComponent();
		if (!Comp || !Comp->GetStaticMesh()) return TEXT("  (none)");

		UStaticMesh* SM = Comp->GetStaticMesh();
		const FStaticMeshRenderData* RD = SM->GetRenderData();
		if (!RD || RD->LODResources.Num() == 0) return TEXT("  (no render data)");

		const FStaticMeshLODResources& LOD0 = RD->LODResources[0];
		int32 Verts = LOD0.GetNumVertices();
		int32 Tris = LOD0.GetNumTriangles();
		FBox Bounds = SM->GetBoundingBox();
		FVector Size = Bounds.GetSize() / 100.0;

		return FString::Printf(TEXT("  Verts: %s\n  Tris:  %s\n  Size:  %.2fx%.2fx%.2f m"),
			*FText::AsNumber(Verts).ToString(),
			*FText::AsNumber(Tris).ToString(),
			Size.X, Size.Y, Size.Z);
	};

	FString SourceStats = GetMeshStats(View->GetSourceActor());
	FString CollisionStats = GetMeshStats(View->GetCollisionActor());
	FString Ratio = TEXT("--");

	AStaticMeshActor* SrcActor = Cast<AStaticMeshActor>(View->GetSourceActor());
	AStaticMeshActor* ColActor = Cast<AStaticMeshActor>(View->GetCollisionActor());
	if (SrcActor && ColActor)
	{
		UStaticMesh* SrcMesh = SrcActor->GetStaticMeshComponent()->GetStaticMesh();
		UStaticMesh* ColMesh = ColActor->GetStaticMeshComponent()->GetStaticMesh();
		if (SrcMesh && ColMesh && SrcMesh->GetRenderData() && ColMesh->GetRenderData()
			&& SrcMesh->GetRenderData()->LODResources.Num() > 0
			&& ColMesh->GetRenderData()->LODResources.Num() > 0)
		{
			int32 SrcTris = SrcMesh->GetRenderData()->LODResources[0].GetNumTriangles();
			int32 ColTris = ColMesh->GetRenderData()->LODResources[0].GetNumTriangles();
			if (SrcTris > 0)
			{
				Ratio = FString::Printf(TEXT("%.2f%%"), 100.0 * ColTris / SrcTris);
			}
		}
	}

	FString Display = FString::Printf(TEXT(
		"Original\n%s\n\n"
		"Collision\n%s\n"
		"  Ratio: %s"),
		*SourceStats, *CollisionStats, *Ratio);

	StatsText->SetText(FText::FromString(Display));
}
