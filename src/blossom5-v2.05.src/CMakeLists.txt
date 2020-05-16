add_definitions(-DPERFECT_MATCHING_DOUBLE)

add_library(blossom5
        misc.cpp
        PMduals.cpp
        PMexpand.cpp
        PMinit.cpp
        PMinterface.cpp
        PMmain.cpp
        PMrepair.cpp
        PMshrink.cpp
        MinCost/MinCost.cpp
        GEOM/GPMinit.cpp
        GEOM/GPMinterface.cpp
        GEOM/GPMkdtree.cpp
        GEOM/GPMmain.cpp)
