#include <Uefi.h>
#include <Uefi/UefiBaseType.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Protocol/GraphicsOutput.h>


EFI_STATUS EFIAPI UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable){
    
    ///Variabilele folosite	
	EFI_GRAPHICS_OUTPUT_PROTOCOL         *GraphicsOutput;   ///Pointer catre protocolul GraphicsOutput
    EFI_STATUS                            Status;
    UINT32                                Index;
	UINTN                                 SizeOfInfo;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *InfoMode;         ///Un mod video
    EFI_INPUT_KEY                         key;              ///O tasta apasata
	CHAR16                                MyKey;
	UINTN                                 Junk;
	BOOLEAN                               BreakOut;
    
    ///Protocolul care ne da acces la FrameBuffer
	Status = SystemTable->BootServices->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &GraphicsOutput);
    if (EFI_ERROR(Status)) GraphicsOutput = NULL;
    
    
	if (GraphicsOutput != NULL) { ///Daca avem GraphicsOutput
        
		BreakOut=FALSE;
		while (!BreakOut) {   ///Intram in meniul aplicatiei pana cand cineva apasa zero (BreakOut=TRUE)
            
            Print(L"Current Mode: %d X %d \r\n", GraphicsOutput->Mode->Info->HorizontalResolution, GraphicsOutput->Mode->Info->VerticalResolution);
            
            
            ///Scriem meniul plimbandu-ne prin toate modurile video (in Total date de MaxMode)
            for (Index = 0; Index < GraphicsOutput->Mode->MaxMode; Index++){
                Status=GraphicsOutput->QueryMode(GraphicsOutput, Index, &SizeOfInfo, &InfoMode);
                Print(L"Mode ID %d : %d X %d \r\n", (Index+1), InfoMode->HorizontalResolution, InfoMode->VerticalResolution);
            };
            
            
            ///Si optiunea de iesire
            Print(L"        0 : Quit\r\n");
            
            ///Ce vrei sa facem
            Print(L"Select Mode:");
            
            
            ///Asa arata un event in C in lumea EFI
            Status = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &key);
            
            if (Status == EFI_NOT_READY) 
            {
                SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &Junk);
                Status = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &key);
            }
            
            ///Mi-e lene sa tot scriu key.UnicodeChar
            MyKey=key.UnicodeChar;
            
            
            if (MyKey != 0) {///Daca e zero inseamna ca e key.ScanCode, adica taste non-unicode cum e PgUp, Home, F1, etc.
                
                if ((MyKey >= 0x30) && (MyKey < 0x3A)) { ///In Unicode numerele 0-9 sunt 0x30-0x39
			        if (MyKey == 0x30) BreakOut=TRUE; else { ///A apasat 0.
                        MyKey--; ///Din moment ce am incrementat numerele in meniu (0 nu mai e mod video ci e quit), acum trebuie sa corectam.
                        Status=GraphicsOutput->SetMode(GraphicsOutput, (MyKey-0x30));
                        if (EFI_ERROR(Status)) Print(L"Cannot set mode, did you set a number smaller than %d?\n", GraphicsOutput->Mode->MaxMode);
                    }
                } else { Print(L"Non-numeric key pressed"); }
                
            } else { Print(L"Non alpha-numeric key pressed"); }
		}///While !BreakOut
        
	} else { Print(L"No GOP\r\n"); };
    
	return EFI_SUCCESS;
}
