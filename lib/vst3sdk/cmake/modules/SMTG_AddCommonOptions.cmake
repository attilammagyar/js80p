
#------------------------------------------------------------------------
# Includes
#------------------------------------------------------------------------
include(SMTG_Platform_Windows)


# Use by default SMTG_ as prefix for ASSERT,...
option(SMTG_RENAME_ASSERT "Rename ASSERT to SMTG_ASSERT" ON)

# Logging
option(SMTG_ENABLE_TARGET_VARS_LOG "Enable Target variables Logging" OFF)

# Create Bundle on Windows for the Plug-ins
if(SMTG_WIN)
    option(SMTG_CREATE_BUNDLE_FOR_WINDOWS "Create Bundle on Windows for the Plug-ins (New since VST 3.6.10!)" ON)
endif(SMTG_WIN)

# Specific Windows Stuff
# Create Symbolic Link for the Plug-ins
if(SMTG_WIN)
    set(DEF_OPT_LINK ON) # be sure to start visual with admin right or adapt the user Group Policy when enabling this
else()
    set(DEF_OPT_LINK ON)
endif(SMTG_WIN)
option(SMTG_CREATE_PLUGIN_LINK "Create symbolic link for each Plug-in (you need to have the Administrator right on Windows! or change the Local Group Policy to allow create symbolic links)" ${DEF_OPT_LINK})

# Specific Mac Stuff
if(SMTG_MAC)
	if(DEFINED SMTG_IOS_DEVELOPMENT_TEAM)
		set(DEF_OPT_BUILD_IOS ON)
	else()
		set(DEF_OPT_BUILD_IOS OFF)
	endif()
    option(SMTG_ENABLE_IOS_TARGETS "Enable building iOS targets" ${DEF_OPT_BUILD_IOS})

    set(SMTG_CODE_SIGN_IDENTITY_MAC "Apple Development" CACHE STRING "macOS Code Sign Identity")
    set(SMTG_CODE_SIGN_IDENTITY_IOS "Apple Development" CACHE STRING "iOS Code Sign Identity")
endif(SMTG_MAC)
