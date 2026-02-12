// =============================================================================
// IronvalePlayerCharacter.cpp — Player character implementation
// Project Ironvale
// =============================================================================

#include "Characters/IronvalePlayerCharacter.h"
#include "Ironvale.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Combat/IronvaleCombatComponent.h"
#include "Combat/IronvaleStaminaComponent.h"
#include "Combat/IronvaleLockOnComponent.h"
#include "Needs/IronvaleNeedsComponent.h"
#include "Needs/IronvaleEncumbranceComponent.h"
#include "Immersion/IronvaleInteractionComponent.h"
#include "Core/IronvaleStatics.h"

AIronvalePlayerCharacter::AIronvalePlayerCharacter()
{
	// Camera setup
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = FirstPersonArmLength;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Player-specific components
	NeedsComponent = CreateDefaultSubobject<UIronvaleNeedsComponent>(TEXT("NeedsComponent"));
	EncumbranceComponent = CreateDefaultSubobject<UIronvaleEncumbranceComponent>(TEXT("EncumbranceComponent"));
	LockOnComponent = CreateDefaultSubobject<UIronvaleLockOnComponent>(TEXT("LockOnComponent"));
	InteractionComponent = CreateDefaultSubobject<UIronvaleInteractionComponent>(TEXT("InteractionComponent"));

	// Movement defaults
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 400.0f;
		Movement->MaxWalkSpeedCrouched = 200.0f;
		Movement->bCanWalkOffLedges = true;
	}
}

void AIronvalePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add input mapping context
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AIronvalePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AIronvalePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	// Bind movement
	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AIronvalePlayerCharacter::HandleMove);
	}
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AIronvalePlayerCharacter::HandleLook);
	}
	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleJump);
	}

	// Sprint
	if (SprintAction)
	{
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleSprintStart);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AIronvalePlayerCharacter::HandleSprintStop);
	}

	// Combat
	if (AttackAction)
	{
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleAttack);
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Completed, this, &AIronvalePlayerCharacter::HandleAttackRelease);
	}
	if (BlockAction)
	{
		EnhancedInput->BindAction(BlockAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleBlockStart);
		EnhancedInput->BindAction(BlockAction, ETriggerEvent::Completed, this, &AIronvalePlayerCharacter::HandleBlockStop);
	}
	if (LockOnAction)
	{
		EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleLockOnToggle);
	}
	if (CombatDirectionAction)
	{
		EnhancedInput->BindAction(CombatDirectionAction, ETriggerEvent::Triggered, this, &AIronvalePlayerCharacter::HandleCombatDirection);
	}

	// Interaction
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleInteract);
	}
}

// =============================================================================
// INPUT HANDLERS
// =============================================================================

void AIronvalePlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AIronvalePlayerCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AIronvalePlayerCharacter::HandleJump()
{
	Jump();
}

void AIronvalePlayerCharacter::HandleSprintStart()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 600.0f;
	}
}

void AIronvalePlayerCharacter::HandleSprintStop()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 400.0f;
	}
}

void AIronvalePlayerCharacter::HandleAttack(const FInputActionValue& Value)
{
	if (!CombatComponent) return;

	// Convert current directional input to attack direction
	const EIronvaleAttackDirection Direction =
		UIronvaleStatics::InputVectorToAttackDirection(CurrentCombatDirectionInput);

	CombatComponent->StartAttack(Direction);
}

void AIronvalePlayerCharacter::HandleAttackRelease(const FInputActionValue& Value)
{
	if (!CombatComponent) return;
	CombatComponent->ReleaseAttack();
}

void AIronvalePlayerCharacter::HandleBlockStart()
{
	if (!CombatComponent) return;
	CombatComponent->StartBlock();
}

void AIronvalePlayerCharacter::HandleBlockStop()
{
	if (!CombatComponent) return;
	CombatComponent->StopBlock();
}

void AIronvalePlayerCharacter::HandleLockOnToggle()
{
	if (!LockOnComponent) return;
	LockOnComponent->ToggleLockOn();
}

void AIronvalePlayerCharacter::HandleInteract()
{
	if (!InteractionComponent) return;
	InteractionComponent->TryInteract();
}

void AIronvalePlayerCharacter::HandleCombatDirection(const FInputActionValue& Value)
{
	CurrentCombatDirectionInput = Value.Get<FVector2D>();
}

// =============================================================================
// CAMERA
// =============================================================================

void AIronvalePlayerCharacter::ToggleCameraMode()
{
	bIsFirstPerson = !bIsFirstPerson;

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = bIsFirstPerson ? FirstPersonArmLength : ThirdPersonArmLength;
	}

	// In first person, hide the character mesh (arms-only mesh would be used instead)
	if (GetMesh())
	{
		GetMesh()->SetOwnerNoSee(bIsFirstPerson);
	}
}
