
using UnrealBuildTool;

public class Slither3D : ModuleRules
{
    public Slither3D(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "NetCore",
            "InputCore",
            "EnhancedInput",
            "UMG",
        });

        PrivateDependencyModuleNames.AddRange(new string[] {  });
    }
}
