// Copyright Epic Games, Inc. All Rights Reserved.

#include "MGP_2526Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MGP_2526.h"
#include "CableComponent.h"


AMGP_2526Character::AMGP_2526Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	GrappleCable = CreateDefaultSubobject<UCableComponent>(TEXT("Grappling Line"));
	GrappleCable->SetupAttachment(GetRootComponent());
	GrappleCable->SetVisibility(false);
}

void AMGP_2526Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	distanceFromGrapple = (GetActorLocation() - GrapplePoint).Length();
	if (distanceFromGrapple > grappleLength)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue, "hit.GetActor()->GetFName().ToString()");
		GetCharacterMovement()->StopMovementImmediately();
	}

	if (isGrappling)
	{
		SwingBoost = FVector(0.f, 0.f, 0.f);
		GrappleCable->EndLocation = GetActorTransform().InverseTransformPosition(GrapplePoint);
		ForceDirection = GrapplePoint - GetActorLocation();
		if (GetCharacterMovement()->IsFalling() && GetActorLocation().Z < GrapplePoint.Z)
		{
			isSwinging = true;
			Velocity = GetVelocity();

			//pendulum swing
			pendulumDotProduct = ((Velocity.X * ForceDirection.X) + (Velocity.Y * ForceDirection.Y) + (Velocity.Z * ForceDirection.Z));
			PendulumVector = pendulumDotProduct * (ForceDirection.GetSafeNormal()) * -2;

			//force where player is looking
			ForwardBoost.X = FollowCamera->GetForwardVector().X;
			ForwardBoost.Y = FollowCamera->GetForwardVector().Y;
			ForwardBoost.Z = 0;
			ForwardBoost = ForwardBoost.GetSafeNormal() * 10000;

			//boost when reaching bottom of the arc
			pendulumCrossProductX = ((ForceDirection.GetSafeNormal().Y * Velocity.Z) - (ForceDirection.GetSafeNormal().Z * Velocity.Y));
			pendulumCrossProductY = ((ForceDirection.GetSafeNormal().Z * Velocity.X) - (ForceDirection.GetSafeNormal().X) * Velocity.Z);
			pendulumCrossProductZ = ((ForceDirection.GetSafeNormal().X * Velocity.Y) - (ForceDirection.GetSafeNormal().Y) * Velocity.X);
			pendulumCrossProduct = FVector(pendulumCrossProductX, pendulumCrossProductY, pendulumCrossProductZ) * -1;

			PointOnArc = FRotationMatrix::MakeFromZX(ForceDirection, pendulumCrossProduct).Rotator();

			if (PointOnArc.Roll > -10 && PointOnArc.Roll < 30)
			{
				SwingBoost = Velocity.GetSafeNormal() * 1000;
			}

			GetCharacterMovement()->AddForce(PendulumVector + ForwardBoost + SwingBoost);
		}
		else
		{
			isSwinging = false;
		}
		//Gravity compensation
		GetCharacterMovement()->AddForce(ForceDirection.GetSafeNormal() * 10000);
		//pulls the player to the GrapplePoint	
		if (isRetracting)
		{
			GetCharacterMovement()->AddForce(ForceDirection.GetSafeNormal() * 500000);
		}
	}
}

void AMGP_2526Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AMGP_2526Character::DoSprintStart);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMGP_2526Character::DoSprintStop);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMGP_2526Character::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMGP_2526Character::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMGP_2526Character::Look);

		// Grappling
		EnhancedInputComponent->BindAction(GrappleAction, ETriggerEvent::Started, this, &AMGP_2526Character::GrappleStart);
		EnhancedInputComponent->BindAction(GrappleAction, ETriggerEvent::Completed, this, &AMGP_2526Character::GrappleStop);

		//Retracting
		EnhancedInputComponent->BindAction(RetractAction, ETriggerEvent::Started, this, &AMGP_2526Character::RetractStart);
		EnhancedInputComponent->BindAction(RetractAction, ETriggerEvent::Completed, this, &AMGP_2526Character::RetractStop);
	}
	else
	{
		UE_LOG(LogMGP_2526, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMGP_2526Character::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMGP_2526Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AMGP_2526Character::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AMGP_2526Character::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMGP_2526Character::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AMGP_2526Character::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AMGP_2526Character::DoSprintStart()
{
	GetCharacterMovement()->MinAnalogWalkSpeed = 1000.f;
}

void AMGP_2526Character::DoSprintStop()
{
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
}

void AMGP_2526Character::GrappleStart()
{
	FVector start = GetActorLocation();
	FVector forward = FollowCamera->GetForwardVector();
	start = FVector(start.X + (forward.X * 100), start.Y + (forward.Y * 100), start.Z + 75 + (forward.Z * 100));
	FVector end = start + (forward * (grappleLength-2000));
	FHitResult hit;
	FCollisionQueryParams collisionParams;
	collisionParams.AddIgnoredActor(this);

	if (GetWorld())
	{
		World = GetWorld();
		bool actorHit = World->LineTraceSingleByChannel(hit, start, end, ECC_Pawn, collisionParams, FCollisionResponseParams());
		FCollisionShape::MakeSphere(50.f);


		// debug line
		//DrawDebugLine(World, start, end, FColor::Red, false, 2.f, 0.f, 10.f);
		if (actorHit && hit.GetActor())
		{
			//make actor class here
			//Grapple = World->SpawnActor<AActor>(GrappleClass, SpawnLocation, SpawnRotation, FActorSpawnParameters());

			isGrappling = true;
			GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
			GrappleCable->SetVisibility(true);
			GrapplePoint = hit.ImpactPoint;

			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, hit.GetActor()->GetFName().ToString());
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, hit.ImpactPoint.ToString());
		}
	}
}

void AMGP_2526Character::GrappleStop()
{
	isGrappling = false;
	if (!GetCharacterMovement()->IsFalling())
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	}
	GrappleCable->SetVisibility(false);
}

void AMGP_2526Character::RetractStart()
{
	isRetracting = true;
}

void AMGP_2526Character::RetractStop()
{
	isRetracting = false;
}
