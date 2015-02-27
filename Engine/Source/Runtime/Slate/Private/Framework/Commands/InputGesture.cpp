// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"


#define LOCTEXT_NAMESPACE "FInputGesture"


/* FInputGesture interface
 *****************************************************************************/

/**
 * Returns the friendly, localized string name of this key binding
 * @todo Slate: Got to be a better way to do this
 */
FText FInputGesture::GetInputText( ) const
{
#if PLATFORM_MAC
    const FText CommandText = LOCTEXT("KeyName_Control", "Ctrl");
    const FText ControlText = LOCTEXT("KeyName_Command", "Cmd");
#else
    const FText ControlText = LOCTEXT("KeyName_Control", "Ctrl");
    const FText CommandText = LOCTEXT("KeyName_Command", "Cmd"); 
#endif
    const FText AltText = LOCTEXT("KeyName_Alt", "Alt");
    const FText ShiftText = LOCTEXT("KeyName_Shift", "Shift");
    
	const FText AppenderText = LOCTEXT("ModAppender", "+");

	FFormatNamedArguments Args;
	int32 ModCount = 0;

    if (ModifierKeys & EModifierKey::Control)
    {
		Args.Add(FString::Printf(TEXT("Mod%d"),++ModCount), ControlText);
    }
    if (ModifierKeys & EModifierKey::Command)
    {
		Args.Add(FString::Printf(TEXT("Mod%d"),++ModCount), CommandText);
    }
    if (ModifierKeys & EModifierKey::Alt)
    {
		Args.Add(FString::Printf(TEXT("Mod%d"),++ModCount), AltText);
    }
    if (ModifierKeys & EModifierKey::Shift)
    {
		Args.Add(FString::Printf(TEXT("Mod%d"),++ModCount), ShiftText);
    }

	for (int32 i = 1; i <= 4; ++i)
	{
		if (i > ModCount)
		{
			Args.Add(FString::Printf(TEXT("Mod%d"), i), FText::GetEmpty());
			Args.Add(FString::Printf(TEXT("Appender%d"), i), FText::GetEmpty());
		}
		else
		{
			Args.Add(FString::Printf(TEXT("Appender%d"), i), AppenderText);
		}

	}

	Args.Add(TEXT("Key"), GetKeyText());

	return FText::Format(LOCTEXT("FourModifiers", "{Mod1}{Appender1}{Mod2}{Appender2}{Mod3}{Appender3}{Mod4}{Appender4}{Key}"), Args);
}


FText FInputGesture::GetKeyText( ) const
{
	FText OutString; // = KeyGetDisplayName(Key);

	if (Key.IsValid() && !Key.IsModifierKey())
	{
		OutString = Key.GetDisplayName();
	}

	return OutString;
}


#undef LOCTEXT_NAMESPACE