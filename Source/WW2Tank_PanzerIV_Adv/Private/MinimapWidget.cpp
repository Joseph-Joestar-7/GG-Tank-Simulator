// MinimapWidget.cpp
#include "MinimapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Slate/WidgetTransform.h"
#include "Styling/CoreStyle.h"

namespace
{
	UOverlaySlot* AddCentered(UOverlay* Overlay, UWidget* Child)
	{
		UOverlaySlot* S = Overlay->AddChildToOverlay(Child);
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
		return S;
	}
}

TSharedRef<SWidget> UMinimapWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UMinimapWidget::BuildLayout()
{
	USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MinimapRoot"));
	Root->SetWidthOverride(WidgetSize);
	Root->SetHeightOverride(WidgetSize);
	WidgetTree->RootWidget = Root;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MinimapOverlay"));
	Root->AddChild(Overlay);

	// 1) Map (material with circle mask)
	UImage* MapImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MapImage"));
	if (MinimapMaterial)
	{
		MapMID = UMaterialInstanceDynamic::Create(MinimapMaterial, this);
		if (MapTexture)
		{
			MapMID->SetTextureParameterValue(TEXT("MapTexture"), MapTexture);
		}
		MapImage->SetBrushFromMaterial(MapMID);
	}
	UOverlaySlot* MapSlot = Overlay->AddChildToOverlay(MapImage);
	MapSlot->SetHorizontalAlignment(HAlign_Fill);
	MapSlot->SetVerticalAlignment(VAlign_Fill);

	// 2) Ring / frame
	if (RingTexture)
	{
		UImage* Ring = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RingImage"));
		Ring->SetBrushFromTexture(RingTexture, false);
		UOverlaySlot* RingSlot = Overlay->AddChildToOverlay(Ring);
		RingSlot->SetHorizontalAlignment(HAlign_Fill);
		RingSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// 3) Marker layer (markers are added to this at runtime)
	MarkerLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MarkerLayer"));
	UOverlaySlot* LayerSlot = Overlay->AddChildToOverlay(MarkerLayer);
	LayerSlot->SetHorizontalAlignment(HAlign_Fill);
	LayerSlot->SetVerticalAlignment(VAlign_Fill);

	// 4) Player arrow (center)
	PlayerArrow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PlayerArrow"));
	if (PlayerArrowTexture)
	{
		PlayerArrow->SetBrushFromTexture(PlayerArrowTexture, false);
	}
	else
	{
		PlayerArrow->SetBrush(FSlateColorBrush(FLinearColor::White));
	}
	PlayerArrow->SetColorAndOpacity(PlayerColor);
	PlayerArrow->SetDesiredSizeOverride(FVector2D(PlayerArrowSize));
	AddCentered(Overlay, PlayerArrow);

	// 5) Compass letters
	CompassLabels.Reset();
	if (bShowCompass)
	{
		const TCHAR* Letters[4] = { TEXT("N"), TEXT("E"), TEXT("S"), TEXT("W") };
		for (int32 i = 0; i < 4; ++i)
		{
			UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			T->SetText(FText::FromString(Letters[i]));
			T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
			T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			T->SetShadowOffset(FVector2D(1.f, 1.f));
			AddCentered(Overlay, T);
			CompassLabels.Add(T);
		}
	}
}

int32 UMinimapWidget::AddActorMarker(AActor* Target, UTexture2D* Icon, FLinearColor Tint, float Size, bool bClampToEdge)
{
	return AddMarkerInternal(Target, FVector::ZeroVector, Icon, Tint, Size, bClampToEdge);
}

int32 UMinimapWidget::AddLocationMarker(FVector WorldLocation, UTexture2D* Icon, FLinearColor Tint, float Size, bool bClampToEdge)
{
	return AddMarkerInternal(nullptr, WorldLocation, Icon, Tint, Size, bClampToEdge);
}

int32 UMinimapWidget::AddMarkerInternal(AActor* Target, const FVector& Loc, UTexture2D* Icon, const FLinearColor& Tint, float Size, bool bClamp)
{
	if (!MarkerLayer || !WidgetTree)
	{
		return INDEX_NONE; // widget not built yet: call after AddToViewport
	}

	UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	FMarker M;
	M.bDiamond = (Icon == nullptr);
	if (Icon)
	{
		Img->SetBrushFromTexture(Icon, false);
	}
	else
	{
		Img->SetBrush(FSlateColorBrush(FLinearColor::White));
	}
	Img->SetColorAndOpacity(Tint);
	Img->SetDesiredSizeOverride(FVector2D(Size));
	AddCentered(MarkerLayer, Img);

	M.Handle = NextHandle++;
	M.Actor = Target;
	M.bUseActor = (Target != nullptr);
	M.StaticLocation = Loc;
	M.bClamp = bClamp;
	M.Image = Img;
	Markers.Add(M);
	return M.Handle;
}

void UMinimapWidget::RemoveMarker(int32 Handle)
{
	for (int32 i = 0; i < Markers.Num(); ++i)
	{
		if (Markers[i].Handle == Handle)
		{
			if (Markers[i].Image)
			{
				Markers[i].Image->RemoveFromParent();
			}
			Markers.RemoveAtSwap(i);
			return;
		}
	}
}

void UMinimapWidget::ClearMarkers()
{
	for (FMarker& M : Markers)
	{
		if (M.Image)
		{
			M.Image->RemoveFromParent();
		}
	}
	Markers.Reset();
}

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn)
	{
		return;
	}

	const FVector PlayerLoc = Pawn->GetActorLocation();

	// Camera yaw if available (good for 3rd person), else control yaw.
	float PlayerYawDeg = 0.f;
	if (APlayerCameraManager* Cam = GetOwningPlayerCameraManager())
	{
		PlayerYawDeg = Cam->GetCameraRotation().Yaw;
	}
	else if (APlayerController* PC = GetOwningPlayer())
	{
		PlayerYawDeg = PC->GetControlRotation().Yaw;
	}
	const float PlayerYawRad = FMath::DegreesToRadians(PlayerYawDeg);

	// Map is either rotated by yaw, or fixed north-up.
	const float MapYawDeg = bRotateWithPlayer ? PlayerYawDeg : 0.f;
	const float MapYawRad = FMath::DegreesToRadians(MapYawDeg);

	// --- Material ---
	if (MapMID)
	{
		const float U = (PlayerLoc.Y - WorldOrigin.Y) / WorldSize;
		const float V = 1.f - (PlayerLoc.X - WorldOrigin.X) / WorldSize;
		MapMID->SetVectorParameterValue(TEXT("PlayerUV"), FLinearColor(U, V, 0.f, 0.f));
		MapMID->SetScalarParameterValue(TEXT("Zoom"), Zoom);
		MapMID->SetScalarParameterValue(TEXT("Rotation"), MapYawRad);
	}

	// --- Player arrow ---
	if (PlayerArrow)
	{
		// Rotates only when the map is fixed.
		const float ArrowAngle = bRotateWithPlayer ? 0.f : PlayerYawDeg;
		PlayerArrow->SetRenderTransformAngle(ArrowAngle);
	}

	const float Radius = WidgetSize * 0.5f;
	const float S = FMath::Sin(MapYawRad);
	const float C = FMath::Cos(MapYawRad);

	// --- Compass letters ---
	const float LetterRadius = Radius - CompassInset;
	for (int32 i = 0; i < CompassLabels.Num(); ++i)
	{
		const float A = FMath::DegreesToRadians(90.f * i) - MapYawRad;
		const FVector2D Pos(FMath::Sin(A) * LetterRadius, -FMath::Cos(A) * LetterRadius);
		CompassLabels[i]->SetRenderTranslation(Pos);
	}

	// --- Markers ---
	const float PixelsPerUnit = WidgetSize / (WorldSize * Zoom);
	const float MaxR = Radius - 10.f;

	for (int32 i = Markers.Num() - 1; i >= 0; --i)
	{
		FMarker& M = Markers[i];

		FVector TargetLoc = M.StaticLocation;
		if (M.bUseActor)
		{
			if (!M.Actor.IsValid())
			{
				if (M.Image)
				{
					M.Image->RemoveFromParent();
				}
				Markers.RemoveAtSwap(i);
				continue;
			}
			TargetLoc = M.Actor->GetActorLocation();
		}

		const FVector Delta = TargetLoc - PlayerLoc;
		// world -> image-space offset (right = +Y, down = -X)
		const FVector2D Img(Delta.Y * PixelsPerUnit, -Delta.X * PixelsPerUnit);
		// image-space -> screen: rotate by -yaw so forward is up
		FVector2D Screen(Img.X * C + Img.Y * S, -Img.X * S + Img.Y * C);

		const float Len = Screen.Size();
		bool bVisible = true;
		if (Len > MaxR)
		{
			if (M.bClamp)
			{
				Screen *= MaxR / Len;
			}
			else
			{
				bVisible = false;
			}
		}

		if (M.Image)
		{
			M.Image->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			M.Image->SetRenderTransform(FWidgetTransform(Screen, FVector2D(1.f, 1.f), FVector2D::ZeroVector, M.bDiamond ? 45.f : 0.f));
		}
	}
}