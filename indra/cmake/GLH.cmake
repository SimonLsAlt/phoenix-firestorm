# -*- cmake -*-
include(Prebuilt)

add_library( ll::glh_linear INTERFACE IMPORTED )

use_conan_binary( glh_linear )
use_prebuilt_binary(glh_linear)
