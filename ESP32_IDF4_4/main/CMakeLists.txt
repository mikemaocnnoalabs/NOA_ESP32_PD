set(APP_SRCS
 "src/APP/NOA_App.cpp"
 )

set(PD_SRCS
 "src/PD/NOA_Pd.cpp"
 )
 
set(DRV_SRCS
 "src/DRV/PDM/FUSB302.cpp"
 "src/DRV/PDM/NCP81239.cpp"
 "src/DRV/PDM/tcpm_driver.cpp"
 "src/DRV/PDM/usb_pd_driver.cpp"
 "src/DRV/PDM/usb_pd_policy.cpp"
 "src/DRV/PDM/usb_pd_protocol.cpp"
 )

set(LIB_PUB_SRCS
 "src/LIB/PUB/NOA_debug.cpp"
 "src/LIB/PUB/NOA_public.cpp"
 )

set(UI_SRCS
 )
 
set(includedirs
 )

set(srcs "main.cpp" "${DRV_SRCS}" "${UI_SRCS}" "${LIB_PUB_SRCS}" "${APP_SRCS}" "${PD_SRCS}")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "${includedirs}")