// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TestInterfaceObject.generated.h"

UCLASS()
class UTestInterfaceObject : public UObject, public ITestInterface
{
	GENERATED_UCLASS_BODY()

//	UFUNCTION(BlueprintNativeEvent)
	FString SomeFunction(int32 Val) const;
};