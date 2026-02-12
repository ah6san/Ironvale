// =============================================================================
// IronvalePlayerCharacter.h — Player-specific character with input and camera
// Project Ironvale
//
// Extends AIronvaleCharacterBase with:
//   - First-person (optional third-person) camera
//   - Enhanced Input binding for combat, movement, and interaction
//   - Needs component (hunger, thirst, etc.) — player-only
//   - Lock-on component for combat targeting
//   - Interaction trace for world objects
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Characters/IronvaleCharacterBase.h"
#include "InputActionValue.h"
#include "IronvalePlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UIronvaleNeedsComponent;
class UIronvaleEncumbranceComponent;
class UIronvaleLockOnComponent;
class UIronvaleInteractionComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class IRONVALE_API AIronvalePlayerCharacter : public AIronvaleCharacterBase
{
	GENERATED_BODY()

public:
	AIronvalePlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// =========================================================================
	// ADDITIONAL COMPONENT ACCESSORS
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Player")
	UIronvaleNeedsComponent* GetNeedsComponent() const { return NeedsComponent; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Player")
	UIronvaleLockOnComponent* GetLockOnComponent() const { return LockOnComponent; }

	// =========================================================================
	// CAMERA
	// =========================================================================

	/** Toggle between first-person and third-person camera */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Player")
	void ToggleCameraMode();

	UFUNCTION(BlueprintPure, Category = "Ironvale|Player")
	bool IsFirstPerson() const { return bIsFirstPerson; }

protected:
	// --- Camera ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Camera")
	bool bIsFirstPerson = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Camera")
	float FirstPersonArmLength = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Camera")
	float ThirdPersonArmLength = 300.0f;

	// --- Player-specific components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleNeedsComponent* NeedsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleEncumbranceComponent* EncumbranceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleLockOnComponent* LockOnComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleInteractionComponent* InteractionComponent;

	// --- Input Actions (set in Blueprint) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Input")
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Input")
	UInputAction* SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Input")
	UInputAction* AttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Input")
	UInputAction* BlockAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Input")
	UInputAction* LockOnAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Input")
	UInputAction* InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Input")
	UInputAction* CombatDirectionAction;

	// --- Input Handlers ---
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJump();
	void HandleSprintStart();
	void HandleSprintStop();
	void HandleAttack(const FInputActionValue& Value);
	void HandleAttackRelease(const FInputActionValue& Value);
	void HandleBlockStart();
	void HandleBlockStop();
	void HandleLockOnToggle();
	void HandleInteract();
	void HandleCombatDirection(const FInputActionValue& Value);

	/** Current combat direction input (updated each frame from mouse/stick) */
	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Combat")
	FVector2D CurrentCombatDirectionInput = FVector2D::ZeroVector;
};
