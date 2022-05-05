# NOA Eden Station2

## How to use example


## Example folder contents

```
│  CMakeLists.txt
│  default.csv
│  Makefile
│  README.md											This is the file you are currently reading
│  sdkconfig
│  sdkconfig.debug
│  sdkconfig.defaults
│  sdkconfig.old
│  sdkconfig.source
│  station2_8MB.csv
│  station2_fat8MB.csv
│
└─main
    │  CMakeLists.txt
    │  component.mk
    │  main.cpp
    │
    └─src
        ├─APP
        │      NOA_App.cpp
        │      NOA_App.h
        │
        ├─DRV
        │  └─PDM
        │          FUSB302.cpp
        │          FUSB302.h
        │          NCP81239.cpp
        │          NCP81239.h
        │          tcpm.h
        │          tcpm_driver.cpp
        │          tcpm_driver.h
        │          usb_pd.h
        │          usb_pd_driver.cpp
        │          usb_pd_driver.h
        │          usb_pd_policy.cpp
        │          usb_pd_protocol.cpp
        │          usb_pd_tcpm.h
        │
        ├─LIB
        │  └─PUB
        │          NOA_debug.cpp
        │          NOA_debug.h
        │          NOA_list.cpp
        │          NOA_list.h
        │          NOA_parameter_table.cpp
        │          NOA_parameter_table.h
        │          NOA_public.cpp
        │          NOA_public.h
        │          NOA_syspara.cpp
        │          NOA_syspara.h
        │          NOA_TimeDefs.h
        │          platform.h
        │
        └─PD
                NOA_Pd.cpp
                NOA_Pd.h          
```
