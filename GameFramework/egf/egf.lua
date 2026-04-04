require 'eeProduct'
require 'eePremake'
require 'eeThirdParty'


eeProduct.ProcessProjectEx("egf", "F5F0EFB8-CE62-4175-84C5-FAB7292B5CB3",

    -- The initial creation method
    function ()
        eeGameFramework.SetupLibraryProject(true)
    end,

    -- The "dependencies" method
    function ()
        -- we add the egf SDK to the list of include paths for binary and eval builds that
        -- provide source for egf subprojects but only binaries for the egf library itself.
        -- The egf SDK include path will be searched last so this will work for both source
        -- and binary distributions using this model.
        local SDKIncludePath = "SDK/"..eeCommon.PlatformFolder().."/Include"
        eeProject.AddIncludePath(SDKIncludePath)
    end,

    -- The main setup method
    function ()
        eeCommon.AddEnum(
            -- The source enum as a relative path from root directory:
            "Media/Enums/egfLogIDs.enum",
            -- All base enums listed in order from most derived to least derived
            {
                "Media/Enums/efdLogIDs.enum",
                "Media/Enums/ecrLogIDs.enum"
            },
            -- The generated header file as a relative path from source directory:
            "egfLogIDs.h")

        eeCommon.AddEnum(
            -- The source enum as a relative path from root directory:
            "/Media/Enums/egfMessageIDs.enum",
            -- All base enums listed in order from most derived to least derived
            {
            },
            -- The generated header file as a relative path from source directory:
            "egfMessageIDs.h")

        eeCommon.AddEnum(
            -- The source enum as a relative path from root directory:
            "Media/Enums/egfSystemServiceIDs.enum",
            -- All base enums listed in order from most derived to least derived:
            {
                "Media/Enums/efdSystemServiceIDs.enum"
            },
            -- The generated header file as a relative path from source directory:
            "egfSystemServiceIDs.h")

        eeCommon.AddEnum(
            -- The source enum as a relative path from root directory:
            "Media/Enums/egfPropertyIDs.enum",
            -- All base enums listed in order from most derived to least derived
            {
            },
            -- The generated header file as a relative path from source directory:
            "egfPropertyIDs.h")

        eeCommon.AddEnum(
            -- The source enum as a relative path from root directory:
            "Media/Enums/egfBaseIDs.enum",
            -- All base enums listed in order from most derived to least derived:
            {
                "Media/Enums/efdBaseIDs.enum"
            },
            -- The generated header file as a relative path from source directory:
            "egfBaseIDs.h")

        eeCommon.AddEnum(
            -- The source enum as a relative path from root directory:
            "Media/Enums/egfClassIDs.enum",
            -- All base enums listed in order from most derived to least derived:
            {
                "Media/Enums/efdClassIDs.enum"
            },
            -- The generated header file as a relative path from source directory:
            "egfClassIDs.h")

        eeCommon.AddEnum(
            -- The source enum as a relative path from root directory:
            "Media/Enums/StandardModelLibraryBehaviorIDs.enum",
            -- All base enums listed in order from most derived to least derived:
            {},
            -- The generated header file as a relative path from source directory:
            "StandardModelLibraryBehaviorIDs.h")
        eeCommon.AddEnum(
            -- The source enum as a relative path from root directory:
            "Media/Enums/StandardModelLibraryFlatModelIDs.enum",
            -- All base enums listed in order from most derived to least derived:
            {},
            -- The generated header file as a relative path from source directory:
            "StandardModelLibraryFlatModelIDs.h")
        eeCommon.AddEnum(
            -- The source enum as a relative path from root directory:
            "Media/Enums/StandardModelLibraryPropertyIDs.enum",
            -- All base enums listed in order from most derived to least derived:
            {},
            -- The generated header file as a relative path from source directory:
            "StandardModelLibraryPropertyIDs.h")
    end,

    -- Foundation Dependencies:
    {"efd", "efdNetwork", "efdLogService"}
    -- Framework Dependencies:
    -- Online Dependencies:
    -- Gamebryo Dependencies:
    -- Kernel Dependencies:

)

